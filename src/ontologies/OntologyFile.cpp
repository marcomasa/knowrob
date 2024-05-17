/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <filesystem>
#include <utility>
#include "knowrob/ontologies/OntologyFile.h"
#include "knowrob/Logger.h"
#include "knowrob/ontologies/OntologyParser.h"

using namespace knowrob;

OntologyFile::OntologyFile(VocabularyPtr vocabulary, const URI &uri, std::string_view format)
		: OntologySource(uri, format),
		  vocabulary_(std::move(vocabulary)),
		  tripleFormat_(semweb::tripleFormatFromString(format)),
		  ontologyLanguage_(semweb::OntologyLanguage::OWL) {
}

bool OntologyFile::load(const TripleHandler &callback) {
	auto resolved = URI::resolve(uri());
	auto newVersion = DataSource::getVersionFromURI(resolved);

	// some OWL files are downloaded compile-time via CMake,
	// they are downloaded into owl/external e.g. there are SOMA.owl and DUL.owl.
	auto download_path = std::filesystem::path("owl") / "external" /
			 std::filesystem::path(resolved).filename();
	auto p = std::filesystem::path(URI::resolve(download_path.u8string()));
	const std::string *importURI;
	if (exists(p)) {
		KB_DEBUG("Using downloaded ontology at '{}'.", p.u8string());
		importURI = &p.native();
	} else {
		importURI = &resolved;
	}

	KB_INFO("Loading ontology at '{}' with version "
			"\"{}\" and origin \"{}\".", *importURI, newVersion, origin_);

	OntologyParser parser(*importURI, tripleFormat());
	parser.setOrigin(origin_);
	parser.setFrame(frame());
	// filter is called for each triple, if it returns false, the triple is skipped
	parser.setFilter([this](const FramedTriple &triple) {
		return !vocabulary_->isAnnotationProperty(triple.predicate());
	});
	// define a prefix for naming blank nodes
	parser.setBlankPrefix(std::string("_") + origin_);
	auto result = parser.run([&callback](const TripleContainerPtr &triples) {
		callback(triples);
	});
	if (!result) {
		KB_WARN("Failed to parse ontology {} ({})", *importURI, uri());
		return false;
	}
	// remember owl:imports
	setImports(parser.imports());

	return true;
}
