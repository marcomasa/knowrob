/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_FRAMED_TRIPLE_PATTERN_H
#define KNOWROB_FRAMED_TRIPLE_PATTERN_H

#include "knowrob/formulas/Predicate.h"
#include "knowrob/triples/FramedTriple.h"
#include "knowrob/triples/TripleContainer.h"
#include "knowrob/formulas/FirstOrderLiteral.h"
#include "knowrob/queries/QueryContext.h"
#include "knowrob/terms/groundable.h"
#include "knowrob/terms/Numeric.h"
#include "knowrob/terms/IRIAtom.h"
#include "knowrob/terms/Blank.h"

namespace knowrob {
	/**
	 * Unary operators that can be applied on terms.
	 */
	enum class FilterType {
		EQ = 0,
		NEQ,
		LT,
		GT,
		LEQ,
		GEQ
	};

	/**
	 * Compute the inverse of a filter type.
	 * @param op a filter type.
	 * @return the inverse of op.
	 */
	FilterType inverseFilterType(FilterType op);

	/**
	 * A triple expression where subject, predicate and object are
	 * represented as a term, and an additional unary operator can be applied to the object.
	 */
	class FramedTriplePattern : public FirstOrderLiteral {
	public:
		/**
		 * Copy char data of StatementData object into Term data structures.
		 * @param triple a triple.
		 * @param isNegated a value of true refers to the statement being false.
		 */
		explicit FramedTriplePattern(const FramedTriple &triple, bool isNegated = false);

		/**
		 * @param predicate a predicate with two arguments.
		 * @param isNegated a value of true refers to the statement being false.
		 */
		explicit FramedTriplePattern(const PredicatePtr &predicate, bool isNegated = false);

		/**
		 * @param s the subject term.
		 * @param p the property term.
		 * @param o the object term.
		 * @param isNegated a value of true refers to the statement being false.
		 */
		FramedTriplePattern(const TermPtr &s, const TermPtr &p, const TermPtr &o, bool isNegated = false);

		/**
		 * Apply a frame to this pattern.
		 * @param frame a triple frame.
		 */
		void setTripleFrame(const GraphSelector &frame);

		/**
		 * Apply this pattern to a frame.
		 * @param frame a triple frame.
		 */
		void getTripleFrame(GraphSelector &frame) const;

		/**
		 * @return the subject term of this expression.
		 */
		auto &subjectTerm() const { return subjectTerm_; }

		/**
		 * Set the subject term of this expression.
		 * @param subjectTerm the subject term.
		 */
		void setSubjectTerm(const TermPtr &subjectTerm) { subjectTerm_ = subjectTerm; }

		/**
		 * @return the property term of this expression.
		 */
		auto &propertyTerm() const { return propertyTerm_; }

		/**
		 * @return the object term of this expression.
		 */
		auto &objectTerm() const { return objectTerm_; }

		/**
		 * Set the object term of this expression.
		 * @param objectTerm the object term.
		 */
		void setObjectTerm(const TermPtr &objectTerm) { objectTerm_ = objectTerm; }

		/**
		 * @return an additional object variable to bind the actual value of the object term.
		 */
		auto &objectVariable() const { return objectVariable_; }

		/**
		 * Set the object variable of this expression.
		 * @param objectVariable the object variable.
		 */
		void setObjectVariable(const VariablePtr &objectVariable) { objectVariable_ = objectVariable; }

		/**
		 * @return the graph term of this expression.
		 */
		auto &graphTerm() const { return graphTerm_; }

		/**
		 * Set the graph term of this expression.
		 * @param graphTerm the graph term.
		 */
		void setGraphTerm(const groundable<Atom> &graphTerm) { graphTerm_ = graphTerm; }

		/**
		 * Set the graph term of this expression.
		 * @param graphName the name of the graph.
		 */
		void setGraphName(const std::string_view &graphName) { graphTerm_ = getGraphTerm(graphName); }

		/**
		 * @return the agent term of this expression.
		 */
		auto &perspectiveTerm() const { return perspectiveTerm_; }

		/**
		 * Set the agent term of this expression.
		 * @param perspectiveTerm the perspective term.
		 */
		void setPerspectiveTerm(const groundable<Atom> &perspectiveTerm) { perspectiveTerm_ = perspectiveTerm; }

		/**
		 * @return the begin term of this expression.
		 */
		auto &beginTerm() const { return beginTerm_; }

		/**
		 * Set the begin term of this expression.
		 * @param beginTerm the begin term.
		 */
		void setBeginTerm(const groundable<Double> &beginTerm) { beginTerm_ = beginTerm; }

		/**
		 * @return the end term of this expression.
		 */
		auto &endTerm() const { return endTerm_; }

