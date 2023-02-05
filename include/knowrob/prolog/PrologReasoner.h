/*
 * Copyright (c) 2022, Daniel Beßler
 * All rights reserved.
 *
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_PROLOG_REASONER_H_
#define KNOWROB_PROLOG_REASONER_H_

// STD
#include <string>
#include <list>
#include <filesystem>
#include <map>
#include <memory>
// gtest
#include <gtest/gtest.h>
// KnowRob
#include <knowrob/ThreadPool.h>
#include <knowrob/terms.h>
#include <knowrob/queries.h>
#include <knowrob/LogicProgramReasoner.h>
#include <knowrob/prolog/PrologQuery.h>

namespace knowrob {
	/**
	 * A pool of threads with attached Prolog engines.
	 * Prolog threads have their own stacks and only share the Prolog heap:
	 * predicates, records, flags and other global non-backtrackable data.
	 */
	class PrologThreadPool : public ThreadPool {
	public:
		/**
		 * @maxNumThreads maximum number of worker threads.
		 */
		explicit PrologThreadPool(uint32_t maxNumThreads=0);

	protected:
		// Override ThreadPool
		bool initializeWorker() override;
		
		// Override ThreadPool
		void finalizeWorker() override;
	};
	
	class PrologDataFile : public DataFile {
	public:
		static const std::string PROLOG_FORMAT;

		explicit PrologDataFile(const std::string &path);
	};

	/**
	 * A Prolog reasoner that answers queries using SWI Prolog.
	 */
	class PrologReasoner : public LogicProgramReasoner {
	public:
		/**
		 * @param reasonerID a knowledge base identifier of this reasoner.
		 */
		explicit PrologReasoner(std::string reasonerID);
		
		~PrologReasoner() override;

		/**
		 * Cannot be copy-assigned.
		 */
		PrologReasoner(const PrologReasoner&) = delete;

		/**
		 *
		 * @param key
		 * @param valueString
		 * @return
		 */
		bool setReasonerSetting(const TermPtr &key, const TermPtr &valueString);

		/**
		 * Consults a Prolog file, i.e. loads facts and rules and executed
		 * directives in the file.
		 * May throw an exception if there is no valid Prolog file at the given path.
		 * @prologFile the local path to the file.
         * @return true on success
		 */
		bool consult(const std::filesystem::path &prologFile, const char *contextModule={}, bool doTransformQuery=true);

		/**
		 * @param rdfFile a rdf-xml encoded file.
		 */
		bool load_rdf_xml(const std::filesystem::path &rdfFile);
		
		/**
		 * Evaluates a query and returns one solution if any.
		 * @param goal
		 * @param contextModule
		 * @param doTransformQuery
		 * @return the first solution found, or QueryResultStream::eos() if none.
		 */
		QueryResultPtr oneSolution(const std::shared_ptr<const Query> &goal,
								   const char *contextModule={},
								   bool doTransformQuery=true);

		/**
		 *
		 * @param goal
		 * @param contextModule
		 * @param doTransformQuery
		 * @return
		 */
		QueryResultPtr oneSolution(const std::shared_ptr<Predicate> &goal,
								   const char *contextModule={},
								   bool doTransformQuery=true);
		
		/**
		 * Evaluates a query and returns all solutions.
		 * @param goal
		 * @param contextModule
		 * @param doTransformQuery
		 * @return list of solutions.
		 */
		std::list<QueryResultPtr> allSolutions(const std::shared_ptr<const Query> &goal,
											   const char *contextModule={},
											   bool doTransformQuery=true);

		/**
		 *
		 * @param goal
		 * @param contextModule
		 * @param doTransformQuery
		 * @return
		 */
		std::list<QueryResultPtr> allSolutions(const std::shared_ptr<Predicate> &goal,
											   const char *contextModule={},
											   bool doTransformQuery=true);

		/**
		 *
		 * @param goal
		 * @param contextModule
		 * @param doTransformQuery
		 * @return
		 */
		bool eval(const std::shared_ptr<Predicate> &goal,
				  const char *contextModule={},
				  bool doTransformQuery=true);
		
		/**
		 * Parse a string into a term.
		 */
		TermPtr readTerm(const std::string &queryString);

		/**
		 * Run unittests associated to the given target name.
		 * The target name can be the name of a loaded testcase,
		 * or the path to a "*.pl", "*.plt" file, or the path to
		 * a directory containing such files.
		 * @param target a plunit target
		 * @return true on success
		 */
		std::list<TermPtr> runTests(const std::string &target);
		
		/**
		 */
		virtual const functor_t& callFunctor();

		/**
		 * Resolve the path to a Prolog file.
		 * The function attempts to resolve project-relative paths.
		 * @param filename a name or path.
		 * @return a path where the file might be stored
		 */
		static std::filesystem::path getPrologPath(const std::filesystem::path &filename);

		/**
		 * Resolve the path to a resource file.
		 * The function attempts to resolve project-relative paths.
		 * @param filename a name or path.
		 * @return a path where the file might be stored
		 */
		static std::filesystem::path getResourcePath(const std::filesystem::path &filename);

		// Override LogicProgramReasoner
		bool assertFact(const std::shared_ptr<Predicate> &predicate) override;

		// Override IReasoner
		bool loadConfiguration(const ReasonerConfiguration &cfg) override;

		// Override IReasoner
		std::shared_ptr<PredicateDescription> getPredicateDescription(
				const std::shared_ptr<PredicateIndicator> &indicator) override;

		// Override IReasoner
		unsigned long getCapabilities() const override;

		// Override IReasoner
		void startQuery(uint32_t queryID, const std::shared_ptr<const Query> &uninstantiatedQuery) override;

		// Override IReasoner
		void runQueryInstance(uint32_t queryID, const std::shared_ptr<QueryInstance> &queryInstance) override;
		
		// Override IReasoner
		void finishQuery(uint32_t queryID,
						 const std::shared_ptr<QueryResultStream::Channel> &outputStream,
						 bool isImmediateStopRequested) override;

	protected:
		static bool isInitialized_;
		const std::string reasonerID_;
		std::shared_ptr<StringTerm> reasonerIDTerm_;
		// cache of predicate descriptions
		std::map<PredicateIndicator, std::shared_ptr<PredicateDescription>> predicateDescriptions_;
		bool hasRDFData_;

		/** a query request for a runner */
		struct Request {
			const uint32_t queryID;
			const char *queryModule;
			std::shared_ptr<QueryInstance> queryInstance;
			functor_t callFunctor;
			const std::shared_ptr<const Query> goal;
			Request(const std::shared_ptr<QueryInstance> &queryInstance,
					functor_t callFunctor,
					const char *queryModule,
					uint32_t queryID=0)
			: queryID(queryID),
			  queryModule(queryModule),
			  queryInstance(queryInstance),
			  callFunctor(callFunctor),
			  goal(queryInstance->create()) {};
		};

		/** A runner that evaluates a Prolog query.  */
		class Runner : public ThreadPool::Runner {
		public:
			Runner(PrologReasoner *reasoner, Request request, bool sendEOS=false);

			// Override Runner
			void run() override;

		protected:
			PrologReasoner *reasoner_;
			Request request_;
			bool sendEOS_;

			term_t createQueryArgumentTerms(PrologQuery &pl_goal, term_t solutionScopeVar, term_t predicatesVar);
			term_t createContextTerm(term_t solutionScopeVar, term_t predicatesVar);

			static void setSolutionScope(std::shared_ptr<QueryResult> &solution, term_t solutionScope);

			template <class T> bool createIntervalDict(term_t intervalDict, const FuzzyInterval<T> &interval);
			template <class T> bool createRangeDict(term_t intervalDict, const Range<T> &range);

			std::list<std::shared_ptr<PrologReasoner::Runner>>::iterator requestIterator;
			friend class PrologReasoner;
		};
		
		struct ActiveQuery {
			std::shared_ptr<const Query> goal;
			std::atomic<bool> hasReceivedAllInput;
			std::list<std::shared_ptr<PrologReasoner::Runner>> runner;
			std::mutex mutex;
		};
		using ActiveQueryMap = std::map<uint32_t, PrologReasoner::ActiveQuery*>;
		
		ActiveQueryMap activeQueries_;
		std::mutex request_mutex_;
		
		void finishRunner(uint32_t queryID, PrologReasoner::Runner *runner);

		bool consult(const std::shared_ptr<FactBase> &factBase);
		bool consult(const std::shared_ptr<RuleBase> &ruleBase);

		void initializeProlog();
		bool initializeGlobalPackages();

		virtual bool initializeDefaultPackages() { return true; }

		bool loadDataFileWithUnknownFormat(const DataFilePtr &dataFile) override
		{ return consult(dataFile->path()); };
		
		PrologThreadPool& threadPool();
		
		friend class PrologReasoner::Runner;
	};

	/**
	 * A baseclass for prolog test fixtures.
	 */
	class PrologTestsBase: public testing::Test {
	protected:
		static void runPrologTests(const std::shared_ptr<knowrob::PrologReasoner> &reasoner,
								   const std::string &target);
	};

	template <class T> class PrologTests: public PrologTestsBase {
	protected:
		// Per-test-suite set-up.
		static void SetUpTestSuite() { reasoner();  }

		static void runTests(const std::string &t) { runPrologTests(reasoner(), t); }

		static std::shared_ptr<T> reasoner() {
			static std::shared_ptr<T> r;
			static int reasonerIndex_=0;
			if(!r) {
				std::stringstream ss;
				ss << "prolog" << reasonerIndex_++;
				r = std::make_shared<T>(ss.str());
				r->loadConfiguration(knowrob::ReasonerConfiguration());
			}
			return r;
		}
	};
}

#endif //KNOWROB_PROLOG_REASONER_H_
