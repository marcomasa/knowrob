:- module(mongolog_unification, []).
/** <module> Unification of terms in mongolog programs.

The following predicates are supported:

| Predicate  | Arguments |
| ---        | ---       |
| =/2        | ?Term1, ?Term2 |
| \=/2       | ?Term1, ?Term2 |
| assign/2   | -Term1, +Term2 |

@author Daniel Beßler
@see https://www.swi-prolog.org/pldoc/man?section=compare
@license BSD
*/

:- use_module('mongolog').

%% mongolog:add_command
:- mongolog:add_command(=,2).
:- mongolog:add_command(\=,2).
:- mongolog:add_command(assign,2).

%% @Term1 \= @Term2
%
% Equivalent to \+Term1 = Term2.
%
% This predicate is logically sound if its arguments are sufficiently instantiated.
% In other cases, such as ?- X \= Y., the predicate fails although there are solutions.
% This is due to the incomplete nature of \+/1. 
%
mongolog:step_expand(\=(A,B), Expanded) :-
	mongolog:step_expand(\+(=(A,B)), Expanded).

%%
mongolog:step_compile(
		assign(Var,Value), Ctx,
		[['$set', [[Key,Value0]]]]) :-
	mongolog:var_key_or_val(Value,Ctx,Value0),
	mongolog:var_key(Var,Ctx,Key).

%% ?Term1 = ?Term2
% Unify Term1 with Term2. True if the unification succeeds.
%
mongolog:step_compile(=(Term1, Term2), _, _) :-
	\+ unifiable(Term1, Term2, _), !,
	fail.

mongolog:step_compile(=(Term1, Term2), Ctx, Pipeline) :-
	mongolog:var_key_or_val(Term1,Ctx,Term1_val),
	mongolog:var_key_or_val(Term2,Ctx,Term2_val),
	findall(Step,
		(	mongolog:set_if_var(Term1, Term2_val, Ctx, Step)
		;	mongolog:set_if_var(Term2, Term1_val, Ctx, Step)
		% make both terms accessible via fields
		;	Step=['$set', ['t_term1', Term1_val]]
		;	Step=['$set', ['t_term2', Term2_val]]
		% assign vars in term1 to values of arguments in term2
		;	set_term_arguments(Term1, Term2, 't_term1', 't_term2', Step)
		% assign vars in term2 to values of arguments in term1
		;	set_term_arguments(Term2, Term1, 't_term2', 't_term1', Step)
		% perform equality tests
		;	mongolog:match_equals(string('$t_term1'), string('$t_term2'), Step)
		% project new variable groundings
		;	set_term_vars(Term1, 't_term1', Ctx, Step)
		;	set_term_vars(Term2, 't_term2', Ctx, Step)
		% and cleanup
		;	Step=['$unset', array([string('t_term1'),string('t_term2')])]
		),
		Pipeline).

%%
% this operation replaces all variable arguments in Term1 with
% arguments in Term2.
% NOTE: variables are also replaced if the argument in Term2 is also a variable.
%       this is important for later equality tests!
%
set_term_arguments(List1, List2, List1Key, List2Key,
		['$set', [List1Key, MapList]]) :-
	(is_list(List1);is_list(List2)),!,
	atom_concat('$',List1Key,List1Val),
	atom_concat('$',List2Key,List2Val),
	unify_list(List1Val, List2Val, MapList).

set_term_arguments(_Term1, _Term2, Term1Key, Term2Key, Set) :-
	set_term_arguments(Term1Key, Term2Key, Set).

%%
set_term_arguments(Term1Key, Term2Key,
		['$set', [Term1Key, ['$cond', [
			['if', ['$not', array([string(Term1Args0)])]],
			['then', string(Term1Value)],
			['else', [
				['type', string('compound')],
				['value', [
					['functor', string(FunctorValue)],
					['args', MapList]
				]]
			]]
		]]]]) :-
	atom_concat('$',Term1Key,Term1Value),
	atom_concat(Term1Value,'.value.functor',FunctorValue),
	atomic_list_concat(['$',Term1Key,'.value.args'],Term1Args0),
	atomic_list_concat(['$',Term2Key,'.value.args'],Term2Args0),
	unify_list(Term1Args0, Term2Args0, MapList).

%%
unify_list(List1Val, List2Val, MapList) :-
	MapList=['$map', [
		['input', string(List1Val)],
		['in', ['$cond', [
			% if not a variable
			['if', ['$ne', array([string('$$this.type'), string('var')])]],
			% then use $$this
			['then', string('$$this')],
			% else use argument of other term.
			% Note: afaik there is no built-in way to access the index of the mapped element.
			%    Here we use $indexOfArray to find the index of the element in List1 in List2, but this is
			%    not very efficient, and could cause problems when the list holds multiple identical elements.
			%    Another option would be to use $zip to combine the list with $range list, but not so sure about
			%    if possible
			['else', ['$arrayElemAt', array([
				string(List2Val),
				['$indexOfArray', array([string(List1Val),string('$$this')])]
			])]]
		]]]
	]].

