/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <queue>
#include <set>
#include "knowrob/semweb/Property.h"
#include "knowrob/Logger.h"
#include "knowrob/integration/python/utils.h"

using namespace knowrob::semweb;

Property::Property(std::string_view iri)
		: Resource(iri), reification_(std::make_shared<Class>(reifiedIRI(iri))), flags_(0) {}

Property::Property(const IRIAtomPtr &iri)
		: Resource(iri), reification_(std::make_shared<Class>(reifiedIRI(iri->stringForm()))), flags_(0) {}

bool Property::Comparator::operator()(const std::shared_ptr<Property> &lhs, const std::shared_ptr<Property> &rhs) const {
	return lhs->iri() < rhs->iri();
}

knowrob::IRIAtomPtr Property::reifiedIRI(std::string_view iri) {
	// split the IRI at the last '#' and insert 'Reified' in between
	auto pos = iri.rfind('#');
	char delimiter = '#';
	if (pos == std::string::npos) {
		pos = iri.rfind('/');
		delimiter = '/';
	}
	std::stringstream ss;
	if (pos == std::string::npos) {
		ss << "Reified_" << iri;
		return IRIAtom::Tabled(ss.str());
	}
	ss << iri.substr(0, pos) << delimiter << "Reified_" << iri.substr(pos+1);
	return IRIAtom::Tabled(ss.str());
}

knowrob::IRIAtomPtr Property::unReifiedIRI(std::string_view iri) {
	// split the IRI at the last '#' and remove 'Reified' in between
	auto pos = iri.rfind('#');
	char delimiter = '#';
	if (pos == std::string::npos) {
		pos = iri.rfind('/');
		delimiter = '/';
	}
	if (pos == std::string::npos) {
		return IRIAtom::Tabled(iri);
	}
	auto reified = iri.substr(pos+1);
	if (reified.find("Reified_") != 0) {
		return IRIAtom::Tabled(iri);
	}
	std::stringstream ss;
	ss << iri.substr(0, pos) << delimiter << reified.substr(8);
	return IRIAtom::Tabled(ss.str());
}

void Property::addDirectParent(const std::shared_ptr<Property> &directParent) {
	directParents_.insert(directParent);
	reification_->addDirectParent(directParent->reification_);
}

void Property::removeDirectParent(const std::shared_ptr<Property> &directParent) {
	directParents_.erase(directParent);
	reification_->removeDirectParent(directParent->reification_);
}

void Property::setInverse(const std::shared_ptr<Property> &inverse) {
	inverse_ = inverse;
}

bool Property::hasFlag(PropertyFlag flag) const {
	return flags_ & flag;
}

void Property::setFlag(PropertyFlag flag) {
	flags_ |= flag;
}

void Property::forallParents(const PropertyVisitor &visitor,
							 bool includeSelf,
							 bool skipDuplicates) {
	std::queue<Property *> queue_;
	std::set<std::string_view> visited_;

	// push initial elements to the queue
	if (includeSelf) queue_.push(this);
	else for (auto &x: directParents_) queue_.push(x.get());

	// visit each parent
	while (!queue_.empty()) {
		auto front = queue_.front();
		queue_.pop();
		// visit popped property
		visitor(*front);
		// remember visited nodes
		if (skipDuplicates) visited_.insert(front->iri());
		// push parents of visited property on the queue
		for (auto &directParent: front->directParents_) {
			if (skipDuplicates && visited_.count(directParent->iri()) > 0) continue;
			queue_.push(directParent.get());
		}
	}
}

namespace knowrob::py {
	template<>
	void createType<semweb::Property>() {
		using namespace boost::python;

		enum_<PropertyFlag>("PropertyFlag")
				.value("DATATYPE_PROPERTY", DATATYPE_PROPERTY)
				.value("ANNOTATION_PROPERTY", ANNOTATION_PROPERTY)
				.value("OBJECT_PROPERTY", OBJECT_PROPERTY)
				.value("TRANSITIVE_PROPERTY", TRANSITIVE_PROPERTY)
				.value("REFLEXIVE_PROPERTY", REFLEXIVE_PROPERTY)
				.value("SYMMETRIC_PROPERTY", SYMMETRIC_PROPERTY)
				.export_values();

		class_<semweb::Property, bases<semweb::Resource>, std::shared_ptr<semweb::Property>, boost::noncopyable>
				("Property", init<std::string_view>())
				.def(init<const IRIAtomPtr&>())
				.def("addDirectParent", &semweb::Property::addDirectParent)
				.def("removeDirectParent", &semweb::Property::removeDirectParent)
				.def("directParents", &semweb::Property::directParents, return_value_policy<reference_existing_object>())
				.def("setInverse", &semweb::Property::setInverse)
				.def("inverse", &semweb::Property::inverse, return_value_policy<reference_existing_object>())
				.def("hasFlag", &semweb::Property::hasFlag)
				.def("setFlag", &semweb::Property::setFlag)
				.def("isDatatypeProperty", &semweb::Property::isDatatypeProperty)
				.def("isAnnotationProperty", &semweb::Property::isAnnotationProperty)
				.def("isObjectProperty", &semweb::Property::isObjectProperty)
				.def("isTransitiveProperty", &semweb::Property::isTransitiveProperty)
				.def("isReflexiveProperty", &semweb::Property::isReflexiveProperty)
				.def("isSymmetricProperty", &semweb::Property::isSymmetricProperty)
				.def("forallParents", &semweb::Property::forallParents)
				.def("reification", &semweb::Property::reification)
				.def("reifiedIRI", &semweb::Property::reifiedIRI)
				.def("unReifiedIRI", &semweb::Property::unReifiedIRI);
	}
}
