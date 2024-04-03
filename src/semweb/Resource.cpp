/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include "knowrob/semweb/Resource.h"
#include "knowrob/terms/IRIAtom.h"
#include "knowrob/terms/Blank.h"
#include "knowrob/Logger.h"
#include "knowrob/knowrob.h"
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <iomanip>
#include "knowrob/integration/python/utils.h"

using namespace knowrob::semweb;

Resource::Resource(std::string_view iri) {
	switch (rdfNodeTypeGuess(iri)) {
		case RDFNodeType::BLANK:
			iri_ = Blank::Tabled(iri);
			break;
		case RDFNodeType::IRI:
			iri_ = IRIAtom::Tabled(iri);
			break;
		case RDFNodeType::LITERAL:
			KB_WARN("Resource created with guessed literal type: {}. Treating as IRI.", iri);
			iri_ = IRIAtom::Tabled(iri);
			break;
	}
}

knowrob::IRIAtomPtr Resource::unique_iri(std::string_view ns, std::string_view name) {
	std::stringstream ss;
	ss << ns;
	if (!ns.empty() && ns.back() != '#') {
		ss << "#";
	}
	ss << name << '_';
	insertUnique(ss);
	return IRIAtom::Tabled(ss.str());
}

knowrob::IRIAtomPtr Resource::unique_iri(std::string_view type_iri) {
	std::stringstream ss;
	ss << type_iri;
	ss << '_';
	insertUnique(ss);
	return IRIAtom::Tabled(ss.str());
}

std::string_view Resource::iri_name(std::string_view iri) {
	auto pos = iri.find('#');
	if (pos != std::string::npos) {
		return iri.substr(pos + 1);
	}
	return {iri.data()};
}

std::string_view Resource::name() const {
	return iri_name(iri_->stringForm());
}

std::string_view Resource::iri_ns(std::string_view iri, bool includeDelimiter) {
	auto pos = iri.rfind('#');
	if (pos != std::string::npos) {
		auto pos_x = (includeDelimiter ? pos + 1 : pos);
		return iri.substr(0, pos_x);
	}
	return {};
}

std::string_view Resource::ns(bool includeDelimiter) const {
	return iri_ns(iri_->stringForm(), includeDelimiter);
}

namespace knowrob::py {
	template<>
	void createType<semweb::Resource>() {
		using namespace boost::python;

		using IRIArg1 = IRIAtomPtr (*)(std::string_view);
		using IRIArg2 = IRIAtomPtr (*)(std::string_view,std::string_view);

		class_<semweb::Resource, std::shared_ptr<semweb::Resource>, boost::noncopyable>
				("Resource", no_init)
				.def("iri", &semweb::Resource::iri)
				.def("iriAtom", &semweb::Resource::iriAtom)
				.def("name", &semweb::Resource::name)
				.def("ns", &semweb::Resource::ns)
				.def("unique_iri", static_cast<IRIArg1>(&semweb::Resource::unique_iri))
				.def("unique_iri", static_cast<IRIArg2>(&semweb::Resource::unique_iri))
				.def("iri_name", &semweb::Resource::iri_name)
				.def("iri_ns", &semweb::Resource::iri_ns);
	}
}
