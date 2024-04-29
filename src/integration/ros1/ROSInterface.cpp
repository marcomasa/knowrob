/*
 * Copyright (c) 2023, Sascha Jongebloed
 * All rights reserved.
 *
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include "knowrob/integration/ros1/ROSInterface.h"
// KnowRob
#include "knowrob/knowrob.h"
#include "knowrob/Logger.h"
#include "knowrob/KnowledgeBase.h"
#include "knowrob/queries/QueryParser.h"
#include "knowrob/queries/QueryError.h"
#include "knowrob/formulas/ModalFormula.h"
#include "knowrob/terms/ListTerm.h"
#include "knowrob/queries/QueryTree.h"
// ROS
#include <ros/ros.h>
#include <ros/console.h>
#include <ros/package.h>
#include <knowrob/GraphAnswerMessage.h>
#include <knowrob/GraphQueryMessage.h>
#include <knowrob/KeyValuePair.h>
#include <knowrob/askallAction.h>
#include "knowrob/integration/InterfaceUtils.h"
#include <boost/property_tree/json_parser.hpp>
#include <utility>

using namespace knowrob;

ROSInterface::ROSInterface(const boost::property_tree::ptree& config)
        : askall_action_server_(nh_, "knowrob/askall", boost::bind(&ROSInterface::executeAskAllCB, this, _1), false),
          askone_action_server_(nh_, "knowrob/askone", boost::bind(&ROSInterface::executeAskOneCB, this, _1), false),
          askincremental_action_server_(nh_, "knowrob/askincremental", boost::bind(&ROSInterface::executeAskIncrementalCB, this, _1), false),
          tell_action_server_(nh_, "knowrob/tell", boost::bind(&ROSInterface::executeTellCB, this, _1), false),
          kb_(config)
{
    // Start all action servers
    askall_action_server_.start();
    askone_action_server_.start();
    askincremental_action_server_.start();
    tell_action_server_.start();
}

ROSInterface::~ROSInterface() = default;

// Function to convert GraphQueryMessage to std::unordered_map
std::unordered_map<std::string, boost::any> ROSInterface::translateGraphQueryMessage(const GraphQueryMessage& query) {
    std::unordered_map<std::string, boost::any> options;

    options["epistemicOperator"] = query.epistemicOperator;
    options["aboutAgentIRI"] = query.aboutAgentIRI;
    options["confidence"] = query.confidence;
    options["temporalOperator"] = query.temporalOperator;
    options["minPastTimestamp"] = query.minPastTimestamp;
    options["maxPastTimestamp"] = query.maxPastTimestamp;

    return options;
}

GraphAnswerMessage ROSInterface::createGraphAnswer(std::shared_ptr<const AnswerYes> answer) {
    const BindingsPtr &substitution = answer->substitution();
    GraphAnswerMessage graphAnswer;
    for (const auto& pair : *substitution) {
        KeyValuePair kvpair;
        kvpair.key = pair.first;
        TermPtr term = pair.second.second;
        // Stringstream for list terms
        std::stringstream ss;

		if (term->termType() == TermType::ATOMIC) {
			auto atomic = std::static_pointer_cast<Atomic>(term);
			switch (atomic->atomicType()) {
				case AtomicType::STRING:
				case AtomicType::ATOM:
					kvpair.type = KeyValuePair::TYPE_STRING;
					kvpair.value_string = atomic->stringForm().data();
					break;
				case AtomicType::NUMERIC: {
					auto numeric = std::static_pointer_cast<Numeric>(atomic);
					switch (numeric->xsdType()) {
						case XSDType::FLOAT:
						case XSDType::DOUBLE:
							kvpair.type = KeyValuePair::TYPE_FLOAT;
							kvpair.value_float = numeric->asDouble();
							break;
						case XSDType::NON_NEGATIVE_INTEGER:
						case XSDType::UNSIGNED_INT:
						case XSDType::INTEGER:
							kvpair.type = KeyValuePair::TYPE_INT;
							kvpair.value_int = numeric->asInteger();
							break;
						case XSDType::UNSIGNED_LONG:
						case XSDType::LONG:
							kvpair.type = KeyValuePair::TYPE_LONG;
							kvpair.value_long = numeric->asLong();
							break;
						case XSDType::UNSIGNED_SHORT:
						case XSDType::SHORT:
							kvpair.type = KeyValuePair::TYPE_INT;
							kvpair.value_int = numeric->asShort();
							break;
						case XSDType::BOOLEAN:
						case XSDType::STRING:
						case XSDType::LAST:
							break;
					}
					break;
				}
			}
		} else if (term->termType() == TermType::FUNCTION) {
			// TODO: Can this happen? If yes implement it
		} else if (term->termType() == TermType::VARIABLE) {
			// TODO: Can this happen? If yes implement it
		}

        graphAnswer.substitution.push_back(kvpair);
    }
    return graphAnswer;
}

void ROSInterface::executeAskAllCB(const askallGoalConstPtr& goal)
{

    // Implement your action here
    FormulaPtr phi(QueryParser::parse(goal->query.queryString));

    FormulaPtr mPhi = InterfaceUtils::applyModality(translateGraphQueryMessage(goal->query), phi);

	auto ctx = std::make_shared<QueryContext>(QUERY_FLAG_ALL_SOLUTIONS);
    auto resultStream = kb_.submitQuery(mPhi, ctx);
    auto resultQueue = resultStream->createQueue();

    int numSolutions_ = 0;
    askallResult result;
    while(true) {
        auto nextResult = resultQueue->pop_front();

        if(nextResult->indicatesEndOfEvaluation()) {
            break;
        }
		else if (nextResult->tokenType() == TokenType::ANSWER_TOKEN) {
			auto answer = std::static_pointer_cast<const Answer>(nextResult);
			if (answer->isPositive()) {
				auto positiveAnswer = std::static_pointer_cast<const AnswerYes>(answer);
				if (positiveAnswer->substitution()->empty()) {
					numSolutions_ = 1;
					break;
				} else {
					// Push one answer
					GraphAnswerMessage graphAns = createGraphAnswer(positiveAnswer);
					result.answer.push_back(graphAns);
					numSolutions_ += 1;
					// publish feedback
					askallFeedback feedback;
					feedback.numberOfSolutions = numSolutions_;
					askall_action_server_.publishFeedback(feedback);
				}
			}
        }
    }

    if(numSolutions_ == 0) {
        result.status = askallResult::FALSE;
    } else {
        result.status = askallResult::TRUE;
    }
    askall_action_server_.setSucceeded(result);
}

void ROSInterface::executeAskIncrementalCB(const askincrementalGoalConstPtr& goal)
{

    // Implement your action here
    FormulaPtr phi(QueryParser::parse(goal->query.queryString));

    FormulaPtr mPhi = InterfaceUtils::applyModality(translateGraphQueryMessage(goal->query), phi);

	auto ctx = std::make_shared<QueryContext>(QUERY_FLAG_ALL_SOLUTIONS);
	auto resultStream = kb_.submitQuery(mPhi, ctx);
    auto resultQueue = resultStream->createQueue();

    int numSolutions_ = 0;
    bool isTrue = false;
    askincrementalResult result;
    askincrementalFeedback feedback;
    while(true) {
        auto nextResult = resultQueue->pop_front();

        if(nextResult->indicatesEndOfEvaluation()) {
            break;
        }
        else if (nextResult->tokenType() == TokenType::ANSWER_TOKEN) {
			auto answer = std::static_pointer_cast<const Answer>(nextResult);
			if (answer->isPositive()) {
				auto positiveAnswer = std::static_pointer_cast<const AnswerYes>(answer);
				isTrue = true;
				if (positiveAnswer->substitution()->empty()) {
					break;
				} else {
					// Publish feedback
					feedback.answer = createGraphAnswer(positiveAnswer);
					numSolutions_ += 1;
					askincremental_action_server_.publishFeedback(feedback);
				}
			}
        }
    }

    if(isTrue) {
        result.status = askallResult::TRUE;
    } else {
        result.status = askallResult::FALSE;
    }
    result.numberOfSolutionsFound = numSolutions_;
    askincremental_action_server_.setSucceeded(result);
}

void ROSInterface::executeAskOneCB(const askoneGoalConstPtr& goal)
{
    FormulaPtr phi(QueryParser::parse(goal->query.queryString));

    FormulaPtr mPhi = InterfaceUtils::applyModality(translateGraphQueryMessage(goal->query), phi);

	auto ctx = std::make_shared<QueryContext>(QUERY_FLAG_ONE_SOLUTION);
	auto resultStream = kb_.submitQuery(mPhi, ctx);
    auto resultQueue = resultStream->createQueue();

    askoneResult result;
    auto nextResult = resultQueue->pop_front();

    if(nextResult->indicatesEndOfEvaluation()) {
        result.status = askoneResult::FALSE;
    } else if (nextResult->tokenType() == TokenType::ANSWER_TOKEN) {
		auto answer = std::static_pointer_cast<const Answer>(nextResult);
		if (answer->isPositive()) {
			auto positiveAnswer = std::static_pointer_cast<const AnswerYes>(answer);
			result.status = askoneResult::TRUE;
			if (positiveAnswer->substitution()->empty()) {
				GraphAnswerMessage answer = createGraphAnswer(positiveAnswer);
				result.answer = answer;
			}
		}
    }

    askoneFeedback  feedback;
    feedback.finished = true;
    askone_action_server_.publishFeedback(feedback);
    askone_action_server_.setSucceeded(result);
}

void ROSInterface::executeTellCB(const tellGoalConstPtr &goal) {
    FormulaPtr phi(QueryParser::parse(goal->query.queryString));

    FormulaPtr mPhi = InterfaceUtils::applyModality(translateGraphQueryMessage(goal->query), phi);

    bool success = InterfaceUtils::assertStatements(kb_, {mPhi});

    tellResult result;
    tellFeedback feedback;
    if(success) {
        result.status = tellResult::TRUE;
    }
    else {
        result.status = tellResult::TELL_FAILED;
    }
    feedback.finished = true;
    tell_action_server_.publishFeedback(feedback);
    tell_action_server_.setSucceeded(result);
}

boost::property_tree::ptree loadSetting() {
    // Check for settings file
    std::string config_path = "default.json";
    if(std::getenv("KNOWROB_SETTINGS")) {
        config_path = std::getenv("KNOWROB_SETTINGS");
    }

    // read the settings
    boost::property_tree::ptree config;
    boost::property_tree::read_json(
            config_path,
            config);

    return config;

}

int main(int argc, char **argv) {
    InitKnowledgeBase(argc, argv);

    // Load settings files
    try {
        ros::init(argc, argv, "knowrob_node");
        ROSInterface ros_interface(loadSetting());
        ros::spin();
    }
    catch(std::exception& e) {
        KB_ERROR("an exception occurred: {}.", e.what());
        return EXIT_FAILURE;
    }
}