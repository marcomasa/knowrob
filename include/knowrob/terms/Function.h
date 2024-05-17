/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_FUNCTION_H
#define KNOWROB_FUNCTION_H

#include "vector"
#include "Term.h"
#include "Atom.h"

namespace knowrob {
	/**
	 * A function term is a compound term with a functor and a list of arguments.
	 */
	class Function : public Term {
	public:
		/**
		 * Constructs a function term from a functor and a list of arguments.
		 * @param functor the functor
		 * @param arguments the arguments
		 */
		Function(AtomPtr functor, const std::vector<TermPtr> &arguments);

		/**
		 * Constructs a function term from a functor and a list of arguments.
		 * @param functor the functor
		 * @param arguments the arguments
		 */
		Function(std::string_view functor, const std::vector<TermPtr> &arguments);

		/**
		 * @param other another function
		 * @return true if both functions are equal
		 */
		bool isSameFunction(const Function &other) const;

		/**
		 * @return the functor
		 */
		auto &functor() const { return functor_; }

		/**
		 * @return the arguments
		 */
		auto &arguments() const { return arguments_; }

		/**
		 * @return the arity of the function
		 */
		auto arity() const { return arguments_.size(); }

		/**
		 * @return the hash of this function.
		 */
		size_t hashOfFunction() const;

		// Override Term
		const std::set<std::string_view> &variables() const override { return variables_; }

	protected:
		const std::shared_ptr<Atom> functor_;
		const std::vector<TermPtr> arguments_;
		const std::set<std::string_view> variables_;

		std::set<std::string_view> getVariables1() const;

		// Override Term
		void write(std::ostream &os) const override;
	};

	using FunctionPtr = std::shared_ptr<Function>;

} // knowrob

#endif //KNOWROB_FUNCTION_H
