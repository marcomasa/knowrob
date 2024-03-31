/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_QUERY_PIPELINE_H
#define KNOWROB_QUERY_PIPELINE_H

#include "memory"
#include "vector"
#include "TokenStream.h"
#include "knowrob/semweb/RDFComputable.h"
#include "knowrob/formulas/DependencyGraph.h"
#include "knowrob/triples/GraphPathQuery.h"

namespace knowrob {
	class KnowledgeBase;

	/**
	 * Holds a reference to pipeline stages during execution,
	 * and stops each stage on destruction ensuring that none of them
	 * continues broadcasting messages.
	 */
	class QueryPipeline {
	public:
		/**
		 * Create a query pipeline for the given formula.
		 * @param kb the knowledge base to query.
		 * @param phi the formula to query.
		 * @param ctx the query context.
		 */
		QueryPipeline(KnowledgeBase *kb, const FormulaPtr &phi, const QueryContextPtr &ctx);

		/**
		 * Create a query pipeline for the given graph query.
		 * @param kb the knowledge base to query.
		 * @param graphQuery the graph query to execute.
		 */
		QueryPipeline(KnowledgeBase *kb, const GraphPathQueryPtr &graphQuery);

		~QueryPipeline();

		/**
		 * Stream the last stage of the pipeline into the given stage.
		 * @param stage the stage to stream the last stage into.
		 */
		void operator>>(const std::shared_ptr<TokenStream> &stage);

		/**
		 * After creation of the pipeline, messages are buffered until this is called.
		 */
		void stopBuffering();

	protected:
		std::vector<std::shared_ptr<TokenStream>> stages_;
		std::shared_ptr<TokenBroadcaster> finalStage_;
		std::shared_ptr<TokenBuffer> bufferStage_;

		void addStage(const std::shared_ptr<TokenStream> &stage);

		static std::vector<RDFComputablePtr> createComputationSequence(
				KnowledgeBase *kb,
				const std::list<DependencyNodePtr> &dependencyGroup);

		void createComputationPipeline(
				KnowledgeBase *kb,
				const std::vector<RDFComputablePtr> &computableLiterals,
				const std::shared_ptr<TokenBroadcaster> &pipelineInput,
				const std::shared_ptr<TokenBroadcaster> &pipelineOutput,
				const QueryContextPtr &ctx);
	};

	class AnswerBuffer_WithReference : public TokenBuffer {
	public:
		explicit AnswerBuffer_WithReference(const std::shared_ptr<QueryPipeline> &pipeline)
				: TokenBuffer(), pipeline_(pipeline) {}
	protected:
		std::shared_ptr<QueryPipeline> pipeline_;
	};
}


#endif //KNOWROB_QUERY_PIPELINE_H
