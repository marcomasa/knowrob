:- module(mongolog_meta, []).
/** <module> Meta predicates in mongolog programs.

The following predicates are supported:

| Predicate    | Arguments |
| ---          | ---       |
| call/1       | :Goal |
| once/1       | :Goal |
| ignore/1     | :Goal |
| limit/2      | +Count, :Goal |

@author Daniel Beßler
@see https://www.swi-prolog.org/pldoc/man?section=metacall
@license BSD
*/

:- use_module('client', [ mng_strip/4 ]).
:- use_module('mongolog').
:- use_module('scope', [ mongolog_resolve_scope/3 ]).

%%%% query commands
:- mongolog:add_command(call,1).
:- mongolog:add_command(call,2).
:- mongolog:add_command(call,3).
:- mongolog:add_command(call,4).
:- mongolog:add_command(call,5).
:- mongolog:add_command(once,1).
:- mongolog:add_command(limit,2).
:- mongolog:add_command(ignore,1).
:- mongolog:add_command(call_with_args,2).
:- mongolog:add_command(call_with_context,2).
:- mongolog:add_command(context,1).

%%%% query expansion
	

%% once(:Goal)
% Make a possibly nondet goal semidet, i.e., succeed at most once.
%
mongolog:step_expand(once(Goal), Expanded) :-
	append_cut(Goal, WithCut),
	mongolog_expand(WithCut, Expanded).

%% ignore(:Goal)
% Calls Goal as once/1, but succeeds, regardless of whether Goal succeeded or not.
%
mongolog:step_expand(ignore(Goal), Expanded) :-
	append_cut(Goal, WithCut),
	mongolog_expand((WithCut ; true), Expanded).

%%
mongolog:step_expand(
		limit(Count, Goal),
		limit(Count, Expanded)) :-
	mongolog_expand(Goal, Expanded).

%%
mongolog:step_expand(call(Goal), call(Expanded)) :-
	mongolog_expand(Goal, Expanded).

mongolog:step_expand(call(Goal,Arg1),
		call_with_args(Expanded,[Arg1])) :-
	mongolog_expand(Goal, Expanded).

mongolog:step_expand(call(Goal,Arg1,Arg2),
		call_with_args(Expanded,[Arg1,Arg2])) :-
	mongolog_expand(Goal, Expanded).

mongolog:step_expand(call(Goal,Arg1,Arg2,Arg3),
		call_with_args(Expanded,[Arg1,Arg2,Arg3])) :-
	mongolog_expand(Goal, Expanded).

mongolog:step_expand(call(Goal,Arg1,Arg2,Arg3,Arg4),
		call_with_args(Expanded,[Arg1,Arg2,Arg3,Arg4])) :-
	mongolog_expand(Goal, Expanded).

%%
ensure_list([X|Xs],[X|Xs]) :- !.
ensure_list(X,[X]).

%% limit(+Count, :Goal)
% Limit the number of solutions.
% True if Goal is true, returning at most Count solutions.
%
mongolog:step_compile(
		limit(_, Terminals), _Ctx, [], []) :-
	Terminals==[],
	!.

mongolog:step_compile(
		limit(Count, Terminals),
		Ctx, Pipeline, StepVars) :-
	mongolog:var_key_or_val(Count,Ctx,Count0),
	% appended to inner pipeline of lookup
	Suffix=[['$limit',Count0]],
	% create a lookup and append $limit to inner pipeline,
	% then unwind next and assign variables to the toplevel document.
	lookup_next_unwind(Terminals, Suffix, Ctx, Pipeline, StepVars0),
	%
	(	mongolog:goal_var(Count,Ctx,Count_var)
	->	StepVars=[Count_var|StepVars0]
	;	StepVars=StepVars0
	).

%% call(:Goal)
% Call Goal. This predicate is normally used for goals that are not known at compile time.
%
mongolog:step_compile(
		call(Terminals), Ctx, Pipeline, StepVars) :-
	lookup_next_unwind(Terminals, [], Ctx, Pipeline, StepVars).

%% call_with_args(:Goal,:Args)
% Call Goal. This predicate is normally used for goals that are not known at compile time.
%
mongolog:step_compile(
		call_with_args(Term0,Args), Ctx, Pipeline, StepVars) :-
	Term0 =.. Buf0,
	append(Buf0, Args, Buf1),
	Term1 =.. Buf1,
	mongolog:step_compile(call(Term1), Ctx, Pipeline, StepVars).

