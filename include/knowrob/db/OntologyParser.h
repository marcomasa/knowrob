/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_ONTOLOGY_PARSER_H
#define KNOWROB_ONTOLOGY_PARSER_H

#include <functional>
#include <string>
#include <raptor.h>
#include "knowrob/semweb/TripleContainer.h"
#include "knowrob/semweb/TripleFormat.h"
#include "knowrob/semweb/FramedTriple.h"
#include "knowrob/semweb/GraphSelector.h"
#include "RaptorContainer.h"

namespace knowrob {
	/**
	 * A parser for RDF data.
	 */
	class OntologyParser {
	public:
		static const std::string NAME_TURTLE;
		static const std::string NAME_TRIG;
		static const std::string NAME_GRDDL;
		static const std::string NAME_NTRIPLES;
		static const std::string NAME_RDFXML;
		static const std::string NAME_RDFA;
		static const std::string NAME_RSS_TAG_SOUP;

		/**
		 * @param fileURI the URI of the file to parse.
		 * @param format the format of the file.
		 * @param batchSize the max size of each batch to use for parsing.
		 */
		OntologyParser(const std::string_view &fileURI, semweb::TripleFormat format, uint32_t batchSize = 1000);

		~OntologyParser();

		/**
		 * @param frame the graph selector to use for filtering triples.
		 */
		void setFrame(const GraphSelectorPtr &frame) { frame_ = frame; }

		/**
		 * @param origin the origin to use for triples.
		 */
		void setOrigin(std::string_view origin) { origin_ = origin; }

		/**
		 * @param blankPrefix the prefix to use for blank nodes.
		 */
		void setBlankPrefix(const std::string_view &blankPrefix) { blankPrefix_ = blankPrefix; }

		/**
		 * @param filter the filter to use for filtering triples.
		 */
		void setFilter(const semweb::TripleFilter &filter) { filter_ = filter; }

		/**
		 * @param callback the callback to use for handling triples.
		 * @return true if the parsing was successful, false otherwise.
		 */
		bool run(const semweb::TripleHandler &callback);

		/**
		 * @param statement a raptor statement
		 * @param callback the callback to use for handling triples.
		 */
		void add(raptor_statement *statement, const semweb::TripleHandler &callback);

		/**
		 * Flush the parser, pushing all remaining triples to the callback.
		 */
		void flush(const semweb::TripleHandler &callback);

		/**
		 * @return the list of directly imported ontologies.
		 */
		auto &imports() const { return imports_; }

		static const char* mimeType(knowrob::semweb::TripleFormat format);

	protected:
		raptor_uri *uri_;
		raptor_uri *uriBase_;
		raptor_parser *parser_;
		raptor_world *world_;
		GraphSelectorPtr frame_;
		semweb::TripleFilter filter_;
		std::string blankPrefix_;
		std::string origin_;
		std::vector<std::string> imports_;
		std::function<int()> doParse_;

		raptor_parser *createParser(knowrob::semweb::TripleFormat format);

		static raptor_world *createWorld();

		void applyFrame(FramedTriple *triple);

		std::shared_ptr<RaptorContainer> currentBatch_;
		uint32_t batchSize_;
	};

} // knowrob

#endif //KNOWROB_ONTOLOGY_PARSER_H
