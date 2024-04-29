/*
 * Copyright (c) 2023, Sascha Jongebloed
 * All rights reserved.
 *
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <unordered_map>
#include <boost/any.hpp>
#include "knowrob/integration/InterfaceUtils.h"
#include "knowrob/formulas/ModalFormula.h"
#include "knowrob/triples/FramedTriple.h"
#include "knowrob/queries/QueryTree.h"
#include "knowrob/queries/QueryError.h"
#include "knowrob/KnowledgeBase.h"


using namespace knowrob;

bool InterfaceUtils::assertStatements(KnowledgeBase kb_, const std::vector<FormulaPtr> &args) {
	std::vector<FramedTriplePtr> data(args.size());
	std::vector<FramedTriplePatternPtr> buf(args.size());
	uint32_t dataIndex = 0;

	for (auto &phi: args) {
		const QueryTree qt(phi);
		if (qt.numPaths() > 1) {
			throw QueryError("Disjunctions are not allowed in assertions. "
							 "Appears in statement {}.", *phi);
		} else if (qt.numPaths() == 0) {
			throw QueryError("Invalid assertion: '{}'", *phi);
		}
		for (auto &psi: qt.begin()->nodes()) {
			switch (psi->type()) {
				case knowrob::FormulaType::PREDICATE:
					buf[dataIndex] = std::make_shared<FramedTriplePattern>(
							std::static_pointer_cast<Predicate>(psi), false);
					data[dataIndex].ptr = new FramedTripleCopy();
					data[dataIndex].owned = true;
					buf[dataIndex]->instantiateInto(*data[dataIndex].ptr);
					dataIndex += 1;
					break;
				default:
					throw QueryError("Invalid assertion: '{}'", *phi);
			}
		}
	}
	if (kb_.insertAll(data)) {
		std::cout << "success, " << dataIndex << " statement(s) were asserted." << "\n";
		return true;
	} else {
		std::cout << "assertion failed." << "\n";
		return false;
	}
}

FormulaPtr
InterfaceUtils::applyModality(const std::unordered_map<std::string, boost::any> &options,
						   FormulaPtr phi) {
	FormulaPtr mFormula = std::move(phi);

	// Retrieve epistemicOperator and check if it is "BELIEF"
	auto epistemicOperator = boost::any_cast<std::string>(options.at("epistemicOperator"));
	if (epistemicOperator == "BELIEF") {
		// Retrieve aboutAgentIRI and confidence
		auto aboutAgentIRI = boost::any_cast<std::string>(options.at("aboutAgentIRI"));
		auto confidence = boost::any_cast<double>(options.at("confidence"));
		if (!aboutAgentIRI.empty()) {
			if (confidence != 1.0){
				mFormula = std::make_shared<ModalFormula>(
						modals::B(aboutAgentIRI, confidence), mFormula);
			} else {
				mFormula = std::make_shared<ModalFormula>(
						modals::B(aboutAgentIRI), mFormula);
			}
		}
	} else if (epistemicOperator == "KNOWLEDGE") {
		// Retrieve aboutAgentIRI
		auto aboutAgentIRI = boost::any_cast<std::string>(options.at("aboutAgentIRI"));
		if (!aboutAgentIRI.empty()) {
			mFormula = std::make_shared<ModalFormula>(
					modals::K(aboutAgentIRI),mFormula);
		}
	}
	// Retrieve temporalOperator
	auto temporalOperator = boost::any_cast<std::string>(options.at("temporalOperator"));

	// Retrieve minPastTimestamp and maxPastTimestamp
	auto minPastTimestamp = boost::any_cast<long long>(options.at("minPastTimestamp"));
	auto maxPastTimestamp = boost::any_cast<long long>(options.at("maxPastTimestamp"));

	auto minPastTimePoint = minPastTimestamp != -1 ? std::optional<TimePoint>(knowrob::time::fromSeconds(minPastTimestamp)) : std::nullopt;
	auto maxPastTimePoint = maxPastTimestamp != -1 ? std::optional<TimePoint>(knowrob::time::fromSeconds(maxPastTimestamp)) : std::nullopt;

	if (temporalOperator == "SOME_PAST") {
		if (minPastTimestamp != -1 || maxPastTimestamp != -1) {
			if (minPastTimestamp == -1) {
				mFormula = std::make_shared<ModalFormula>(
						modals::P(TimeInterval(std::nullopt,
											   maxPastTimePoint)),mFormula);
			} else if (maxPastTimestamp == -1) {
				mFormula = std::make_shared<ModalFormula>(
						modals::P(TimeInterval(minPastTimePoint,
											   std::nullopt)),mFormula);
			} else {
				mFormula = std::make_shared<ModalFormula>(
						modals::P(TimeInterval(minPastTimePoint,
											   maxPastTimePoint)),mFormula);
			}
		} else {
			mFormula = std::make_shared<ModalFormula>(
					modals::P(),mFormula);
		}
	} else if (temporalOperator == "ALL_PAST") {
		if (minPastTimestamp != -1 || maxPastTimestamp != -1) {
			if (minPastTimestamp == -1) {
				mFormula = std::make_shared<ModalFormula>(
						modals::H(TimeInterval(std::nullopt,
											   maxPastTimePoint)),mFormula);
			} else if (maxPastTimestamp == -1) {
				mFormula = std::make_shared<ModalFormula>(
						modals::H(TimeInterval(minPastTimePoint,
											   std::nullopt)),mFormula);
			} else {
				mFormula = std::make_shared<ModalFormula>(
						modals::H(TimeInterval(minPastTimePoint,
											   maxPastTimePoint)),mFormula);
			}
		} else {
			mFormula = std::make_shared<ModalFormula>(
					modals::H(),mFormula);
		}
	}
	return mFormula;
}