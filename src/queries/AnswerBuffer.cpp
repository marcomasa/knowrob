//
// Created by daniel on 31.03.23.
//

#include "knowrob/queries/AnswerBuffer.h"
#include "knowrob/queries/AnswerQueue.h"
#include "knowrob/Logger.h"

using namespace knowrob;

AnswerBuffer::AnswerBuffer()
: AnswerBroadcaster(), isBuffering_(true)
{}

void AnswerBuffer::stopBuffering()
{
    if(isBuffering_) {
        isBuffering_ = false;
        for(auto &buffered : buffer_) {
            AnswerBroadcaster::push(buffered);
        }
        buffer_.clear();
    }
}

std::shared_ptr<AnswerQueue> AnswerBuffer::createQueue()
{
    auto queue = std::make_shared<AnswerQueue>();
    addSubscriber(Channel::create(queue));
    stopBuffering();
    // TODO: should keep reference on buffer?
    return queue;
}

void AnswerBuffer::push(const AnswerPtr &msg)
{
    if(isBuffering_) {
        buffer_.push_back(msg);
    }
    else {
        AnswerBroadcaster::push(msg);
    }
}
