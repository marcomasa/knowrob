/*
 * Copyright (c) 2022, Daniel Beßler
 * All rights reserved.
 *
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

// logging
#include <spdlog/spdlog.h>
// KnowRob
#include <knowrob/qa/Blackboard.h>

using namespace knowrob;

Blackboard::Blackboard(
	const std::shared_ptr<ReasonerManager> &reasonerManager,
	const std::shared_ptr<QueryResultQueue> &outputQueue,
	const std::shared_ptr<Query> &goal)
: reasonerManager_(reasonerManager),
  outputQueue_(outputQueue),
  goal_(goal)
{
	// decompose the reasoning task
	inputStream_ = std::shared_ptr<QueryResultBroadcaster>(new QueryResultBroadcaster);
	inputChannel_ = inputStream_->createChannel();
	std::shared_ptr<QueryResultBroadcaster> out =
		std::shared_ptr<QueryResultBroadcaster>(new QueryResultBroadcaster);
	
	// create a channel of outputQueue_, and add it as subscriber
	auto queueChannel = outputQueue_->createChannel();
	out->addSubscriber(queueChannel);
	
	// decompose into atomic formulas
	decompose(goal->formula(), inputStream_, out);
}

Blackboard::~Blackboard()
{
	stop();
}

void Blackboard::start()
{
	// push empty message into in stream
	inputChannel_->push(QueryResultStream::bos());
	inputChannel_->close();
}

void Blackboard::stop()
{
	for(auto &segment : segments_) {
		segment->stop();
	}
}

void Blackboard::decompose(const std::shared_ptr<Formula> &phi,
		std::shared_ptr<QueryResultBroadcaster> &in,
		std::shared_ptr<QueryResultBroadcaster> &out)
{
	switch(phi->type()) {
	case FormulaType::PREDICATE: {
		decomposePredicate(
			std::static_pointer_cast<PredicateFormula>(phi),
			in, out);
		break;
	}
	case FormulaType::CONJUNCTION: {
		decomposeConjunction(
			std::static_pointer_cast<ConjunctionFormula>(phi),
			in, out);
		break;
	}
	case FormulaType::DISJUNCTION: {
		decomposeDisjunction(
			std::static_pointer_cast<DisjunctionFormula>(phi),
			in, out);
		break;
	}
	default:
		spdlog::warn("Ignoring unknown formula type '{}'.", phi->type());
		break;
	}
}

void Blackboard::decomposePredicate(
		const std::shared_ptr<PredicateFormula> &phi,
		std::shared_ptr<QueryResultBroadcaster> &in,
		std::shared_ptr<QueryResultBroadcaster> &out)
{
	std::shared_ptr<Blackboard::Stream> reasonerIn;
	std::shared_ptr<Blackboard::Segment> segment;
	std::shared_ptr<QueryResultStream::Channel> reasonerInChan, reasonerOutChan;
	
	// get ensemble of reasoner
	std::list<std::shared_ptr<IReasoner>> ensemble =
		reasonerManager_->getReasonerForPredicate(phi->predicate()->indicator());
	// create a subquery for the predicate p
	std::shared_ptr<Query> subq = std::shared_ptr<Query>(new Query(phi));

	for(std::shared_ptr<IReasoner> &r : ensemble) {
		// create IO channels
		reasonerOutChan = out->createChannel();
		reasonerIn = std::shared_ptr<Blackboard::Stream>(
			new Blackboard::Stream(r,reasonerOutChan,subq));
		reasonerInChan = reasonerIn->createChannel();
		in->addSubscriber(reasonerInChan);
		// create a new segment
		segments_.push_back(std::shared_ptr<Segment>(new Segment(reasonerIn)));
	}
}

void Blackboard::decomposeConjunction(
		const std::shared_ptr<ConjunctionFormula> &phi,
		std::shared_ptr<QueryResultBroadcaster> &firstQueue,
		std::shared_ptr<QueryResultBroadcaster> &lastQueue)
{
	std::shared_ptr<QueryResultBroadcaster> in = firstQueue;
	std::shared_ptr<QueryResultBroadcaster> out;
	
	// TODO: compute a dependency relation between atomic formulae of a query.
	// atomic formulae that are independent can be evaluated concurrently.
	
	// add blackboard segments for each predicate in the conjunction
	int counter = 1;
	int numFormulae = phi->formulae().size();
	for(const std::shared_ptr<Formula> &psi : phi->formulae()) {
		if(numFormulae == counter) {
			out = lastQueue;
		}
		else {
			out = std::shared_ptr<QueryResultBroadcaster>(new QueryResultBroadcaster);
		}
		decompose(psi, in, out);
		// output queue of this predicate is used as input for the next one
		in = out;
		counter += 1;
	}
}

void Blackboard::decomposeDisjunction(
		const std::shared_ptr<DisjunctionFormula> &phi,
		std::shared_ptr<QueryResultBroadcaster> &in,
		std::shared_ptr<QueryResultBroadcaster> &out)
{
	// add blackboard segments for each formula in the disjunction.
	// all having the same input and out stream.
	for(const std::shared_ptr<Formula> &psi : phi->formulae()) {
		decompose(psi, in, out);
	}
}


Blackboard::Stream::Stream(
	const std::shared_ptr<IReasoner> &reasoner,
	const std::shared_ptr<QueryResultStream::Channel> &outputStream,
	const std::shared_ptr<Query> &goal)
: QueryResultStream(),
  queryID_(reinterpret_cast<std::uintptr_t>(this)),
  reasoner_(reasoner),
  isQueryOpened_(true),
  hasStopRequest_(false)
{
	// tell the reasoner that a new request has been made
	reasoner_->startQuery(queryID_, outputStream, goal);
}

Blackboard::Stream::~Stream()
{
	stop();
}

void Blackboard::Stream::push(QueryResultPtr &msg)
{
	if(QueryResultStream::isEOS(msg)) {
		// tell the reasoner to finish up.
		// if hasStopRequest_=true it means that the reasoner is requested
		// to immediately shutdown. however, note that not all reasoner
		// implementations may support this and may take longer to stop anyways.
		if(isQueryOpened()) {
			reasoner_->finishQuery(queryID_, hasStopRequest());
			{
				std::lock_guard<std::mutex> lock(mutex_);
				isQueryOpened_ = false;
			}
		}
	}
	else if(!isQueryOpened()) {
		spdlog::warn("ignoring attempt to write to a closed stream.");
	}
	else {
		// push substitution to reasoner
		reasoner_->pushSubstitution(queryID_, msg);
	}
}

bool Blackboard::Stream::isQueryOpened() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return isQueryOpened_;
}

bool Blackboard::Stream::hasStopRequest() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return hasStopRequest_;
}

void Blackboard::Stream::stop()
{
	{
		std::lock_guard<std::mutex> lock(mutex_);
		hasStopRequest_ = true;
	}
	close();
}


Blackboard::Segment::Segment(const std::shared_ptr<Blackboard::Stream> &in)
: in_(in)
{}

void Blackboard::Segment::stop()
{
	in_->stop();
}

