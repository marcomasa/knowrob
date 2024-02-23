/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_KNOWLEDGE_BASE_H
#define KNOWROB_KNOWLEDGE_BASE_H

#include <memory>
#include <boost/property_tree/ptree.hpp>
#include <utility>
#include "ThreadPool.h"
#include "knowrob/formulas/DependencyGraph.h"
#include "knowrob/queries/QueryPipeline.h"
#include "knowrob/queries/QueryContext.h"
#include "knowrob/semweb/RDFComputable.h"
#include "knowrob/reasoner/DefinedReasoner.h"
#include "knowrob/db/OntologyFile.h"
#include "knowrob/db/QueryableBackend.h"
#include "knowrob/db/DefinedBackend.h"

namespace knowrob {
	enum QueryFlag {
		QUERY_FLAG_ALL_SOLUTIONS = 1 << 0,
		QUERY_FLAG_ONE_SOLUTION = 1 << 1,
		QUERY_FLAG_PERSIST_SOLUTIONS = 1 << 2,
		QUERY_FLAG_UNIQUE_SOLUTIONS = 1 << 3,
		//QUERY_FLAG_ORDER_PRESERVING = 1 << 4,
	};

	// forward declaration
	class ReasonerManager;

	// forward declaration
	class BackendManager;

	/**
	 * The main interface to the knowledge base system implementing
	 * its 'tell' and 'ask' interface.
	 */
	class KnowledgeBase : public IDataBackend {
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

		/**
		 * @return the set of reasoner of this KB.
		 */
		const std::map<std::string, std::shared_ptr<DefinedReasoner>, std::less<>> &reasonerPool() const;

		/**
		 * Load a data source into the knowledge base.
		 * This will potentially load the data source into multiple backends
		 * depending on which data formats are supported by the backends.
		 * @param source the data source to load
		 * @return true if the data source was loaded successfully
		 */
		bool loadDataSource(const DataSourcePtr &source);


		QueryableBackendPtr getBackendForQuery(const RDFLiteralPtr &query, const QueryContextPtr &ctx) const;

		QueryableBackendPtr
		getBackendForQuery(const std::vector<RDFLiteralPtr> &query, const QueryContextPtr &ctx) const;

		/**
		 * @return the vocabulary of this knowledge base, i.e. all known properties and classes
		 */
		auto vocabulary() const { return vocabulary_; }

		/**
		 * @param property a property IRI
		 * @return true if the property is materialized in the EDB
		 */
		bool isMaterializedInEDB(std::string_view property) const;

		/**
		 * @return import hierarchy of named graphs
		 */
		auto importHierarchy() const { return importHierarchy_; }

		/**
		 * Evaluate a query represented as a vector of literals.
		 * The call is non-blocking and returns a stream of answers.
		 * @param literals a vector of literals
		 * @param label an optional modalFrame label
		 * @return a stream of query results
		 */
		TokenBufferPtr submitQuery(const ConjunctiveQueryPtr &graphQuery);

		/**
		 * Evaluate a query represented as a Literal.
		 * The call is non-blocking and returns a stream of answers.
		 * @param query a literal
		 * @return a stream of query results
		 */
		TokenBufferPtr submitQuery(const FirstOrderLiteralPtr &query, const QueryContextPtr &ctx);

		/**
		 * Evaluate a query represented as a Formula.
		 * The call is non-blocking and returns a stream of answers.
		 * @param query a formula
		 * @return a stream of query results
		 */
		TokenBufferPtr submitQuery(const FormulaPtr &query, const QueryContextPtr &ctx);

		auto &reasonerManager() const { return reasonerManager_; }

		auto &backendManager() const { return backendManager_; }

		void init();

		// override IDataBackend
		bool insertOne(const FramedTriple &triple) override;

		// override IDataBackend
		bool insertAll(const semweb::TripleContainerPtr &triples) override;

		bool insertAll(const std::vector<FramedTriplePtr> &triples);

		// override IDataBackend
		bool removeOne(const FramedTriple &triple) override;

