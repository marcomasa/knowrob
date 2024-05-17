/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_REASONER_ERROR_H_
#define KNOWROB_REASONER_ERROR_H_

#include <fmt/core.h>
#include <knowrob/KnowRobError.h>

namespace knowrob {
	/**
	 * A reasoner-related runtime error.
	 */
	class ReasonerError : public KnowRobError {
	public:
		/**
		 * @tparam Args fmt-printable arguments.
		 * @param fmt A fmt string pattern.
		 * @param args list of arguments used to instantiate the pattern.
		 */
		template<typename ... Args>
		explicit ReasonerError(const char *fmt, Args &&... args)
				: KnowRobError("ReasonerError", fmt::format(fmt, args...)) {}
	};
}

#endif //KNOWROB_REASONER_ERROR_H_
