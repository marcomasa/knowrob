/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <utility>

#include "iomanip"
#include "knowrob/queries/AnswerYes.h"
#include "knowrob/knowrob.h"
#include "knowrob/Logger.h"
#include "knowrob/integration/python/utils.h"

using namespace knowrob;

AnswerYes::AnswerYes()
		: Answer(),
		  substitution_(std::make_shared<Bindings>()) {
	setIsPositive(true);
}

AnswerYes::AnswerYes(BindingsPtr substitution)
		: Answer(),
		  substitution_(std::move(substitution)) {
	setIsPositive(true);
}

AnswerYes::AnswerYes(const AnswerYes &other)
		: Answer(other),
		  substitution_(std::make_shared<Bindings>(*other.substitution_)),
		  positiveGroundings_(other.positiveGroundings_),
		  negativeGroundings_(other.negativeGroundings_) {
	setIsPositive(true);
}

bool AnswerYes::isRicherThan(const AnswerYes &other) const {
	if (isUncertain() != other.isUncertain()) {
		// certain answer is richer than uncertain answer
		return isUncertain() < other.isUncertain();
	} else if (substitution_->size() != other.substitution_->size()) {
		// answer with more substitutions is richer
		return substitution_->size() > other.substitution_->size();
	} else if (positiveGroundings_.size() != other.positiveGroundings_.size()) {
		// answer with more positive groundings is richer
		return positiveGroundings_.size() > other.positiveGroundings_.size();
	} else if (negativeGroundings_.size() != other.negativeGroundings_.size()) {
		// answer with more negative groundings is richer
		return negativeGroundings_.size() > other.negativeGroundings_.size();
	} else {
		return false;
	}
}

bool AnswerYes::isGenericYes() const {
	return substitution_->empty();
}

bool AnswerYes::addGrounding(const std::shared_ptr<Predicate> &predicate,
							 const GraphSelectorPtr &frame,
							 bool isNegated) {
	if (!frame_->mergeWith(*frame)) {
		KB_WARN("Failed to frame \"{}\" with \"{}\".", *frame_, *frame);
		return false;
	}
	if (isNegated) {
		negativeGroundings_.emplace_back(predicate, frame, reasonerTerm_);
	} else {
		positiveGroundings_.emplace_back(predicate, frame, reasonerTerm_);
	}
	return true;
}

bool AnswerYes::mergeWith(const AnswerYes &other, bool ignoreInconsistencies) {
	auto mergedBindings = std::make_shared<Bindings>(*substitution_);
	if (ignoreInconsistencies) {
		// insert all substitutions of other answer, possibly overwriting existing ones
		*mergedBindings += *other.substitution_;
	} else {
		// unify substitutions
		if (!mergedBindings->unifyWith(*other.substitution_)) {
			// unification failed -> results cannot be combined
			return false;
		}
	}
	substitution_ = mergedBindings;

	if (!frame_->mergeWith(*other.frame_)) {
		// merging frames failed -> results cannot be combined
		return false;
	}

	reasonerTerm_ = {};
	// insert groundings of other answer
	positiveGroundings_.insert(positiveGroundings_.end(),
							   other.positiveGroundings_.begin(), other.positiveGroundings_.end());
	negativeGroundings_.insert(negativeGroundings_.end(),
							   other.negativeGroundings_.begin(), other.negativeGroundings_.end());

	return true;
}

std::string AnswerYes::stringFormOfYes() const {
	std::stringstream os;
	if (reasonerTerm_) {
		os << "[" << *reasonerTerm_ << "] ";
	}
	if (isUncertain()) {
		os << "probably ";
	}
	os << "yes";
	if (!positiveGroundings_.empty() || !negativeGroundings_.empty()) {
		os << ", because:\n";
		for (auto &x: positiveGroundings_) {
			os << '\t' << *x.graphSelector() << ' ' << *x.predicate();
			if (x.reasonerTerm() && x.reasonerTerm() != reasonerTerm_) {
				os << ' ' << '[' << *x.reasonerTerm() << "]";
			}
			os << '\n';
		}
		for (auto &x: negativeGroundings_) {
			os << '\t' << *x.graphSelector() << '~' << *x.predicate();
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

std::string AnswerYes::humanReadableFormOfYes() const {
	std::stringstream os;
	os << std::setprecision(4);
	os << "the query is ";
	if (isUncertain()) {
		if (frame_->confidence.has_value()) {
			os << "true with a confidence of " << std::setprecision(4) << frame_->confidence.value();
		} else {
			os << "probably true";
		}
	} else {
		os << "true";
	}
	if (frame_->occasional) {
		os << " at some time ";
	}
	if (frame_->begin.has_value() && frame_->end.has_value()) {
		os << " during the time points " << frame_->begin.value() << " and " << frame_->end.value();
	} else if (frame_->begin.has_value()) {
		os << " since " << frame_->begin.value();
	} else if (frame_->end.has_value()) {
		os << " until " << frame_->end.value();
	}
	os << ".";
	return os.str();
}

namespace knowrob {
	const std::shared_ptr<const AnswerYes> &GenericYes() {
		static const auto instance = std::make_shared<const AnswerYes>();
		return instance;
	}

	AnswerPtr mergePositiveAnswers(const AnswerYesPtr &a, const AnswerYesPtr &b, bool ignoreInconsistencies) {
		if (!a || a->isGenericYes()) {
			return b;
		} else if (!b || b->isGenericYes()) {
			return a;
		} else {
			AnswerYesPtr smaller, larger;
			if (a->isRicherThan(*b)) {
				smaller = b;
				larger = a;
			} else {
				smaller = a;
				larger = b;
			}
			auto mergedAnswer = std::make_shared<AnswerYes>(*larger);
			if (mergedAnswer->mergeWith(*smaller, ignoreInconsistencies)) {
				return mergedAnswer;
			} else {
				// merging failed
				return {};
			}
		}
	}
} // namespace knowrob

namespace knowrob::py {
	template<>
	void createType<AnswerYes>() {
		using namespace boost::python;
		class_<AnswerYes, bases<Answer>, std::shared_ptr<AnswerYes>, boost::noncopyable>
				("AnswerYes", init<>())
				.def("stringFormOfYes", &AnswerYes::stringFormOfYes)
				.def("humanReadableFormOfYes", &AnswerYes::humanReadableFormOfYes)
				.def("hasGrounding", &AnswerYes::hasGrounding)
				.def("addGrounding", &AnswerYes::addGrounding)
				.def("substitution", &AnswerYes::substitution, return_value_policy<reference_existing_object>())
				.def("positiveGroundings", &AnswerYes::positiveGroundings, return_value_policy<reference_existing_object>())
				.def("negativeGroundings", &AnswerYes::negativeGroundings, return_value_policy<reference_existing_object>())
				.def("mergeWith", &AnswerYes::mergeWith)
				.def("isRicherThan", &AnswerYes::isRicherThan)
				.def("isGenericYes", &AnswerYes::isGenericYes);
	}
}
