/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <thread>
#include <boost/property_tree/json_parser.hpp>
#include <knowrob/Logger.h>
#include <knowrob/KnowledgeBase.h>
#include <knowrob/URI.h>
#include "knowrob/queries/QueryPipeline.h"
#include "knowrob/triples/TripleFormat.h"
#include "knowrob/semweb/PrefixRegistry.h"
#include "knowrob/semweb/rdf.h"
#include "knowrob/semweb/owl.h"
#include "knowrob/semweb/rdfs.h"
#include "knowrob/semweb/OntologyLanguage.h"
#include "knowrob/reasoner/ReasonerManager.h"
#include "knowrob/ontologies/OntologyFile.h"
#include "knowrob/ontologies/SPARQLService.h"
#include "knowrob/ontologies/GraphTransformation.h"
#include "knowrob/ontologies/TransformedOntology.h"

using namespace knowrob;

KnowledgeBase::KnowledgeBase()
		: isInitialized_(false) {
	vocabulary_ = std::make_shared<Vocabulary>();
	// use "system" as default origin until initialization completed
	vocabulary_->importHierarchy()->setDefaultGraph(ImportHierarchy::ORIGIN_SYSTEM);
	backendManager_ = std::make_shared<BackendManager>(vocabulary_);
	reasonerManager_ = std::make_shared<ReasonerManager>(this, backendManager_);
	edb_ = std::make_shared<BackendInterface>(backendManager_);
}

KnowledgeBase::KnowledgeBase(const boost::property_tree::ptree &config) : KnowledgeBase() {
	configure(config);
	init();
}

KnowledgeBase::KnowledgeBase(const std::string_view &configFile) : KnowledgeBase() {
	boost::property_tree::ptree config;
	boost::property_tree::read_json(URI::resolve(configFile), config);
	configure(config);
	init();
}

KnowledgeBase::~KnowledgeBase() {
	stopReasoner();
}

void KnowledgeBase::init() {
	isInitialized_ = true;
	vocabulary_->importHierarchy()->setDefaultGraph(ImportHierarchy::ORIGIN_USER);
	initBackends();
	synchronizeBackends();
	initVocabulary();
	startReasoner();
}

void KnowledgeBase::initBackends() {
	for (auto &pair: backendManager_->plugins()) {
		auto definedBackend = pair.second;
		definedBackend->value()->setVocabulary(vocabulary_);
	}
}

void KnowledgeBase::synchronizeBackends() {
	// TODO: support initial synch of persistent with non-persistent data backends.
	//       e.g. mongo KG could be filled with facts initially, these should be mirrored into
	//       other backends.
	//       at least make sure no persistent backend has an outdated version of an ontology
}

void KnowledgeBase::initVocabulary() {
	auto v_s = std::make_shared<Variable>("?s");
	auto v_o = std::make_shared<Variable>("?o");

	for (auto &it: backendManager_->persistent()) {
		auto backend = it.second;

		// initialize the import hierarchy
		for (auto &origin: backend->getOrigins()) {
			vocabulary_->importHierarchy()->addDirectImport(vocabulary_->importHierarchy()->ORIGIN_SYSTEM, origin);
		}

		// iterate over all rdf:type assertions and add them to the vocabulary
		backend->match(FramedTriplePattern(v_s, rdf::type, v_o),
					   [this](const FramedTriplePtr &triple) {
						   vocabulary_->addResourceType(triple->subject(), triple->valueAsString());
					   });
		// iterate over all rdfs::subClassOf assertions and add them to the vocabulary
		backend->match(FramedTriplePattern(v_s, rdfs::subClassOf, v_o),
					   [this](const FramedTriplePtr &triple) {
						   vocabulary_->addSubClassOf(triple->subject(), triple->valueAsString());
					   });
		// iterate over all rdfs::subPropertyOf assertions and add them to the vocabulary
		backend->match(FramedTriplePattern(v_s, rdfs::subPropertyOf, v_o),
					   [this](const FramedTriplePtr &triple) {
						   vocabulary_->addSubPropertyOf(triple->subject(), triple->valueAsString());
					   });
		// iterate over all owl::inverseOf assertions and add them to the vocabulary
		backend->match(FramedTriplePattern(v_s, owl::inverseOf, v_o),
					   [this](const FramedTriplePtr &triple) {
						   vocabulary_->setInverseOf(triple->subject(), triple->valueAsString());
					   });

		// query number of assertions of each property/class.
		// this is useful information for optimizing the query planner.
		std::vector<semweb::PropertyPtr> reifiedProperties;
		backend->count([this, &reifiedProperties](std::string_view resource, uint64_t count) {
			// special handling for reified relations: they are concepts, but do also increase the relation counter
			auto reifiedProperty = vocabulary_->getDefinedReification(resource);
			if (reifiedProperty) reifiedProperties.push_back(reifiedProperty);
			vocabulary_->setFrequency(resource, count);
		});
		for (auto &p: reifiedProperties) {
			vocabulary_->increaseFrequency(p->iri());
		}
	}
}