		// override IDataBackend
		bool removeAll(const semweb::TripleContainerPtr &triples) override;

		bool removeAll(const std::vector<FramedTriplePtr> &triples);

		// override IDataBackend
		bool removeAllWithOrigin(std::string_view origin) override;

		// override IDataBackend
		bool removeAllMatching(const RDFLiteral &query) override;

	protected:
		std::shared_ptr<ReasonerManager> reasonerManager_;
		std::shared_ptr<BackendManager> backendManager_;
		std::shared_ptr<semweb::Vocabulary> vocabulary_;
		std::shared_ptr<semweb::ImportHierarchy> importHierarchy_;
		uint32_t tripleBatchSize_;
		bool isInitialized_;

		// used to sort dependency nodes in a priority queue.
		// the nodes are considered to be dependent on each other through free variables.
		// the priority value is used to determine which nodes should be evaluated first.
		struct DependencyNodeComparator {
			bool operator()(const DependencyNodePtr &a, const DependencyNodePtr &b) const;
		};

		struct DependencyNodeQueue {
			const DependencyNodePtr node_;
			std::priority_queue<DependencyNodePtr, std::vector<DependencyNodePtr>, DependencyNodeComparator> neighbors_;

			explicit DependencyNodeQueue(const DependencyNodePtr &node);
		};

		// compares literals
		struct EDBComparator {
			explicit EDBComparator(semweb::VocabularyPtr vocabulary)
					: vocabulary_(std::move(vocabulary)) {}

			bool operator()(const RDFLiteralPtr &a, const RDFLiteralPtr &b) const;

			semweb::VocabularyPtr vocabulary_;
		};

		struct IDBComparator {
			explicit IDBComparator(semweb::VocabularyPtr vocabulary)
					: vocabulary_(std::move(vocabulary)) {}

			bool operator()(const RDFComputablePtr &a, const RDFComputablePtr &b) const;

			semweb::VocabularyPtr vocabulary_;
		};

		void construct();

		void initFromConfig(const boost::property_tree::ptree &config);

		void loadConfiguration(const boost::property_tree::ptree &config);

		static DataSourcePtr createDataSource(const boost::property_tree::ptree &subtree);

		void startReasoner();

		void stopReasoner();

		DataBackendPtr findSourceBackend(const FramedTriple &triple);

		std::vector<std::shared_ptr<DefinedBackend>> prepareLoad(std::string_view origin, std::string_view newVersion) const;

		void finishLoad(const std::shared_ptr<OntologySource> &source, std::string_view origin, std::string_view newVersion);

		bool loadNonOntologySource(const DataSourcePtr &source) const;

		bool loadOntologyFile(const std::shared_ptr<OntologyFile> &source, bool followImports = true);

		bool loadSPARQLDataSource(const std::shared_ptr<DataSource> &source);

		bool insertAllInto(const semweb::TripleContainerPtr &triples, const std::vector<std::shared_ptr<DefinedBackend>> &backends);

		static DataSourceType getDataSourceType(const std::string &format, const boost::optional<std::string> &language,
												const boost::optional<std::string> &type);

		void updateVocabularyInsert(const FramedTriple &tripleData);

		void updateVocabularyRemove(const FramedTriple &tripleData);

		std::optional<std::string> getVersionOfOrigin(const std::shared_ptr<DefinedBackend> &definedBackend,
													  std::string_view origin) const;

		std::vector<RDFComputablePtr> createComputationSequence(
				const std::list<DependencyNodePtr> &dependencyGroup) const;

		void createComputationPipeline(
				const std::shared_ptr<QueryPipeline> &pipeline,
				const std::vector<RDFComputablePtr> &computableLiterals,
				const std::shared_ptr<TokenBroadcaster> &pipelineInput,
				const std::shared_ptr<TokenBroadcaster> &pipelineOutput,
				const QueryContextPtr &ctx) const;
	};

	using KnowledgeBasePtr = std::shared_ptr<KnowledgeBase>;
}

#endif //KNOWROB_KNOWLEDGE_BASE_H
