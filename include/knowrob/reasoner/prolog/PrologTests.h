/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_PROLOG_TESTS_H_
#define KNOWROB_PROLOG_TESTS_H_

#include <gtest/gtest.h>

#include "PrologReasoner.h"
#include "knowrob/reasoner/ReasonerManager.h"
#include "knowrob/Logger.h"
#include "knowrob/knowrob.h"

namespace knowrob {
	/**
	 * A baseclass for prolog test fixtures.
	 */
	class PrologTestsBase : public testing::Test {
	protected:
		static void runPrologTests(
				const std::shared_ptr<knowrob::PrologReasoner> &reasoner,
				const std::string &target);
	};

	template<class ReasonerType, class BackendType>
	class PrologTests : public PrologTestsBase {
	protected:
		static std::shared_ptr<BackendType> createBackend(const std::string &name, const std::shared_ptr<KnowledgeBase> &kb) {
			auto db = std::make_shared<BackendType>();
			kb->backendManager()->addPlugin(name, db);
			db->initializeBackend(knowrob::PropertyTree());
			return db;
		}

		static std::shared_ptr<ReasonerType> createReasoner(const std::string &name, const std::shared_ptr<KnowledgeBase> &kb, const std::shared_ptr<BackendType> &db) {
			auto r = std::make_shared<ReasonerType>();
			kb->reasonerManager()->addPlugin(name, r);
			r->setDataBackend(db);
			r->initializeReasoner(knowrob::PropertyTree());
			return r;
		}

		// Per-test-suite set-up.
		static void SetUpTestSuite() {
			// Initialize the reasoner
			try {
				reasoner();
			} catch (std::exception &e) {
				FAIL() << "SetUpTestSuite failed: " << e.what();
			}
		}

		static void runTests(const std::string &t) {
			try {
				runPrologTests(reasoner(), t);
			} catch (std::exception &e) {
				FAIL() << "runTests failed: " << e.what();
			}
		}

		static std::shared_ptr<ReasonerType> reasoner() {
			static std::shared_ptr<ReasonerType> reasoner;
			static std::shared_ptr<KnowledgeBase> kb;
			static std::shared_ptr<BackendType> db;

			if(!reasoner) {
				std::stringstream ss;
				ss << "prolog_";
				insertUnique(ss);

				kb = std::make_shared<KnowledgeBase>();
				db = createBackend(ss.str(), kb);
				reasoner = createReasoner(ss.str(), kb, db);

				kb->loadCommon();
				kb->init();
			}
			return reasoner;
		}
	};
}

#endif //KNOWROB_PROLOG_TESTS_H_
