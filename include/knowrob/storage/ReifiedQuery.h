/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_REIFIED_QUERY_H
#define KNOWROB_REIFIED_QUERY_H

#include "knowrob/triples/GraphQuery.h"
#include "knowrob/semweb/Vocabulary.h"
#include "knowrob/triples/GraphPattern.h"
#include "knowrob/triples/GraphUnion.h"
#include "knowrob/triples/GraphSequence.h"

namespace knowrob {
	enum ReificationFlag {
		IncludeOriginal = 1 << 0,
		IncludeReified  = 1 << 1,
	};

	/**
	 * Reifies an inout query such that it can be issued to a backend that does not support
	 * contextualization of triples.
	 */
	class ReifiedQuery : public GraphQuery {
	public:
		/**
		 * @param nonReified a query that is not reified.
		 * @param vocabulary the vocabulary to use for reification.
		 * @param withFullFrame if true, the reified query will contain a full frame for each reified triple.
		 */
		explicit ReifiedQuery(const std::shared_ptr<GraphQuery> &nonReified, VocabularyPtr vocabulary, bool withFullFrame = false);

		/**
		 * @param nonReified a query that is not reified.
		 * @param vocabulary the vocabulary to use for reification.
		 * @param withFullFrame if true, the reified query will contain a full frame for each reified triple.
		 */
		explicit ReifiedQuery(const FramedTriplePattern &nonReified, VocabularyPtr vocabulary, bool withFullFrame = false);

		/**
		 * @return true if the query has reifiable patterns.
		 */
		static bool hasReifiablePattern(const std::shared_ptr<GraphQuery> &nonReified);

		/**
		 * @param q a triple query.
		 * @return a bitmask of ReificationFlag's that indicates if the pattern may have instances in the original or reified form.
		 */
		static int getReificationFlags(const FramedTriplePattern &q);

	protected:
		VocabularyPtr vocabulary_;
		uint32_t varCounter_;
		bool withFullFrame_;

		void setNonReified(const std::shared_ptr<GraphTerm> &nonReified);

		std::shared_ptr<GraphTerm> reifiedPatternSequence(const FramedTriplePattern &pattern);

		std::shared_ptr<GraphTerm> reifyPattern(const std::shared_ptr<GraphPattern> &nonReified);

		std::shared_ptr<GraphUnion> reifyUnion(const std::shared_ptr<GraphUnion> &graphUnion);

		std::shared_ptr<GraphSequence> reifySequence(const std::shared_ptr<GraphSequence> &graphSequence);

		void reifyConnective(const std::shared_ptr<GraphConnective> &reifiedConnective, const std::shared_ptr<GraphConnective> &originalConnective);

		static bool hasReifiablePattern(const GraphTerm *term);
	};

} // knowrob

#endif //KNOWROB_REIFIED_QUERY_H
