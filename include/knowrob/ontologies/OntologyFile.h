/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_ONTOLOGY_FILE_H
#define KNOWROB_ONTOLOGY_FILE_H

#include "knowrob/ontologies/OntologySource.h"
#include "knowrob/triples/TripleContainer.h"
#include "knowrob/triples/TripleFormat.h"
#include "knowrob/semweb/OntologyLanguage.h"
#include "knowrob/semweb/Vocabulary.h"

namespace knowrob {
	/**
	 * An ontology file is a data source that provides ontology data in a file.
	 */
	class OntologyFile : public OntologySource {
	public:
		/**
		 * @param vocabulary the vocabulary of the ontology.
		 * @param uri URI of the data source.
		 * @param format string identifier of the data format.
		 */
		OntologyFile(VocabularyPtr vocabulary, const URI &uri, std::string_view format);

		/**
		 * @return the format of the triples in the file.
		 */
		semweb::TripleFormat tripleFormat() const { return tripleFormat_; }

		/**
		 * @param language the language of the ontology.
		 */
		void setOntologyLanguage(semweb::OntologyLanguage language) { ontologyLanguage_ = language; }

		/**
		 * @return the language of the ontology.
		 */
		semweb::OntologyLanguage ontologyLanguage() const { return ontologyLanguage_; }

		// Override OntologySource
		bool load(const TripleHandler &callback) override;

	protected:
		VocabularyPtr vocabulary_;
		semweb::TripleFormat tripleFormat_;
		semweb::OntologyLanguage ontologyLanguage_;
	};

} // knowrob

#endif //KNOWROB_ONTOLOGY_FILE_H
