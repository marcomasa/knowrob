/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_KNOWLEDGE_BASE_H
#define KNOWROB_KNOWLEDGE_BASE_H

#include <memory>
#include <boost/property_tree/ptree.hpp>
#include <utility>
#include "knowrob/queries/QueryContext.h"
#include "knowrob/backend/QueryableBackend.h"
#include "knowrob/backend/BackendManager.h"
#include "knowrob/backend/BackendInterface.h"
#include "knowrob/triples/GraphPathQuery.h"
#include "knowrob/ontologies/OntologySource.h"

namespace knowrob {
	// forward declaration
	class ReasonerManager;

	/**
	 * The main interface to the knowledge base system implementing
	 * its 'tell' and 'ask' interface.
	 */
	class KnowledgeBase {
	public:
		/**
		 * @param config a property tree used to configure this.
		 */
		explicit KnowledgeBase(const boost::property_tree::ptree &config);

		/**
		 * @param configFile path to file that encodes a boost property tree used to configure the KB.
		 */
		explicit KnowledgeBase(const std::string_view &configFile);

		KnowledgeBase();

		~KnowledgeBase();

		void init();

		void loadCommon();

		/**
		 * Load a data source into the knowledge base.
		 * This will potentially load the data source into multiple backends
		 * depending on which data formats are supported by the backends.
		 * @param source the data source to load
		 * @return true if the data source was loaded successfully
		 */
		bool loadDataSource(const DataSourcePtr &source);

		/**
		 * @return the vocabulary of this knowledge base, i.e. all known properties and classes
		 */
		auto &vocabulary() const { return vocabulary_; }

		auto &edb() const { return edb_; }

		QueryableBackendPtr getBackendForQuery() const;

		/**
		 * @param property a property IRI
		 * @return true if the property is materialized in the EDB
		 */
		bool isMaterializedInEDB(std::string_view property) const;

		/**
		 * Evaluate a query represented as a vector of literals.
		 * The call is non-blocking and returns a stream of answers.
		 * @param graphQuery a graph path query
		 * @return a stream of query results
		 */
		TokenBufferPtr submitQuery(const GraphPathQueryPtr &graphQuery);

		/**
		 * Evaluate a query represented as a Literal.
		 * The call is non-blocking and returns a stream of answers.
		 * @param query a literal
		 * @param ctx a query context
		 * @return a stream of query results
		 */
		TokenBufferPtr submitQuery(const FirstOrderLiteralPtr &query, const QueryContextPtr &ctx);

		/**
		 * Evaluate a query represented as a Formula.
		 * The call is non-blocking and returns a stream of answers.
		 * @param query a formula
		 * @param ctx a query context
		 * @return a stream of query results
		 */
		TokenBufferPtr submitQuery(const FormulaPtr &query, const QueryContextPtr &ctx);

		auto &reasonerManager() const { return reasonerManager_; }

		auto &backendManager() const { return backendManager_; }

		bool insertOne(const FramedTriple &triple);

		bool insertAll(const TripleContainerPtr &triples);

		bool insertAll(const std::vector<FramedTriplePtr> &triples);

		bool removeOne(const FramedTriple &triple);

		bool removeAll(const TripleContainerPtr &triples);

		bool removeAll(const std::vector<FramedTriplePtr> &triples);

		bool removeAllWithOrigin(std::string_view origin);

	protected:
		std::shared_ptr<BackendInterface> edb_;
		std::shared_ptr<ReasonerManager> reasonerManager_;
		std::shared_ptr<BackendManager> backendManager_;
		std::shared_ptr<Vocabulary> vocabulary_;
		bool isInitialized_;

		void configure(const boost::property_tree::ptree &config);

		static void configurePrefixes(const boost::property_tree::ptree &config);

		void configureDataSources(const boost::property_tree::ptree &config);

		void configureBackends(const boost::property_tree::ptree &config);

		void configureReasoner(const boost::property_tree::ptree &config);

		void initVocabulary();

		void initBackends();

		void synchronizeBackends();

		std::shared_ptr<NamedBackend> findSourceBackend(const FramedTriple &triple);

		DataSourcePtr createDataSource(const boost::property_tree::ptree &subtree);

		void startReasoner();

		void stopReasoner();

		std::vector<std::shared_ptr<NamedBackend>>
		prepareLoad(std::string_view origin, std::string_view newVersion) const;

		void
		finishLoad(const std::shared_ptr<OntologySource> &source, std::string_view origin, std::string_view newVersion);

		bool loadNonOntologySource(const DataSourcePtr &source) const;

		bool loadOntologySource(const std::shared_ptr<OntologySource> &source);

		std::optional<std::string> getVersionOfOrigin(const std::shared_ptr<NamedBackend> &definedBackend,
													  std::string_view origin) const;
	};

	using KnowledgeBasePtr = std::shared_ptr<KnowledgeBase>;
}

#endif //KNOWROB_KNOWLEDGE_BASE_H
