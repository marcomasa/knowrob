/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <gtest/gtest.h>
#include "knowrob/terms/Unifier.h"
#include "knowrob/terms/Function.h"
#include "knowrob/terms/Numeric.h"

#define USE_OCCURS_CHECK

using namespace knowrob;

TEST(unifier, unify) {
	auto varX = std::make_shared<Variable>("X");
	auto varX_2 = std::make_shared<Variable>("X");
	auto varY = std::make_shared<Variable>("Y");

	auto x0 = std::make_shared<Function>("p", std::vector<TermPtr>{varX});
	auto x1 = std::make_shared<Function>("p", std::vector<TermPtr>{varY});
	auto x2 = std::make_shared<Function>("q", std::vector<TermPtr>{varX});
	auto x3 = std::make_shared<Function>("p", std::vector<TermPtr>{varX, varY});
	auto x4 = std::make_shared<Function>("p", std::vector<TermPtr>{
			std::make_shared<Function>("p", std::vector<TermPtr>{varX_2})});
	auto x5 = std::make_shared<Function>("p", std::vector<TermPtr>{
			std::make_shared<Long>(4)});

	// some positive examples:
	// - variable aliasing
	EXPECT_TRUE(Unifier(x0, x1).exists());
	// - instantiation of a variable to a constant
	EXPECT_TRUE(Unifier(x0, x5).exists());
#ifndef USE_OCCURS_CHECK
	// - instantiation of a variable to a term in which the variable occurs (without occurs check)
	EXPECT_TRUE(Unifier(x0,x4).exists());
	// for positive examples, the unifier can be applied to get an instance of the input terms
	EXPECT_EQ(*((Term*)x4.get()), *(Unifier(x0,x4).apply()));
#endif
	EXPECT_EQ(*((Term *) x5.get()), *(Unifier(x0, x5).apply()));

	// some negative examples
	// - functor missmatch
	EXPECT_FALSE(Unifier(x0, x2).exists());
	EXPECT_FALSE(Unifier(x1, x2).exists());
	// - arity missmatch
	EXPECT_FALSE(Unifier(x0, x3).exists());
	EXPECT_FALSE(Unifier(x1, x3).exists());
	EXPECT_FALSE(Unifier(x2, x3).exists());
#ifdef USE_OCCURS_CHECK
	// - occurs check
	EXPECT_FALSE(Unifier(x0, x4).exists());
#endif
}
