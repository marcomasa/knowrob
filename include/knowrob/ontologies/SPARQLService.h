/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_SPARQL_SERVICE_H
#define KNOWROB_SPARQL_SERVICE_H

#include <redland.h>
#include "knowrob/triples/TripleFormat.h"
#include "knowrob/triples/TripleContainer.h"
#include "OntologySource.h"
#include "knowrob/backend/redland/RedlandModel.h"

namespace knowrob {
	/**
	 * A SPARQL service is a data service that can be queried for triples.
	 */
	class SPARQLService : public OntologySource {
	public:
		/**
		 * @param uri the URI of the SPARQL endpoint.
		 * @param format the format of the triples in the endpoint.
		 */
		SPARQLService(const URI &uri, semweb::TripleFormat format);

		/**s
		 * @param uri the URI of the SPARQL endpoint.
		 * @param format the format of the triples in the endpoint.
		 */
		SPARQLService(const URI &uri, std::string_view format);

		// Override OntologySource
		bool load(const TripleHandler &callback) override;

	protected:
		RedlandModel model_;
	};

} // knowrob

#endif //KNOWROB_SPARQL_SERVICE_H
