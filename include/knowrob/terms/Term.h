/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_TERM_H_
#define KNOWROB_TERM_H_

#include <set>
#include <memory>
#include <ostream>

namespace knowrob {
	/**
	 * The type of a term.
	 */
	enum class TermType : uint8_t {
		/** atomic term */
		ATOMIC = 0,
		/** a variable */
		VARIABLE,
		/** compound term with functor and arguments */
		FUNCTION
	};

	/**
	 * Terms are used as components of formulas and are recursively
	 * constructed over the set of constants, variables, and function symbols.
	 */
	class Term {
	public:
		explicit Term(TermType termType) : termType_(termType) {};

		/**
		 * @param other another term
		 * @return true if both terms are equal
		 */
		bool operator==(const Term &other) const;

		/**
		 * @param other another term
		 * @return true if both terms are not equal
		 */
		bool operator!=(const Term &other) const { return !this->operator==(other); }

		/**
		 * @return the type of this term.
		 */
		TermType termType() const { return termType_; }

		/**
		 * @return true if this term has no variables.
		 */
		bool isGround() const { return variables().empty(); }

		/**
		 * @return true if this term is bound and not compound.
		 */
		bool isAtomic() const { return termType_ == TermType::ATOMIC; }

		/**
		 * @return true if this term is an atom.
		 */
		bool isAtom() const;

		/**
		 * @return true if this term is a variable.
		 */
		bool isVariable() const;

		/**
		 * @return true if this term is a function.
		 */
		bool isFunction() const;

		/**
		 * @return true if this term is a numeric.
		 */
		bool isNumeric() const;

		/**
		 * @return true if this term is a string.
		 */
		bool isString() const;

		/**
		 * @return true if this term is an IRI.
		 */
		virtual bool isIRI() const;

		/**
		 * @return true if this term is a blank node.
		 */
		virtual bool isBlank() const;

		/**
		 * @return set of variables of this term.
		 */
		virtual const std::set<std::string_view> &variables() const = 0;

		/**
		 * @return the hash of this.
		 */
		size_t hash() const;

	protected:
		static const std::set<std::string_view> noVariables_;
		const TermType termType_;

		/**
		 * Write the term into an ostream.
		 */
		virtual void write(std::ostream &os) const = 0;

		friend struct TermWriter;
	};

	// alias declaration
	using TermPtr = std::shared_ptr<Term>;

	/**
	 * Writes a term into an ostream.
	 */
	struct TermWriter {
		TermWriter(const Term &term, std::ostream &os) { term.write(os); }
	};
}

namespace std {
	std::ostream &operator<<(std::ostream &os, const knowrob::Term &t);
}

#endif //KNOWROB_TERM_H_
