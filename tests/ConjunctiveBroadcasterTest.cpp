/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <gtest/gtest.h>
#include <knowrob/queries/ConjunctiveBroadcaster.h>
#include "knowrob/queries/TokenQueue.h"
#include "knowrob/queries/QueryParser.h"
#include "knowrob/queries/AnswerYes.h"
#include "knowrob/queries/Answer.h"
#include "knowrob/terms/Numeric.h"

using namespace knowrob;

// fixture class for testing
namespace knowrob::testing {
	class ConjunctiveBroadcasterTest : public ::testing::Test {
	protected:
		void SetUp() override {}

		void TearDown() override {}
	};
}
using namespace knowrob::testing;

TEST_F(ConjunctiveBroadcasterTest, CombineOne) {
	auto combiner = std::make_shared<ConjunctiveBroadcaster>();
	// feed broadcast into queue
	auto output = std::make_shared<TokenQueue>();
	combiner->addSubscriber(TokenStream::Channel::create(output));
	// create channels for broadcast
	auto input1 = TokenStream::Channel::create(combiner);
	// push a message
	EXPECT_EQ(output->size(), 0);
	input1->push(GenericYes());
	EXPECT_EQ(output->size(), 1);
	// push more messages
	input1->push(GenericYes());
	input1->push(GenericYes());
	EXPECT_EQ(output->size(), 3);
}

TEST_F(ConjunctiveBroadcasterTest, CombineMany_DifferentVariables) {
	auto combiner = std::make_shared<ConjunctiveBroadcaster>();
	// feed broadcast into queue
	auto output = std::make_shared<TokenQueue>();
	combiner->addSubscriber(TokenStream::Channel::create(output));
	// create channels for broadcast
	auto input1 = TokenStream::Channel::create(combiner);
	auto input2 = TokenStream::Channel::create(combiner);
	// construct partial answers
	auto answer11 = std::make_shared<AnswerYes>(std::make_shared<Bindings>(Bindings::VarMap{
		{std::make_shared<Variable>("a"), std::make_shared<Integer>(4)}
	}));
	auto answer21 = std::make_shared<AnswerYes>(std::make_shared<Bindings>(Bindings::VarMap{
		{std::make_shared<Variable>("b"), std::make_shared<Integer>(6)}
	}));
	auto answer22 = std::make_shared<AnswerYes>(std::make_shared<Bindings>(Bindings::VarMap{
		{std::make_shared<Variable>("b"), std::make_shared<Integer>(7)}
	}));
	// push a partial answer via input1
	EXPECT_EQ(output->size(), 0);
	input1->push(answer11);
	EXPECT_EQ(output->size(), 0);
	// push a partial answer via input2
	input2->push(answer21);
	EXPECT_EQ(output->size(), 1);
	input2->push(answer22);
	EXPECT_EQ(output->size(), 2);
}

TEST_F(ConjunctiveBroadcasterTest, CombineMany_Unification) {
	auto combiner = std::make_shared<ConjunctiveBroadcaster>(false);
	// feed broadcast into queue
	auto output = std::make_shared<TokenQueue>();
	combiner->addSubscriber(TokenStream::Channel::create(output));
	// create channels for broadcast
	auto input1 = TokenStream::Channel::create(combiner);
	auto input2 = TokenStream::Channel::create(combiner);
	// construct partial answers
	auto answer11 = std::make_shared<AnswerYes>(std::make_shared<Bindings>(Bindings::VarMap{
		{std::make_shared<Variable>("a"), QueryParser::parseFunction("p(X,1)")}
	}));
	auto answer21 = std::make_shared<AnswerYes>(std::make_shared<Bindings>(Bindings::VarMap{
		{std::make_shared<Variable>("a"), QueryParser::parseFunction("p(2,Y)")}
	}));
	auto answer22 = std::make_shared<AnswerYes>(std::make_shared<Bindings>(Bindings::VarMap{
		{std::make_shared<Variable>("a"), QueryParser::parseFunction("p(2,2)")}
	}));
	// push "a=p(X,1)" and "a=p(2,Y)" into combiner via two channels
	input1->push(answer11);
	input2->push(answer21);
	// expect that the combiner has one output "a=p(2,1)" unifying both inputs.
	EXPECT_EQ(output->size(), 1);
	if (output->size() == 1) {
		auto combinedResult = output->front();
		EXPECT_EQ(combinedResult->tokenType(), TokenType::ANSWER_TOKEN);
		if (combinedResult->tokenType() == TokenType::ANSWER_TOKEN) {
			auto answer = std::static_pointer_cast<const Answer>(combinedResult);
			EXPECT_TRUE(answer->isPositive());
			EXPECT_FALSE(answer->isNegative());
			EXPECT_FALSE(answer->isUncertain());
			if (answer->isPositive()) {
				auto positiveAnswer = std::static_pointer_cast<const AnswerYes>(answer);
				EXPECT_TRUE(positiveAnswer->hasGrounding(Variable("a")));
				auto instantiation = positiveAnswer->substitution()->get("a");
				EXPECT_EQ(*instantiation, *QueryParser::parseFunction("p(2,1)"));
			}
		}
	}
	// "a=p(X,1)" and "a=p(2,2)" cannot be combined, number of outputs stays at 1
	input2->push(answer22);
	EXPECT_EQ(output->size(), 1);
}
