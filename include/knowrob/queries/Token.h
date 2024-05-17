/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_TOKEN_H
#define KNOWROB_TOKEN_H

#include <memory>
#include <map>
#include <vector>
#include <string>

namespace knowrob {
	/**
	 * The type of a token.
	 */
	enum class TokenType : uint8_t {
		// A control token is used to control the evaluation pipeline.
		CONTROL_TOKEN = 0,
		// An answer token is the result of a query evaluation.
		ANSWER_TOKEN
	};

	/**
	 * A token is a single element in a query evaluation pipeline.
	 */
	class Token {
	public:
		explicit Token(TokenType tokenType) : tokenType_(tokenType) {};

		/**
		 * @return the type of this token.
		 */
		TokenType tokenType() const { return tokenType_; }

		/**
		 * @return the hash of this token.
		 */
		size_t hash() const;

		/**
		 * @return a programmer-readable string representation of this token.
		 */
		std::string stringForm() const;

		/**
		 * @return true if this token is a control token.
		 */
		bool isControlToken() const { return tokenType() == TokenType::CONTROL_TOKEN; }

		/**
		 * @return true if this token is an answer token.
		 */
		bool isAnswerToken() const { return tokenType() == TokenType::ANSWER_TOKEN; }

		/**
		 * @return true if this token indicates the end of an evaluation.
		 */
		bool indicatesEndOfEvaluation() const { return isTerminalToken_; }

	protected:
		TokenType tokenType_;
		bool isTerminalToken_ = false;
	};

	// alias
	using TokenPtr = std::shared_ptr<const Token>;
	using TokenMap = std::map<uint32_t, std::vector<TokenPtr>>;
}

namespace std {
	std::ostream &operator<<(std::ostream &os, const knowrob::Token &tok);

	std::ostream &operator<<(std::ostream &os, const knowrob::TokenPtr &tok);
}

#endif //KNOWROB_TOKEN_H
