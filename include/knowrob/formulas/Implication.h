/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_IMPLICATION_H
#define KNOWROB_IMPLICATION_H

#include "CompoundFormula.h"

namespace knowrob {
	/**
	 * An implication formula.
	 */
	class Implication : public CompoundFormula {
	public:
		/**
		 * @param antecedent the antecedent of the implication.
		 * @param consequent the consequent of the implication.
		 */
		explicit Implication(const FormulaPtr &antecedent, const FormulaPtr &consequent);

		/**
		 * @return the antecedent of the implication.
		 */
		const FormulaPtr &antecedent() const { return formulae_[0]; }

		/**
		 * @return the consequent of the implication.
		 */
		const FormulaPtr &consequent() const { return formulae_[1]; }

		// Override ConnectiveFormula
		const char *operator_symbol() const override { return "->"; }

	protected:
		bool isEqual(const Formula &other) const override;
	};

} // knowrob

#endif //KNOWROB_IMPLICATION_H
