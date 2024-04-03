/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <gtest/gtest.h>
#include "knowrob/Logger.h"
#include "knowrob/queries/QueryTree.h"
#include "knowrob/formulas/Top.h"

// fixture class for testing
namespace knowrob::testing {
	class QueryTreeTest : public ::testing::Test {
	protected:
		FormulaPtr p_, q_, r_;
		void SetUp() override {
			p_ = std::make_shared<Predicate>("p");
			q_ = std::make_shared<Predicate>("q");
			r_ = std::make_shared<Predicate>("r");
		}
		// void TearDown() override {}
	};
}
using namespace knowrob::testing;
using namespace knowrob;
using namespace knowrob::modals;

TEST_F(QueryTreeTest, PositiveLiteral)
{
    QueryTree qt(p_);
    EXPECT_EQ(qt.numPaths(), 1);
    if (qt.numPaths() > 0) {
        auto &path = qt.paths().front();
        EXPECT_EQ(*path.toFormula(), *p_);
        EXPECT_EQ(path.numNodes(), 1);
    }
}

TEST_F(QueryTreeTest, NegativeLiteral)
{
    QueryTree qt(~p_);
    EXPECT_EQ(qt.numPaths(), 1);
    if (qt.numPaths() > 0) {
        auto &path = qt.paths().front();
        EXPECT_EQ(*path.toFormula(), *(~p_));
        EXPECT_EQ(path.numNodes(), 1);
    }
}

TEST_F(QueryTreeTest, LiteralWithModality)
{
    QueryTree qt(K(p_));
    EXPECT_EQ(qt.numPaths(), 1);
    if (qt.numPaths() > 0) {
        auto &path = qt.paths().front();
        EXPECT_EQ(*path.toFormula(), *K(p_));
        EXPECT_EQ(path.numNodes(), 1);
    }
}

TEST_F(QueryTreeTest, NestedModality)
{
    QueryTree qt(K(B(~p_)));
    EXPECT_EQ(qt.numPaths(), 1);
    if (qt.numPaths() > 0) {
        auto &path = qt.paths().front();
        EXPECT_EQ(*path.toFormula(), *K(B(~p_)));
        EXPECT_EQ(path.numNodes(), 1);
    }
}

TEST_F(QueryTreeTest, Conjunction_pq)
{
    QueryTree qt(p_ & q_);
    EXPECT_EQ(qt.numPaths(), 1);
    if(qt.numPaths() > 0) {
        auto &path = qt.paths().front();
        EXPECT_EQ(path.numNodes(), 2);
        EXPECT_EQ(*path.toFormula(), *(p_ & q_));
    }
}

TEST_F(QueryTreeTest, Conjunction_pqr)
{
    QueryTree qt(~p_ & q_ & r_);
    EXPECT_EQ(qt.numPaths(), 1);
    if(qt.numPaths() > 0) {
        auto &path = qt.paths().front();
        EXPECT_EQ(path.numNodes(), 3);
        EXPECT_EQ(*path.toFormula(), *(~p_ & q_ & r_));
    }
}

TEST_F(QueryTreeTest, Disjunction_pq)
{
    QueryTree qt(p_ | q_);
    EXPECT_EQ(qt.numPaths(), 2);
    if(qt.numPaths() > 0) {
        auto &path = qt.paths().front();
        EXPECT_EQ(path.numNodes(), 1);
    }
}

TEST_F(QueryTreeTest, Disjunction_pqr)
{
    QueryTree qt(~p_ | q_ | r_);
    EXPECT_EQ(qt.numPaths(), 3);
    if(qt.numPaths()==3) {
        auto &path = qt.paths().front();
        EXPECT_EQ(path.numNodes(), 1);
    }
}

TEST_F(QueryTreeTest, AndOr)
{
    QueryTree qt((p_ | ~r_) & (q_ | r_));
    EXPECT_EQ(qt.numPaths(), 4);
    if(qt.numPaths()==4) {
        auto &path = qt.paths().front();
        EXPECT_EQ(path.numNodes(), 2);
    }
}

TEST_F(QueryTreeTest, ModalAndOr)
{
    QueryTree qt(P(p_ | ~r_) & P(q_ | r_));
    EXPECT_EQ(qt.numPaths(), 1);
}

TEST_F(QueryTreeTest, OrAnd)
{
    QueryTree qt((p_ & ~r_) | (q_ & r_));
    EXPECT_EQ(qt.numPaths(), 2);
    if(qt.numPaths() > 0) {
        auto &path = qt.paths().front();
        EXPECT_EQ(path.numNodes(), 2);
    }
}
