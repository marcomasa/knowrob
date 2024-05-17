/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include "knowrob/ontologies/TransformedOntology.h"

using namespace knowrob;

TransformedOntology::TransformedOntology(const URI &uri, std::string_view format)
		: OntologySource(uri, format) {
	storage_ = std::make_unique<RedlandModel>();
	storage_->setStorageType(RedlandStorageType::MEMORY);
	storage_->setOrigin(origin_);
}

bool TransformedOntology::load(const TripleHandler &callback) {
	storage_->batch(callback);
	return true;
}
