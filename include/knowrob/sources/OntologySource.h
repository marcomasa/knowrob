/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_ONTOLOGY_SOURCE_H
#define KNOWROB_ONTOLOGY_SOURCE_H

#include "knowrob/sources/DataSource.h"
#include "knowrob/triples/GraphSelector.h"
#include "knowrob/triples/TripleContainer.h"

namespace knowrob {
	/**
	 * An ontology source is a data source that contains triples.
	 */
	class OntologySource : public DataSource {
	public:
		OntologySource(const URI &uri, std::string_view format)
				: DataSource(uri, format, DataSourceType::ONTOLOGY),
				  origin_(DataSource::getNameFromURI(uri())) {}

		void setFrame(const GraphSelectorPtr &frame) { frame_ = frame; }

		const auto &frame() const { return frame_; }

		/**
		 * The meaning is that parent origin imports this ontology file.
		 * @param parentOrigin the origin of the parent ontology.
		 */
		void setParentOrigin(std::string_view parentOrigin) { parentOrigin_ = parentOrigin; }

		/**
		 * @return the origin of the parent ontology.
		 */
		auto &parentOrigin() const { return parentOrigin_; }

		/**
		 * @return the origin identifier of the ontology.
		 */
		std::string_view origin() const { return origin_; }

		/**
		 * Load triples from the SPARQL endpoint.
		 * @param callback the callback to handle the triples.
		 * @return true if the triples were loaded successfully.
		 */
		virtual bool load(const TripleHandler &callback) = 0;

		/**
		 * @return the imports of the ontology.
		 */
		const auto &imports() const { return imports_; }

		/**
		 * @param imports the imports of the ontology.
		 */
		void setImports(const std::vector<std::string> &imports) { imports_ = imports; }

	protected:
		GraphSelectorPtr frame_;
		std::optional<std::string> parentOrigin_;
		std::string origin_;
		std::vector<std::string> imports_;
	};

} // knowrob

#endif //KNOWROB_ONTOLOGY_SOURCE_H
