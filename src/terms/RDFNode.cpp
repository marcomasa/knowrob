/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <boost/algorithm/string/predicate.hpp>
#include "knowrob/terms/RDFNode.h"

namespace knowrob {
	RDFNodeType rdfNodeTypeGuess(std::string_view str) {
		if (str.empty()) return RDFNodeType::LITERAL;
		if (str[0] == '_') return RDFNodeType::BLANK;
		if (str[0] == '<') return RDFNodeType::IRI;
		if (boost::algorithm::starts_with(str, "http://") ||
		    boost::algorithm::starts_with(str, "https://")) return RDFNodeType::IRI;
		return RDFNodeType::LITERAL;
	}
}