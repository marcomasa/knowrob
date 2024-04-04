/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include "knowrob/queries/RedundantAnswerFilter.h"
#include "knowrob/knowrob.h"

using namespace knowrob;

void RedundantAnswerFilter::push(const TokenPtr &tok) {
	auto msgHash = tok->hash();
	if (previousAnswers_.count(msgHash) == 0) {
		TokenBroadcaster::push(tok);
		previousAnswers_.insert(msgHash);
	}
}
