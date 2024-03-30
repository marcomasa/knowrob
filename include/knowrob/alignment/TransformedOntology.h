/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_TRANSFORMED_ONTOLOGY_H
#define KNOWROB_TRANSFORMED_ONTOLOGY_H

#include "knowrob/sources/OntologySource.h"
#include "knowrob/backend/redland/RedlandModel.h"

namespace knowrob {
	/**
	 * The result of an ontology transformation as a new ontology source.
	 */
	class TransformedOntology : public OntologySource {
	public:
		/**
		 * Create a new transformed ontology.
		 * @param uri the URI of the original ontology.
		 * @param format the format of the original ontology.
		 */
		TransformedOntology(const URI &uri, std::string_view format);

		/**
		 * Load the transformed ontology.
		 * @param callback the callback to handle the triples.
		 * @return true if the ontology was loaded successfully.
		 */
		bool load(const TripleHandler &callback) override;

		/**
		 * @return the storage of the transformed ontology.
		 */
		auto &storage() { return storage_; }

	protected:
		std::unique_ptr<RedlandModel> storage_;
	};

} // knowrob

#endif //KNOWROB_TRANSFORMED_ONTOLOGY_H