%%
% =/2 builds up two fields 't_term1' and 't_term2'.
% at this point both of them are equal.
% here we analyze the compile-time argument of the predicate
% and project variables with new groundings as separate fields
% in the document.
%
set_term_vars(Term, _, _, _) :-
	% no projection needed if term is already ground
	ground(Term),!,
	fail.

set_term_vars(Term, Field, Ctx, ['$set', [TermField, string(FieldValue)]]) :-
	% the term itself is a var -> $set var field
	var(Term),!,
	mongolog:var_key(Term, Ctx, TermField),
	atom_concat('$', Field, FieldValue).

set_term_vars(Args, Field, Ctx, SetVars) :-
	is_list(Args),!,
	atomic_list_concat(['$',Field], ArrayField),
	set_term_vars1(Args, ArrayField, Ctx, SetVars).

set_term_vars(Term, Field, Ctx, SetVars) :-
	% nonvar(Term),
	Term =.. [_Functor|Args],
	atomic_list_concat(['$',Field,'.value.args'], ArrayField),
	set_term_vars1(Args, ArrayField, Ctx, SetVars).

%%
set_term_vars1(Args, ArrayField, Ctx, ['$set', [ArgField,
		['$arrayElemAt', array([string(ArrayField),integer(Index)])]]]) :-
	% iterate over arguments
	length(Args, NumArgs),
	NumArgs0 is NumArgs - 1,
	between(0, NumArgs0, Index),
	nth0(Index, Args, Arg),
	% get the varkey or fail if it is not a var
	mongolog:var_key(Arg, Ctx, ArgField).

		 /*******************************
		 *    	  UNIT TESTING     		*
		 *******************************/

:- begin_tests('mongolog_unification').

test('atom unification'):-
	assert_true(mongolog:test_call(=(X,a), X, a)),
	assert_true(mongolog:test_call(=(a,X), X, a)),
	assert_true(mongolog:test_call(=(_,X), X, a)),
	assert_false(mongolog:test_call(=(b,X), X, a)),
	assert_false(mongolog:test_call(=(a(b),X), X, a)).

test('number unification'):-
	assert_true(mongolog:test_call(=(X,7), X, 7)),
	assert_true(mongolog:test_call(=(7,X), X, 7)),
	assert_true(mongolog:test_call(=(_,X), X, 7)),
	assert_false(mongolog:test_call(=(8,X), X, 7)),
	assert_false(mongolog:test_call(=(a(7),X), X, 7)).

test('compound unification'):-
	assert_true(mongolog:test_call(=(X,foo(a,b)), X, foo(a,b))),
	assert_true(mongolog:test_call(=(foo(a,b),X), X, foo(a,b))),
	assert_true(mongolog:test_call(=(foo(a,_),X), X, foo(a,b))),
	assert_true(mongolog:test_call(=(_,X), X, foo(a,b))),
	assert_false(mongolog:test_call(=(foo(a,c),X), X, foo(a,b))).

test('list unification'):-
	assert_true(mongolog:test_call(=(X,[a]), X, [a])),
	assert_true(mongolog:test_call(=(X,[a,b]), X, [a,b])),
	assert_true(mongolog:test_call(=(X,[_,_]), X, [a,b])),
	assert_true(mongolog:test_call(=(X,[a,_]), X, [a,b])),
	(	mongolog:test_call(=(X,[A,B]), X, [a,b])
	->	assert_equals([A,B],[a,b])
	;	true
	),
	(	mongolog:test_call(=(X,Y), X, [a,b])
	->	assert_equals(Y,[a,b])
	;	true
	).

test('compound+list unification'):-
	assert_true(mongolog:test_call(=(X,foo([a1,b1])), X, foo([a1,b1]))),
	assert_true(mongolog:test_call(=(foo([a2,b2]),X), X, foo([a2,b2]))),
	assert_true(mongolog:test_call(=(_,X), X, foo([a4,b4]))),
	assert_false(mongolog:test_call(=(foo([a5,c5]),X), X, foo([a5,b5]))),
	(	mongolog:test_call(=(X,foo([A,B])), X, foo([a6,b6]))
	->	assert_equals([A,B],[a6,b6])
	;	true
	).

test('compound+list partially grounded',
		fixme('unification of lists does not work if some list elements are variables')):-
	assert_true(mongolog:test_call(=(foo([a3,_]),X), X, foo([a3,b3]))).

test('unification 1-ary term with var'):-
	mongolog:test_call(=(foo(Y),X), X, foo(a)),
	assert_equals(Y,a).

test('unification 2-ary term with var'):-
	mongolog:test_call(=(foo(a,Y),X), X, foo(a,b)),
	assert_equals(Y,b).

test('unified vars are equivalent'):-
	mongolog:test_call(=(X,Y), X, _),
	assert_equals(X,Y).

test('unified vars can be implicitly instantiated',
		fixme('all equivalent variables must be instantiated together')):-
	mongolog:test_call((=(X,Y), X is 4), X, _),
	assert_equals(X,4),
	assert_equals(Y,4).

test('variables may appear multiple times in terms'):-
	mongolog:test_call(=(f(g(X),X),f(Y,Z)), Z, a),
	assert_equals(X,a),
	assert_equals(Y,g(a)).

:- end_tests('mongolog_unification').