		/**
		 * Set the end term of this expression.
		 * @param endTerm the end term.
		 */
		void setEndTerm(const groundable<Double> &endTerm) { endTerm_ = endTerm; }

		/**
		 * @return the confidence term of this expression.
		 */
		auto &confidenceTerm() const { return confidenceTerm_; }

		/**
		 * Set the confidence term of this expression.
		 * @param confidenceTerm the confidence term.
		 */
		void setConfidenceTerm(const groundable<Double> &confidenceTerm) { confidenceTerm_ = confidenceTerm; }

		/**
		 * @return the operator for the object of the triple.
		 */
		auto objectOperator() const { return objectOperator_; }

		/**
		 * Set the operator for the object of the triple.
		 * @param objectOperator the operator.
		 */
		void setObjectOperator(FilterType objectOperator) { objectOperator_ = objectOperator; }

		/**
		 * @return the isOccasional term of this expression.
		 */
		auto &isOccasionalTerm() const { return isOccasional_; }

		/**
		 * Set the isOccasional term of this expression.
		 * @param isOccasional the isOccasional term.
		 */
		void setIsOccasionalTerm(const groundable<Numeric> &isOccasional) { isOccasional_ = isOccasional; }

		/**
		 * @return the isUncertain term of this expression.
		 */
		auto &isUncertainTerm() const { return isUncertain_; }

		/**
		 * Set the isUncertain term of this expression.
		 * @param isUncertain the isUncertain term.
		 */
		void setIsUncertainTerm(const groundable<Numeric> &isUncertain) { isUncertain_ = isUncertain; }

		/**
		 * @return true if this expression is optional.
		 */
		bool isOptional() const { return isOptional_; }

		/**
		 * Set this expression to be optional.
		 * @param isOptional true if this expression is optional.
		 */
		void setIsOptional(bool isOptional) { isOptional_ = isOptional; }

		/**
		 * @return the number of variables in this expression.
		 */
		uint32_t numVariables() const override;

		/**
		 * @return the variables in this expression.
		 */
		std::vector<VariablePtr> getVariables(bool includeObjectVar = true) const;

		/**
		 * @param triple a triple query.
		 * @return true if the triple matches this pattern.
		 */
		bool filter(const FramedTriple &triple) const;

		/**
		 * Map the instantiation of this expression into a triple.
		 * @param triple the triple to be instantiated.
		 * @param bindings the substitution to be applied.
		 * @return true if the instantiation was successful.
		 */
		bool instantiateInto(FramedTriple &triple,
							 const std::shared_ptr<const Bindings> &bindings = Bindings::emptyBindings()) const;

		static std::shared_ptr<Predicate> getRDFPredicate(const PredicatePtr &predicate);

	protected:
		TermPtr subjectTerm_;
		TermPtr propertyTerm_;
		TermPtr objectTerm_;
		VariablePtr objectVariable_;
		FilterType objectOperator_;
		bool isOptional_;

		// below are treated as optional
		groundable<Atom> graphTerm_;
		groundable<Atom> perspectiveTerm_;
		groundable<Double> beginTerm_;
		groundable<Double> endTerm_;
		groundable<Double> confidenceTerm_;
		groundable<Numeric> isOccasional_;
		groundable<Numeric> isUncertain_;

		static std::shared_ptr<Atom> getGraphTerm(const std::string_view &graphName);

		static std::shared_ptr<Predicate> getRDFPredicate(const TermPtr &s, const TermPtr &p, const TermPtr &o);

		static std::shared_ptr<Predicate> getRDFPredicate(const FramedTriple &data);
	};

	/**
	 * A shared pointer to a framed triple pattern.
	 */
	using FramedTriplePatternPtr = std::shared_ptr<FramedTriplePattern>;

	/**
	 * Apply a substitution to a framed triple pattern.
	 * @param pat the framed triple pattern.
	 * @param bindings the substitution.
	 * @return the framed triple pattern with the substitution applied.
	 */
	FramedTriplePatternPtr applyBindings(const FramedTriplePatternPtr &pat, const Bindings &bindings);

	/**
	 * A container that maps a vector of framed triple patterns into a vector of framed triples.
	 */
	class TriplePatternContainer : public MutableTripleContainer {
	public:
		TriplePatternContainer() = default;

		~TriplePatternContainer();

		TriplePatternContainer(const TriplePatternContainer &other) = delete;

		/**
		 * @param q a triple query.
		 */
		void push_back(const FramedTriplePatternPtr &q);

		// Override TripleContainer
		ConstGenerator cgenerator() const override;

		// Override MutableTripleContainer
		MutableGenerator generator() override;

	protected:
		std::vector<FramedTriplePtr *> data_;
		std::vector<FramedTriplePatternPtr> statements_;
	};
} // knowrob

#endif //KNOWROB_FRAMED_TRIPLE_PATTERN_H
