//
// Created by daniel on 15.07.23.
//

#include <utility>

#include "knowrob/Logger.h"
#include "knowrob/KnowledgeBase.h"
#include "knowrob/queries/QueryStage.h"
#include "knowrob/queries/AnswerTransformer.h"

using namespace knowrob;

namespace knowrob {
	class QueryStageTransformer : public AnswerStream {
	public:
		QueryStageTransformer(const std::shared_ptr<QueryStage> &queryStage,
		                      const AnswerPtr &partialResult)
		: AnswerStream(),
		  queryStage_(queryStage),
		  partialResult_(partialResult)
		{}

		void close() override {
			{
				std::lock_guard<std::mutex> lock(pushLock_);
				queryStage_ = {};
			}
			AnswerStream::close();
		}

		static AnswerPtr transformAnswer(const AnswerPtr &graphQueryAnswer, const AnswerPtr &partialResult) {
			if(AnswerStream::isEOS(graphQueryAnswer) || partialResult->substitution()->empty()) {
				return graphQueryAnswer;
			}
			else {
				auto combined = std::make_shared<Answer>(*graphQueryAnswer);
				// TODO: could be done without unification
				combined->combine(partialResult);
				return combined;
			}
		}

	protected:
		std::shared_ptr<QueryStage> queryStage_;
		std::list<QueryStage::ActiveQuery>::iterator graphQueryIterator_;
		const AnswerPtr partialResult_;
		std::mutex pushLock_;

		// Override AnswerStream
		void push(const AnswerPtr &msg) override {
			std::lock_guard<std::mutex> lock(pushLock_);
			if(queryStage_) {
				if(AnswerStream::isEOS(msg)) {
					queryStage_->pushTransformed(msg, graphQueryIterator_);
				}
				else {
					auto transformed = transformAnswer(msg, partialResult_);
					queryStage_->pushTransformed(transformed, graphQueryIterator_);
				}
			}
		}

		friend class QueryStage;
	};
}

QueryStage::QueryStage(RDFLiteralPtr literal, int queryFlags)
: AnswerBroadcaster(),
  literal_(std::move(literal)),
  queryFlags_(queryFlags),
  isQueryOpened_(true),
  isAwaitingInput_(true),
  hasStopRequest_(false)
{
}

QueryStage::~QueryStage()
{
    QueryStage::close();
}

void QueryStage::setQueryFlags(int queryFlags)
{
    queryFlags_ = queryFlags;
}

void QueryStage::close()
{
    if(hasStopRequest_) return;

    // toggle on stop request
    hasStopRequest_ = true;

    // clear all graph queries
    for(auto &pair : graphQueries_) {
        pair.first->close();
        pair.second->close();
    }
    graphQueries_.clear();

    // close all channels
    AnswerStream::close();
}

void QueryStage::pushTransformed(const AnswerPtr &transformedAnswer,
                                 std::list<ActiveQuery>::iterator graphQueryIterator)
{
	if(AnswerStream::isEOS(transformedAnswer)) {
		graphQueries_.erase(graphQueryIterator);
		// only push EOS message if no query is still active and
		// if the stream has received EOS as input already.
		if(graphQueries_.empty() && !isAwaitingInput_) {
			isQueryOpened_ = false;
			pushToBroadcast(transformedAnswer);
		}
	}
	else if(isQueryOpened()) {
		pushToBroadcast(transformedAnswer);
		// close the stage if only one solution is requested
		if((queryFlags_ & (int)QueryFlag::QUERY_FLAG_ONE_SOLUTION) == (int)QueryFlag::QUERY_FLAG_ONE_SOLUTION) close();
	}
}

void QueryStage::push(const AnswerPtr &partialResult)
{
    if(AnswerStream::isEOS(partialResult)) {
        // EOS indicates that no more input is to be expected
        isAwaitingInput_ = false;

        // only broadcast EOS if no graph query is still active.
        if(graphQueries_.empty() && !hasStopRequest_) {
            isQueryOpened_ = false;
            pushToBroadcast(partialResult);
        }
    }
    else if(!isQueryOpened()) {
        KB_WARN("ignoring attempt to write to a closed stream.");
    }
    else {
        // create a reference on self from a weak reference
        auto selfRef = selfWeakRef_.lock();
        if(!selfRef) return;

        // apply the substitution mapping
        auto literalInstance =
            std::make_shared<RDFLiteral>(*literal_, *partialResult->substitution());

        // submit a query
        auto graphQueryStream = submitQuery(literalInstance);
        // combine query result with partial answer
        auto transformer = std::make_shared<QueryStageTransformer>(selfRef, partialResult);

        // keep a reference on the stream
        auto pair = graphQueries_.emplace_front(graphQueryStream, transformer);
        auto graphQueryIt = graphQueries_.begin();
        transformer->graphQueryIterator_ = graphQueryIt;

        // combine graph query answer with partialResult and push it to the broadcast
        graphQueryStream >> transformer;

        // start sending messages into AnswerTransformer.
        // the messages are buffered before to avoid them being lost before the transformer
        // is connected.
        graphQueryStream->stopBuffering();
    }
}
