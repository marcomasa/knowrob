/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <gtest/gtest.h>
#include "knowrob/formulas/DependencyGraph.h"
#include "knowrob/formulas/ModalOperator.h"
#include "knowrob/queries/QueryParser.h"
#include "knowrob/formulas/ModalFormula.h"

using namespace knowrob;

// fixture class for testing
namespace knowrob::testing {
	class DependencyGraphTest : public ::testing::Test {
	protected:
		FirstOrderLiteralPtr p_, q_, r_, s_;

		void SetUp() override {
			p_ = std::make_shared<FirstOrderLiteral>(QueryParser::parsePredicate("p(a,X)"), false);
			q_ = std::make_shared<FirstOrderLiteral>(QueryParser::parsePredicate("q(X,Y)"), false);
			r_ = std::make_shared<FirstOrderLiteral>(QueryParser::parsePredicate("r(Y,z)"), false);
			s_ = std::make_shared<FirstOrderLiteral>(QueryParser::parsePredicate("s(b,a)"), false);

			auto x = std::make_shared<ModalIteration>();
			*x += modals::K();
		}

		void TearDown() override {}
	};
}
using namespace knowrob::testing;

TEST_F(DependencyGraphTest, SingleLiteral)
{
    DependencyGraph dg;
    dg.insert({p_});
    ASSERT_EQ(dg.numNodes(),1);
    ASSERT_EQ(dg.numGroups(),1);
}

TEST_F(DependencyGraphTest, DependantLiterals)
{
    DependencyGraph dg;
    dg.insert({p_, q_});
    ASSERT_EQ(dg.numNodes(),2);
    ASSERT_EQ(dg.numGroups(),1);
}

TEST_F(DependencyGraphTest, IndependantLiterals)
{
    DependencyGraph dg;
    dg.insert({p_, r_});
    ASSERT_EQ(dg.numNodes(),2);
    ASSERT_EQ(dg.numGroups(),2);
}

TEST_F(DependencyGraphTest, ChainAndOne)
{
    DependencyGraph dg;
    dg.insert({p_, q_, r_});
    ASSERT_EQ(dg.numNodes(),3);
    ASSERT_EQ(dg.numGroups(),1);
}

TEST_F(DependencyGraphTest, MultiModalDependant)
{
    DependencyGraph dg;
    dg.insert({p_, r_, s_});
    dg.insert({q_});
    ASSERT_EQ(dg.numNodes(),4);
    ASSERT_EQ(dg.numGroups(),2);
}

TEST_F(DependencyGraphTest, MultiModalIndependant)
{
    DependencyGraph dg;
    dg.insert({p_, r_, q_});
    dg.insert({s_});
    ASSERT_EQ(dg.numNodes(),4);
    ASSERT_EQ(dg.numGroups(),2);
}
