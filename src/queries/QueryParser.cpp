/*// Define value types for options
using OptionValue = variant<string, double, int, optional<long long>>;
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <boost/spirit/include/qi.hpp>
#include "knowrob/queries/QueryParser.h"
#include "knowrob/queries/QueryError.h"
#include "knowrob/queries/parsers/strings.h"
#include "knowrob/queries/parsers/terms.h"
#include "knowrob/queries/parsers/formula.h"
#include "knowrob/integration/python/utils.h"
#include "knowrob/formulas/ModalFormula.h"

using namespace knowrob;

template<typename ResultType, typename RuleType>
static inline ResultType parse_(const std::string &queryString, const RuleType &rule) {
	auto first = queryString.begin();
	auto last = queryString.end();

	ResultType result;
	bool r = boost::spirit::qi::phrase_parse(first, last, rule, boost::spirit::ascii::space, result);

	if (first == last && r) {
		return result;
	} else {
		throw QueryError("Query string ({}) has invalid syntax.", queryString);
	}
}

FormulaPtr QueryParser::parse(const std::string &queryString) {
	return parse_<FormulaPtr, knowrob::parsers::formula::FormulaRule>(queryString, knowrob::parsers::formula::formula());
}

PredicatePtr QueryParser::parsePredicate(const std::string &queryString) {
	return parse_<PredicatePtr, knowrob::parsers::formula::PredicateRule>(queryString, knowrob::parsers::formula::predicate());
}

FunctionPtr QueryParser::parseFunction(const std::string &queryString) {
	return parse_<FunctionPtr, knowrob::parsers::terms::FunctionRule>(queryString, knowrob::parsers::terms::function());
}

TermPtr QueryParser::parseConstant(const std::string &queryString) {
	return parse_<TermPtr, knowrob::parsers::terms::TermRule>(queryString, knowrob::parsers::terms::atomic());
}

std::string QueryParser::parseRawAtom(const std::string &queryString) {
	return parse_<std::string, knowrob::parsers::str::StringRule>(queryString, knowrob::parsers::str::atom_or_iri());
}

FormulaPtr
QueryParser::applyModality(const std::unordered_map<std::string, boost::any> &options,
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

namespace knowrob::py {
	template<>
	void createType<QueryParser>() {
		using namespace boost::python;
		class_<QueryParser, boost::noncopyable>
				("QueryParser", no_init)
				.def("parse", &QueryParser::parse).staticmethod("parse")
				.def("parsePredicate", &QueryParser::parsePredicate).staticmethod("parsePredicate")
				.def("parseFunction", &QueryParser::parseFunction).staticmethod("parseFunction")
				.def("parseConstant", &QueryParser::parseConstant).staticmethod("parseConstant")
				.def("parseRawAtom", &QueryParser::parseRawAtom).staticmethod("parseRawAtom");
	}
}