%%
mongolog:step_expand(
		call_with_context(Goal,Args),
		call_with_context(Expanded,Args)) :-
	mongolog_expand(Goal, Expanded).

%%
%
mongolog:step_compile(
		call_with_context(Terminals, NewCtx),
		OldCtx, Pipeline, StepVars) :-
	option(outer_vars(V0), OldCtx),
	% resolve since/until values.
	% this is needed if the values are grounded within the query
	% by mongo DB and not provided in the call context.
	mongolog_resolve_scope(NewCtx, OldCtx, NewCtx0),
	% add provided options to context
	merge_options(NewCtx0, OldCtx, Ctx),
	% finally compile call goal with new context
	mongolog:compile_terms(
		Terminals, Pipeline,
		V0->_, StepVars, Ctx).

%%
% context(-Option) and context(-Option, +Default) are used to read
% options from compile context to make them accessible in rules.
% The main usecase is that some temporal predicates need to access
% the query scope.
%
mongolog:step_compile(context(Option), Ctx, []) :-
	option(Option, Ctx).

mongolog:step_compile(context(Option, Default), Ctx, []) :-
	option(Option, Ctx, Default).

%%
lookup_next_unwind(Terminals, Suffix, Ctx, Pipeline, StepVars) :-
	mongolog:lookup_array('next', Terminals, [], Suffix, Ctx, StepVars, Lookup),
	findall(Step,
		% generate steps
		(	Step=Lookup
		% unwind "next" field
		;	Step=['$unwind',string('$next')]
		% set variables from "next" field
		;	mongolog:set_next_vars(StepVars, Step)
		% remove "next" field again
		;	Step=['$unset',string('next')]
		),
		Pipeline),
	% the inner goal is not satisfiable if Pipeline==[]
	Lookup \== [].
	

%%
% append cut operator at the end of a goal.
%
append_cut(Goal, WithCut) :-
	(	is_list(Goal) -> Terms=Goal
	;	comma_list(Goal,Terms)
	),
	append(Terms, ['!'], X),
	comma_list(WithCut, X).

		 /*******************************
		 *    	  UNIT TESTING     		*
		 *******************************/

:- begin_tests('mongolog_meta').

test('limit(1, +Goal)'):-
	mongolog:test_call(
		limit(1, (
			(X is (Num + 5))
		;	(X is (Num * 2))
		)),
		Num, 4.5),
	assert_equals(X,9.5).

test('limit(2, +Goal)'):-
	findall(X,
		mongolog:test_call(
			limit(2, (
				(X is (Num + 5))
			;	(X is (Num * 2))
			;	(X is (Num * 2))
			)),
			Num, 4.5),
		Results
	),
	assert_unifies(Results,[_,_]),
	assert_true(ground(Results)),
	assert_true(memberchk(9.5, Results)),
	assert_true(memberchk(9.0, Results)).

test('once(+Goal)'):-
	mongolog:test_call(
		once((
			(X is (Num + 5))
		;	(X is (Num * 2))
		)),
		Num, 4.5),
	assert_equals(X,9.5).

test('ignore(+Failing)'):-
	mongolog:test_call(
		ignore(((Num < 3), (X is (Num * 2)))),
		Num, 4.5),
	assert_unifies(X,_).

test('ignore(+Failing), +Goal'):-
	mongolog:test_call(
		(ignore(Num < 3), (X is (Num * 2))),
		Num, 4.5),
	assert_equals(X,9.0).

test('ignore(+FailingWithVar), +Goal'):-
	% tests with variable Z being assigned only in failing ignored
	% goal. Then no grounding will be part of result set and Z still a variable
	% after the call.
	mongolog:test_call(
		(	ignore((Num < 3, Z is Num + 2)),
			X is (Num * 2)
		),
		Num, 4.5),
	assert_equals(X,9.0),
	assert_unifies(Z,_).

test('call(+Goal)'):-
	mongolog:test_call(
		call(Y is X), X, -3.25),
	assert_equals(Y, -3.25).

test('call(+Functor, -Arg1, +Arg2)'):-
	mongolog:test_call(
		call(is, Arg1, Arg2), Arg2, -3.25),
	assert_equals(Arg1, -3.25).

:- end_tests('mongolog_meta').