void KnowledgeBase::configure(const boost::property_tree::ptree &config) {
	configurePrefixes(config);
	// initialize data backends from configuration
	configureBackends(config);
	// load reasoners from configuration
	configureReasoner(config);
	// share vocabulary and import hierarchy with backends
	initBackends();
	// load common ontologies
	loadCommon();
	// load the "global" data sources.
	// these are data sources that are loaded into all backends, however
	// the backends may decide to ignore some of the data sources.
	configureDataSources(config);
}

void KnowledgeBase::configurePrefixes(const boost::property_tree::ptree &config) {
	auto semwebTree = config.get_child_optional("semantic-web");
	if (semwebTree) {
		// load RDF URI aliases
		auto prefixesList = semwebTree.value().get_child_optional("prefixes");
		for (const auto &pair: prefixesList.value()) {
			auto alias = pair.second.get("alias", "");
			auto uri = pair.second.get("uri", "");
			if (!alias.empty() && !uri.empty()) {
				PrefixRegistry::registerPrefix(alias, uri);
			} else {
				KB_WARN("Invalid entry in semantic-web::prefixes, 'alias' and 'uri' must be defined.");
			}
		}
	}
}

void KnowledgeBase::configureBackends(const boost::property_tree::ptree &config) {
	auto backendList = config.get_child_optional("data-backends");
	if (backendList) {
		for (const auto &pair: backendList.value()) {
			KB_LOGGED_TRY_CATCH(pair.first, "load", {
				backendManager_->loadPlugin(pair.second);
			});
		}
	} else {
		KB_ERROR("configuration has no 'backends' key.");
	}
}

void KnowledgeBase::configureReasoner(const boost::property_tree::ptree &config) {
	auto reasonerList = config.get_child_optional("reasoner");
	if (reasonerList) {
		for (const auto &pair: reasonerList.value()) {
			KB_LOGGED_TRY_CATCH(pair.first, "load", {
				auto definedReasoner = reasonerManager_->loadPlugin(pair.second);
				// if reasoner implements DataBackend class, add it to the backend manager
				auto reasonerBackend = std::dynamic_pointer_cast<DataBackend>(definedReasoner->value());
				if (reasonerBackend) {
					backendManager_->addPlugin(definedReasoner->name(), reasonerBackend);
				}
			});
		}
	} else {
		KB_ERROR("configuration has no 'reasoner' key.");
	}
}

void KnowledgeBase::loadCommon() {
	for (auto &ontoPath: {"owl/rdf-schema.xml", "owl/owl.rdf"}) {
		loadDataSource(std::make_shared<OntologyFile>(vocabulary_, URI(ontoPath), "rdf-xml"));
	}
}

void KnowledgeBase::startReasoner() {
	for (auto &pair: reasonerManager_->dataDriven()) {
		// TODO: it would be better to remove reasoner and backends if they throw an exception.
		//       but doing this for e.g. query evaluation is more difficult where exception occur in a worker thread
		//       as part of a complex query evaluation pipeline.
		KB_LOGGED_TRY_CATCH(pair.first.data(), "start", {
			pair.second->start();
		});
	}
}

void KnowledgeBase::stopReasoner() {
	for (auto &pair: reasonerManager_->dataDriven()) {
		KB_LOGGED_TRY_CATCH(pair.first.data(), "stop", {
			pair.second->stop();
		});
	}
}

bool KnowledgeBase::isMaterializedInEDB(std::string_view property) const {
	return vocabulary_->frequency(property) > 0;
}

