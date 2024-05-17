/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include "knowrob/ontologies/SPARQLService.h"
#include "knowrob/storage/StorageError.h"

using namespace knowrob;

SPARQLService::SPARQLService(const URI &uri, semweb::TripleFormat format)
		: SPARQLService::SPARQLService(uri, semweb::tripleFormatToString(format)) {
}

SPARQLService::SPARQLService(const URI &uri, std::string_view format)
		: OntologySource(uri, format) {
	// create a Redland model for the SPARQL endpoint
	// Note that the model has interfaces for insert and remove triples, these won't
	// interact with the SPARQL endpoint, but with the local storage of the model.
	model_.setStorageType(RedlandStorageType::MEMORY);
	model_.setOrigin(origin_);
	if (!model_.initializeBackend()) {
		throw StorageError("Failed to initialize Redland backend for SPARQL endpoint at \"{}\".", uri());
	}
	// set up the model to interface with the SPARQL endpoint
	if (!model_.load(uri, semweb::tripleFormatFromString(format))) {
		throw StorageError("Failed to load URI of SPARQL endpoint at \"{}\".", uri());
	}
}

bool SPARQLService::load(const TripleHandler &callback) {
	// iterate over all triples in the SPARQL endpoint
	model_.batch(callback);
	return true;
}
