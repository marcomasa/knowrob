/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include "knowrob/alignment/GraphTransformation.h"
#include "knowrob/Logger.h"
#include "knowrob/alignment/GraphRenaming.h"
#include "knowrob/alignment/GraphRestructuring.h"

#define TRANSFORMATION_SETTING_KEY_TYPE "type"
#define TRANSFORMATION_SETTING_TYPE_RENAMING "renaming"
#define TRANSFORMATION_SETTING_TYPE_RESTRUCTURING "restructuring"
#define TRANSFORMATION_SETTING_TYPE_RULES "rules"

using namespace knowrob;

void GraphTransformation::pushOutputTriples(const TripleContainerPtr &triples) {
	if (nextTransformation_) {
		nextTransformation_->pushInputTriples(triples);
	} else if (next_) {
		next_(triples);
	} else {
		KB_WARN("No next transformation or handler set");
	}
}

void GraphTransformation::initializeNext() {
	if (nextTransformation_) {
		nextTransformation_->initializeTransformation();
	}
}

void GraphTransformation::finalizeNext() {
	if (nextTransformation_) {
		nextTransformation_->finalizeTransformation();
	}
}

void GraphTransformation::apply(OntologySource &ontologySource, const TripleHandler &callback) {
	// set the callback for the last transformation in the chain
	std::shared_ptr<GraphTransformation> last;
	GraphTransformation *current = this;
	while (current->nextTransformation_) {
		last = current->nextTransformation_;
		current = last.get();
	}
	if (last) {
		last->setNext(callback);
	} else {
		setNext(callback);
	}

	initializeTransformation();
	ontologySource.load([this](const TripleContainerPtr &triples) {
		pushInputTriples(triples);
	});
	finalizeTransformation();
}

std::shared_ptr<GraphTransformation> GraphTransformation::create(const boost::property_tree::ptree &config) {
	std::shared_ptr<GraphTransformation> last, first;

	// iterate over list of transformation options
	for (const auto &elem_pair: config) {
		auto &elem = elem_pair.second;

		auto type_opt = elem.get_optional<std::string>(TRANSFORMATION_SETTING_KEY_TYPE);
		if (!type_opt) {
			KB_ERROR("No \"{}\" key specified in graph transformation settings.", TRANSFORMATION_SETTING_KEY_TYPE);
			continue;
		}
		auto type_name = *type_opt;

		std::shared_ptr<GraphTransformation> next;
		if (type_name == TRANSFORMATION_SETTING_TYPE_RULES ||
			type_name == TRANSFORMATION_SETTING_TYPE_RESTRUCTURING) {
			next = std::make_shared<GraphRestructuring>();
		} else if (type_name == TRANSFORMATION_SETTING_TYPE_RENAMING) {
			next = std::make_shared<GraphRenaming>();
		} else {
			KB_ERROR("Unknown transformation type \"{}\"", type_name);
			continue;
		}
		if (!next->configure(config)) {
			KB_ERROR("Failed to configure transformation");
			continue;
		}
		if (!first) {
			first = next;
		} else {
			last->setNext(next);
		}
		last = next;
	}

	if (!first) {
		KB_ERROR("No transformations created");
		return nullptr;
	}
	return first;
}
