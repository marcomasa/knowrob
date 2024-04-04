/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_END_OF_EVALUATION_H
#define KNOWROB_END_OF_EVALUATION_H

#include "memory"
#include "iostream"
#include "Token.h"

namespace knowrob {
	/**
	 * A control token that indicates the end of an evaluation.
	 */
	class EndOfEvaluation : public Token {
	private:
		EndOfEvaluation() : Token(TokenType::CONTROL_TOKEN) {
			isTerminalToken_ = true;
		};

	public:
		/**
		 * @return the singleton instance.
		 */
		static auto &get() {
			static std::shared_ptr<const EndOfEvaluation> instance(new EndOfEvaluation());
			return instance;
		}
	};
}

#endif //KNOWROB_END_OF_EVALUATION_H