QueryableBackendPtr KnowledgeBase::getBackendForQuery() const {
	auto &queryable = backendManager_->queryable();
	if (queryable.empty()) {
		KB_WARN("No queryable backends available.");
		return nullptr;
	} else {
		return queryable.begin()->second;
	}
}

TokenBufferPtr KnowledgeBase::submitQuery(const GraphPathQueryPtr &graphQuery) {
	auto pipeline = std::make_shared<QueryPipeline>(this, graphQuery);
	// Wrap output into AnswerBuffer_WithReference object.
	// Note that the AnswerBuffer_WithReference object is used such that the caller can
	// destroy the whole pipeline by de-referencing the returned AnswerBufferPtr.
	auto out = std::make_shared<AnswerBuffer_WithReference>(pipeline);
	*pipeline >> out;
	pipeline->stopBuffering();
	return out;
}

TokenBufferPtr KnowledgeBase::submitQuery(const FirstOrderLiteralPtr &literal, const QueryContextPtr &ctx) {
	auto rdfLiteral = std::make_shared<FramedTriplePattern>(
			literal->predicate(), literal->isNegated());
	rdfLiteral->setTripleFrame(ctx->selector);
	return submitQuery(std::make_shared<GraphPathQuery>(
			GraphPathQuery({rdfLiteral}, ctx)));
}

TokenBufferPtr KnowledgeBase::submitQuery(const FormulaPtr &phi, const QueryContextPtr &ctx) {
	auto pipeline = std::make_shared<QueryPipeline>(this, phi, ctx);
	auto out = std::make_shared<AnswerBuffer_WithReference>(pipeline);
	*pipeline >> out;
	pipeline->stopBuffering();
	return out;
}

bool KnowledgeBase::insertOne(const FramedTriple &triple) {
	auto sourceBackend = findSourceBackend(triple);
	auto transaction = edb_->createTransaction(
			getBackendForQuery(),
			BackendInterface::Insert,
			BackendInterface::Excluding,
			{sourceBackend});
	return transaction->commit(triple);
}

bool KnowledgeBase::insertAll(const TripleContainerPtr &triples) {
	auto sourceBackend = findSourceBackend(**triples->begin());
	auto transaction = edb_->createTransaction(
			getBackendForQuery(),
			BackendInterface::Insert,
			BackendInterface::Excluding,
			{sourceBackend});
	return transaction->commit(triples);
}

bool KnowledgeBase::removeOne(const FramedTriple &triple) {
	auto sourceBackend = findSourceBackend(triple);
	auto transaction = edb_->createTransaction(
			getBackendForQuery(),
			BackendInterface::Remove,
			BackendInterface::Excluding,
			{sourceBackend});
	return transaction->commit(triple);
}

bool KnowledgeBase::removeAll(const TripleContainerPtr &triples) {
	auto sourceBackend = findSourceBackend(**triples->begin());
	auto transaction = edb_->createTransaction(
			getBackendForQuery(),
			BackendInterface::Remove,
			BackendInterface::Excluding,
			{sourceBackend});
	return transaction->commit(triples);
}

bool KnowledgeBase::insertAll(const std::vector<FramedTriplePtr> &triples) {
	// Note: insertAll blocks until the triples are inserted, so it is safe to use the triples vector as a pointer.
	return insertAll(std::make_shared<ProxyTripleContainer>(&triples));
}

bool KnowledgeBase::removeAll(const std::vector<FramedTriplePtr> &triples) {
	return removeAll(std::make_shared<ProxyTripleContainer>(&triples));
}

bool KnowledgeBase::removeAllWithOrigin(std::string_view origin) {
	return edb_->removeAllWithOrigin(origin);
}

std::shared_ptr<NamedBackend> KnowledgeBase::findSourceBackend(const FramedTriple &triple) {
	if (!triple.graph()) return {};

	auto definedBackend_withID = backendManager_->getPluginWithID(triple.graph().value());
	if (definedBackend_withID) return definedBackend_withID;

	auto definedReasoner = reasonerManager_->getPluginWithID(triple.graph().value());
	if (definedReasoner) {
		auto reasonerBackend = reasonerManager_->getReasonerBackend(definedReasoner);
		if (reasonerBackend) {
			for (auto &it: backendManager_->plugins()) {
				auto &definedBackend_ofReasoner = it.second;
				if (definedBackend_ofReasoner->value() == reasonerBackend) {
					return definedBackend_ofReasoner;
				}
			}
		}
	}

	return {};
}

