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
#include "knowrob/ThreadPool.h"
#include "knowrob/terms/Term.h"
#include "knowrob/reasoner/LogicProgramReasoner.h"
#include "knowrob/reasoner/DefinedReasoner.h"
#include "PrologQuery.h"
#include "PrologQueryRunner.h"
#include "PrologThreadPool.h"
#include "knowrob/semweb/ImportHierarchy.h"

namespace knowrob {
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
        AnswerPtr oneSolution(const std::shared_ptr<const Query> &goal,
                              const char *contextModule={},
                              bool doTransformQuery=true);

        /**
         *
         * @param goal
         * @param contextModule
         * @param doTransformQuery
         * @return
         */
        AnswerPtr oneSolution(const std::shared_ptr<Predicate> &goal,
                              const char *contextModule={},
                              bool doTransformQuery=true);

        /**
         * Evaluates a query and returns all solutions.
         * @param goal
         * @param contextModule
         * @param doTransformQuery
         * @return list of solutions.
         */
        std::list<AnswerPtr> allSolutions(const std::shared_ptr<const Query> &goal,
                                          const char *contextModule={},
                                          bool doTransformQuery=true);

        /**
         *
         * @param goal
         * @param contextModule
         * @param doTransformQuery
         * @return
         */
        std::list<AnswerPtr> allSolutions(const std::shared_ptr<Predicate> &goal,
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
        *
        * @param goal
        * @param contextModule
        * @param doTransformQuery
        * @return
        */
        bool eval(const std::shared_ptr<const Query> &q,
                  const char *moduleName,
                  bool doTransformQuery);

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

        static std::shared_ptr<DefinedReasoner> getDefinedReasoner(
                const term_t &t_reasonerManager, const term_t &t_reasonerModule);

        auto& importHierarchy() { return importHierarchy_; }

        // Override LogicProgramReasoner
        bool assertFact(const std::shared_ptr<Predicate> &predicate) override;

        // Override IReasoner
        bool loadConfiguration(const ReasonerConfiguration &cfg) override;

        // Override IReasoner
        void setDataBackend(const KnowledgeGraphPtr &knowledgeGraph) override;

        // Override IReasoner
        std::shared_ptr<PredicateDescription> getPredicateDescription(
                const std::shared_ptr<PredicateIndicator> &indicator) override;

        // Override IReasoner
        unsigned long getCapabilities() const override;

        // Override IReasoner
        AnswerBufferPtr submitQuery(const RDFLiteralPtr &literal, int queryFlags) override;

    protected:
        static bool isPrologInitialized_;
        static bool isKnowRobInitialized_;
        std::shared_ptr<semweb::ImportHierarchy> importHierarchy_;

        const std::string reasonerID_;
        std::shared_ptr<StringTerm> reasonerIDTerm_;
        // cache of predicate descriptions
        std::map<PredicateIndicator, std::shared_ptr<PredicateDescription>> predicateDescriptions_;

        std::mutex request_mutex_;

        static void initializeProlog();
        virtual bool initializeGlobalPackages();

        virtual bool initializeDefaultPackages() { return true; }

        bool loadDataSourceWithUnknownFormat(const DataSourcePtr &dataFile) override
        { return consult(dataFile->uri()); };

        static PrologThreadPool& threadPool();

        friend class PrologQueryRunner;

    };
}

#endif //KNOWROB_PROLOG_REASONER_H_
