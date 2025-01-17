/*
 * Copyright (c) 2023, Daniel Beßler
 * All rights reserved.
 *
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <memory>

#include "knowrob/reasoner/prolog/PrologTests.h"
#include "knowrob/reasoner/esg/ESGReasoner.h"
#include "knowrob/reasoner/ReasonerManager.h"

using namespace knowrob;

// make reasoner type accessible
KNOWROB_BUILTIN_REASONER("ESG", ESGReasoner)

ESGReasoner::ESGReasoner(const std::string &reasonerID)
: PrologReasoner(reasonerID)
{
}

bool ESGReasoner::initializeDefaultPackages()
{
	return consult(std::filesystem::path("reasoner") / "esg" / "__init__.pl");
}

class ESGTests: public PrologTests<knowrob::ESGReasoner> {
protected:
	static std::string getPath(const std::string &filename)
	{ return std::filesystem::path("reasoner") / "esg" / filename; }
};

TEST_F(ESGTests, esg)      { runTests(getPath("esg.plt")); }
TEST_F(ESGTests, interval) { runTests(getPath("interval.plt")); }
TEST_F(ESGTests, workflow) { runTests(getPath("workflow.pl")); }
TEST_F(ESGTests, parser)   { runTests(getPath("parser.plt")); }
