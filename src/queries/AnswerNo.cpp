/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include "knowrob/queries/AnswerNo.h"
#include "knowrob/knowrob.h"
#include "knowrob/integration/python/utils.h"

namespace knowrob {
	const std::shared_ptr<const AnswerNo> &GenericNo() {
		static const auto instance = std::make_shared<const AnswerNo>();
		return instance;
	}
} // namespace knowrob

using namespace knowrob;

AnswerNo::AnswerNo()
		: Answer() {
	setIsNegative(true);
}

AnswerNo::AnswerNo(const AnswerNo &other)
		: Answer(other) {
	setIsNegative(true);
}

void AnswerNo::addUngrounded(const std::shared_ptr<Predicate> &predicate, bool isNegated) {
	if (isNegated) {
		negativeUngrounded_.emplace_back(predicate, DefaultGraphSelector(), reasonerTerm_);
	} else {
		positiveUngrounded_.emplace_back(predicate, DefaultGraphSelector(), reasonerTerm_);
	}
}

bool AnswerNo::mergeWith(const AnswerNo &other) {
	reasonerTerm_ = {};
	if (!frame_->mergeWith(*other.frame_)) {
		// merging frames failed -> results cannot be combined
		return false;
	}
	// insert groundings of other answer
	positiveUngrounded_.insert(positiveUngrounded_.end(),
							   other.positiveUngrounded_.begin(), other.positiveUngrounded_.end());
	negativeUngrounded_.insert(negativeUngrounded_.end(),
							   other.negativeUngrounded_.begin(), other.negativeUngrounded_.end());
	return true;
}

std::string AnswerNo::stringFormOfNo() const {
	std::stringstream os;
	if (reasonerTerm_) {
		os << "[" << *reasonerTerm_ << "] ";
	}
	if (isUncertain()) {
		os << "probably ";
	}
	os << "no";
	if (!positiveUngrounded_.empty() || !negativeUngrounded_.empty()) {
		os << ", because:\n";
		for (auto &x: positiveUngrounded_) {
			os << '\t' << *x.graphSelector() << ' ' << '~' << *x.predicate();
			if (x.reasonerTerm() && x.reasonerTerm() != reasonerTerm_) {
				os << ' ' << '[' << *x.reasonerTerm() << "]";
			}
			os << '\n';
		}
		for (auto &x: negativeUngrounded_) {
			os << '\t' << *x.graphSelector() << *x.predicate();
			if (x.reasonerTerm() && x.reasonerTerm() != reasonerTerm_) {
				os << ' ' << '[' << *x.reasonerTerm() << "]";
			}
			os << '\n';
		}
	} else {
		os << '\n';
	}
	return os.str();
}

std::string AnswerNo::humanReadableFormOfNo() const {
	static const std::string longMsg = "there was evidence supporting the query to be false";
	return longMsg;
}

namespace knowrob {
	AnswerPtr mergeNegativeAnswers(const AnswerNoPtr &a, const AnswerNoPtr &b) {
		auto mergedAnswer = std::make_shared<AnswerNo>(*a);
		if (mergedAnswer->mergeWith(*b)) {
			return mergedAnswer;
		} else {
			// merging failed
			return {};
		}
	}
} // knowrob

namespace knowrob::py {
	template<>
	void createType<AnswerNo>() {
		using namespace boost::python;
		class_<AnswerNo, bases<Answer>, std::shared_ptr<AnswerNo>, boost::noncopyable>
				("AnswerNo", init<>())
				.def("addUngrounded", &AnswerNo::addUngrounded)
				.def("positiveUngrounded", &AnswerNo::positiveUngrounded, return_value_policy<reference_existing_object>())
				.def("negativeUngrounded", &AnswerNo::negativeUngrounded, return_value_policy<reference_existing_object>())
				.def("mergeWith", &AnswerNo::mergeWith)
				.def("stringFormOfNo", &AnswerNo::stringFormOfNo)
				.def("humanReadableFormOfNo", &AnswerNo::humanReadableFormOfNo);
	}
}
