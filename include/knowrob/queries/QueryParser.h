/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_QUERY_PARSER_H
#define KNOWROB_QUERY_PARSER_H

#include "knowrob/formulas/Formula.h"
#include "knowrob/formulas/Predicate.h"
#include "knowrob/terms/Function.h"
#include "boost/any.hpp"

namespace knowrob {
	/**
	 * Constructs formulae from strings.
	 */
	class QueryParser {
	public:
		/**
		 * Parse a query string into a formula.
		 *
		 * @param queryString the query string.
		 * @return the parsed formula.
		 */
		static FormulaPtr parse(const std::string &queryString);

		/**
		 * Parse a query string into a predicate.
		 *
		 * @param queryString the query string.
		 * @return the parsed predicate.
		 */
		static PredicatePtr parsePredicate(const std::string &queryString);

		/**
		 * Parse a query string into a function.
		 *
		 * @param queryString the query string.
		 * @return the parsed function.
		 */
		static FunctionPtr parseFunction(const std::string &queryString);

		/**
		 * Parse a query string into a constant term.
		 *
		 * @param queryString the query string.
		 * @return the parsed constant term.
		 */
		static TermPtr parseConstant(const std::string &queryString);

		/**
		 * Parse a query string into a raw atom.
		 *
		 * @param queryString the query string.
		 * @return the parsed raw atom.
		 */
		static std::string parseRawAtom(const std::string &queryString);

		static FormulaPtr applyModality(const std::unordered_map<std::string, boost::any> &options, FormulaPtr phi);
	};

} // knowrob

#endif //KNOWROB_QUERY_PARSER_H
