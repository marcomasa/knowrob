/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include "knowrob/triples/GraphTerm.h"
#include "knowrob/integration/python/utils.h"
#include "knowrob/triples/GraphSequence.h"
#include "knowrob/triples/GraphUnion.h"
#include "knowrob/triples/GraphPattern.h"

using namespace knowrob;

namespace std {
	std::ostream &operator<<(std::ostream &os, const knowrob::GraphTerm &t) {
		t.write(os);
		return os;
	}
}

namespace knowrob::py {
	template<>
	void createType<GraphTerm>() {
		using namespace boost::python;

		enum_<GraphTermType>("GraphTermType")
		        .value("Sequence", GraphTermType::Sequence)
		        .value("Union", GraphTermType::Union)
		        .value("Pattern", GraphTermType::Pattern)
		        .value("Builtin", GraphTermType::Builtin);

		class_<GraphTerm, std::shared_ptr<GraphTerm>, boost::noncopyable>
		        ("GraphTerm", no_init)
		        .def("isPattern", &GraphTerm::isPattern)
		        .def("isBuiltin", &GraphTerm::isBuiltin)
		        .def("termType", &GraphTerm::termType);

		class_<GraphSequence, bases<GraphTerm>, std::shared_ptr<GraphSequence>, boost::noncopyable>
		        ("GraphSequence", init<>())
		        .def(init<const std::vector<std::shared_ptr<GraphTerm>> &>());

		class_<GraphUnion, bases<GraphTerm>, std::shared_ptr<GraphUnion>, boost::noncopyable>
		        ("GraphUnion", init<>())
		        .def(init<const std::vector<std::shared_ptr<GraphTerm>> &>());

		class_<GraphPattern, bases<GraphTerm>, std::shared_ptr<GraphPattern>, boost::noncopyable>
		        ("GraphPattern", init<FramedTriplePatternPtr>())
		        .def("value", &GraphPattern::value, return_value_policy<reference_existing_object>());
	}
}
