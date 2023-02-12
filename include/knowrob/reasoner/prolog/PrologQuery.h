/*
 * Copyright (c) 2022, Daniel Beßler
 * All rights reserved.
 *
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_PROLOG_QUERY_H_
#define KNOWROB_PROLOG_QUERY_H_

// STD
#include <string>
#include <memory>
#include <map>
// SWI Prolog
#include <SWI-Prolog.h>
// KnowRob
#include "knowrob/terms/Term.h"
#include "knowrob/queries/Query.h"
#include "knowrob/formulas/ConnectiveFormula.h"

namespace knowrob {
	/**
	 * Maps a Query object into term_t type of Prolog.
	 */
	class PrologQuery {
	public:
		/**
		 * @query a Query object.
		 */
		PrologQuery(const std::shared_ptr<const Query> &query);
		
		~PrologQuery();

		// copy not supported
		PrologQuery(const PrologQuery&) = delete;

		/**
		 * Returns the query as a term_t.
		 * @return the term_t reference.
		 */
		term_t& pl_query() { return pl_query_; }
		
		/**
		 * Returns the query object.
		 * @return the Query object
		 */
		const std::shared_ptr<const Query>& qa_query() const { return qa_query_; }

		/**
		 * Get map of all variables in this query.
		 * @return the map.
		 */
		const std::map<std::string, term_t>& vars() const{ return vars_; }

		/**
		 * Translates a term_t reference to a Term object.
		 * @t a term_t reference.
		 * @return the Term object created.
		 */
		static TermPtr constructTerm(const term_t &t);
		
		/**
		 * Creates a query from a term pointer by translation into a formula.
		 * @t a Term pointer.
		 * @return the Query object created.
		 */
		static std::shared_ptr<const Query> toQuery(const TermPtr &t);

		/**
		 * Converts a formula to a term by converting formula
		 * operators to functors of predicates.
		 * @param phi a formula
		 * @return the term created
		 */
		static TermPtr toTerm(const FormulaPtr &phi);

		/**
		 * @return the 'fail' atom.
		 */
		static const atom_t& ATOM_fail();
		/**
		 * @return the 'false' atom.
		 */
		static const atom_t& ATOM_false();
		/**
		 * @return the 'true' atom.
		 */
		static const atom_t& ATOM_true();
		/**
		 * @return the ',' atom.
		 */
		static const atom_t& ATOM_comma();
		/**
		 * @return the ';' atom.
		 */
		static const atom_t& ATOM_semicolon();

		/**
		 * @return the functor ','/2.
		 */
		static const functor_t& FUNCTOR_comma();
		/**
		 * @return the functor ';'/2.
		 */
		static const functor_t& FUNCTOR_semicolon();

		/**
		 * @return the comma predicate.
		 */
		static const predicate_t& PREDICATE_comma();
		/**
		 * @return the semicolon predicate.
		 */
		static const predicate_t& PREDICATE_semicolon();
		
	protected:
		std::shared_ptr<const Query> qa_query_;
		term_t pl_query_;
		std::map<std::string, term_t> vars_;

		bool constructPrologTerm(const TermPtr& qa_term, term_t &pl_term);
		bool constructPrologTerm(const FormulaPtr& phi, term_t &pl_term);
		bool constructPrologTerm(ConnectiveFormula *phi, const functor_t &pl_functor, term_t &pl_term);
		
		static FormulaPtr toFormula(const TermPtr &t);
		static TermPtr    toTerm(ConnectiveFormula *psi, const std::shared_ptr<PredicateIndicator> &indicator);
	};
}

#endif //KNOWROB_PROLOG_QUERY_H_