static bool isOntologySourceType(
		const std::string &format,
		const boost::optional<std::string> &language,
		const boost::optional<std::string> &type) {
	if (type && (type.value() == "ontology" || type.value() == "sparql")) return true;
	if (language && semweb::isOntologyLanguageString(language.value())) return true;
	if (semweb::isTripleFormatString(format)) return true;
	return false;
}

void KnowledgeBase::configureDataSources(const boost::property_tree::ptree &config) {
	auto dataSourcesList = config.get_child_optional("data-sources");
	if (dataSourcesList) {
		for (const auto &pair: dataSourcesList.value()) {
			auto &subtree = pair.second;
			auto dataSource = createDataSource(subtree);
			if (!dataSource) {
				KB_ERROR("Failed to create data source \"{}\".", pair.first);
				continue;
			}
			bool has_error;
			auto transformationConfig = config.get_child_optional("transformation");
			if (transformationConfig) {
				if (dataSource->dataSourceType() != DataSourceType::ONTOLOGY) {
					KB_ERROR("Transformations can only be applied on ontology data sources.");
					continue;
				}
				auto ontology = std::static_pointer_cast<OntologySource>(dataSource);
				// apply a transformation to the data source if "transformation" key is present
				auto transformation = GraphTransformation::create(transformationConfig.value());
				auto transformed = std::make_shared<TransformedOntology>(URI(ontology->uri()), ontology->format());
				transformation->apply(*ontology, [&transformed](const TripleContainerPtr &triples) {
					transformed->storage()->insertAll(triples);
				});
				has_error = !loadDataSource(transformed);
			} else {
				has_error = !loadDataSource(dataSource);
			}
			if (has_error) {
				KB_ERROR("Failed to load data source from \"{}\".", dataSource->uri());
			}
		}
	}
}

DataSourcePtr KnowledgeBase::createDataSource(const boost::property_tree::ptree &subtree) {
	static const std::string formatDefault = {};

	// read data source settings
	URI dataSourceURI(subtree);
	auto dataSourceFormat = subtree.get("format", formatDefault);
	auto o_dataSourceLanguage = subtree.get_optional<std::string>("language");
	auto o_type = subtree.get_optional<std::string>("type");
	auto isOntology = isOntologySourceType(dataSourceFormat, o_dataSourceLanguage, o_type);
	// an optional frame can be applied to all triples in a data source
	auto o_tripleFrame = subtree.get_child_optional("frame");
	std::shared_ptr<GraphSelector> tripleFrame;
	if (o_tripleFrame) {
		tripleFrame = std::make_shared<GraphSelector>();
		tripleFrame->set(*o_tripleFrame);
	}

	if (isOntology && o_type && o_type.value() == "sparql") {
		auto sparqlService = std::make_shared<SPARQLService>(dataSourceURI, dataSourceFormat);
		if (tripleFrame) {
			sparqlService->setFrame(tripleFrame);
		}
		return sparqlService;
	} else if (isOntology) {
		auto ontoFile = std::make_shared<OntologyFile>(vocabulary_, dataSourceURI, dataSourceFormat);
		if (o_dataSourceLanguage.has_value()) {
			ontoFile->setOntologyLanguage(semweb::ontologyLanguageFromString(o_dataSourceLanguage.value()));
		}
		if (tripleFrame) {
			ontoFile->setFrame(tripleFrame);
		}
		return ontoFile;
	} else {
		return std::make_shared<DataSource>(dataSourceURI, dataSourceFormat, DataSourceType::UNSPECIFIED);
	}
}

bool KnowledgeBase::loadDataSource(const DataSourcePtr &source) {
	switch (source->dataSourceType()) {
		case DataSourceType::ONTOLOGY:
			return loadOntologySource(std::static_pointer_cast<OntologySource>(source));
		case DataSourceType::UNSPECIFIED:
			return loadNonOntologySource(source);
	}
	return false;
}

std::optional<std::string> KnowledgeBase::getVersionOfOrigin(
		const std::shared_ptr<NamedBackend> &definedBackend, std::string_view origin) const {
	// check if the origin was loaded before in this session
	auto runtimeVersion = definedBackend->value()->getVersionOfOrigin(origin);
	if (runtimeVersion) return runtimeVersion;
	// otherwise check if the backend is persistent and if so, ask the persistent backend
	auto persistentBackend = backendManager_->persistent().find(definedBackend->name());
	if (persistentBackend != backendManager_->persistent().end()) {
		return persistentBackend->second->getVersionOfOrigin(origin);
	}
	return {};
}

std::vector<std::shared_ptr<NamedBackend>>
KnowledgeBase::prepareLoad(std::string_view origin, std::string_view newVersion) const {
	std::vector<std::shared_ptr<NamedBackend>> backendsToLoad;
	for (auto &it: backendManager_->plugins()) {
		// check if the ontology is already loaded by the backend,
		// and if so whether it has the right version.
		auto definedBackend = it.second;
		auto currentVersion = getVersionOfOrigin(definedBackend, origin);
		if (currentVersion.has_value()) {
			if (currentVersion.value() != newVersion) {
				backendsToLoad.emplace_back(it.second);
				// TODO: rather include this operation as part of the transaction below
				definedBackend->value()->removeAllWithOrigin(origin);
			}
		} else {
			backendsToLoad.emplace_back(it.second);
		}
	}
	return backendsToLoad;
}

void KnowledgeBase::finishLoad(const std::shared_ptr<OntologySource> &source, std::string_view origin,
							   std::string_view newVersion) {
	// update the version triple
	for (auto &it: backendManager_->plugins()) {
		it.second->value()->setVersionOfOrigin(origin, newVersion);
	}
	for (auto &it: backendManager_->persistent()) {
		auto persistentBackend = it.second;
		persistentBackend->setVersionOfOrigin(origin, newVersion);
	}

	// add direct import
	if (source->parentOrigin().has_value()) {
		vocabulary_->importHierarchy()->addDirectImport(source->parentOrigin().value(), origin);
	} else {
		vocabulary_->importHierarchy()->addDirectImport(vocabulary_->importHierarchy()->defaultGraph(), origin);
	}
}

bool KnowledgeBase::loadOntologySource(const std::shared_ptr<OntologySource> &source) { // NOLINT(misc-no-recursion)
	auto uri = URI::resolve(source->uri());
	// SPARQL does not have versioning. Some endpoints may store the version as a triple,
	// but this is not standardized. Some may encode version in the URI which we try to extract
	// below. Otherwise, we just use the current day as version causing a re-load every day.
	auto newVersion = DataSource::getVersionFromURI(uri);

	// get all backends that do not have the data loaded yet
	auto backendsToLoad = prepareLoad(source->origin(), newVersion);
	if (backendsToLoad.empty()) {
		// data is already loaded
		return true;
	}

	auto result = source->load([this, &backendsToLoad](const TripleContainerPtr &triples) {
		auto transaction = edb_->createTransaction(
				getBackendForQuery(),
				BackendInterface::Insert,
				BackendInterface::Including,
				backendsToLoad);
		transaction->commit(triples);
	});
	if (!result) {
		KB_WARN("Failed to load ontology \"{}\".", uri);
		return false;
	}
	finishLoad(source, source->origin(), newVersion);

	for (auto &imported : source->imports()) {
		auto importedSource = std::make_shared<OntologyFile>(vocabulary_, URI(imported), source->format());
		if (!loadOntologySource(importedSource)) {
			KB_WARN("Failed to load imported ontology \"{}\".", imported);
			return false;
		}
	}

	return true;
}

bool KnowledgeBase::loadNonOntologySource(const DataSourcePtr &source) const {
	bool hasHandler = false;
	bool allSucceeded = true;

	for (auto &kg_pair: backendManager_->plugins()) {
		auto backend = kg_pair.second->value();
		if (backend->hasDataHandler(source)) {
			if (!backend->loadDataSource(source)) {
				allSucceeded = false;
				KB_WARN("backend '{}' failed to load data source '{}'", kg_pair.first, source->uri());
			}
			hasHandler = true;
		}
	}

	if (!hasHandler) {
		KB_WARN("no data handler for data source format '{}'", source->format());
	}

	return hasHandler && allSucceeded;
}
