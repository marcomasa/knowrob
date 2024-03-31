:- module(mongolog_temporal,
	[ during(t,r),
	  since(t,r),
	  until(t,r)
	]).
/** <module> Temporally scoped statements.

@author Daniel Beßler
@license BSD
*/

:- use_module(library('scope'), [ time_scope/3 ]).

:- op(800, yfx, user:during).
:- op(800, yfx, user:since).
:- op(800, yfx, user:until).

%% during(+Statement,?Interval) is nondet.
%
% True iff Statement holds during the whole
% duration of a time interval.
% during/2 is defined as an operator such that
% queries can be written as `Statement during Interval`.
% The Interval is represented as 2-element list `[Since,Until]`
% where Since and Until are the interval boundaries (unix timestamp, double).
% Note that it is currently not allowed to call this predicate
% with one of the boundaries grounded and the other not.
% Either both boundaries must be ground or both variables.
%
% @param Statement A language term.
% @param Interval A 2-element list.
%
during(Statement, [Since,Until]) ?>
	number(Since),
	number(Until),
	pragma(time_scope(=<(Since), >=(Until), Scope)),
	call_with_context(Statement, [query_scope(Scope)]).

during(Statement, [Since,Until]) ?>
	var(Since),
	var(Until),
	% Note: goal must be called with below scope to include all records.
	%       the default mode is to only include records true now.
	pragma(time_scope(
		>=(0),
		=<(double('Infinity')),
		Scope
	)),
	call_with_context(Statement, [query_scope(Scope)]),
	% read computed fact scope
	% FIXME: This is not entirely accurate as get('v_scope') yields the accumulated scope so far.
	%   but we only want the accumulated scope for Goal here.
	%   SOLUTION: do the get within the *call* same for since and until.
	assign(Since, string('$v_scope.time.since')),
	assign(Until, string('$v_scope.time.until')).

%% since(+Statement, ?Instant) is nondet.
%
% True for statements that hold (at least) since some time
% instant, _and_ until at least the current time.
% since/2 is defined as an operator such that
% queries can be written as `Statement since Instant`.
% Instant is a unix timestamp represented as floating point number.
%
% @param Statement A language term.
% @param Instant A time instant.
%
since(Statement, Instant) ?>
	number(Instant),
	% get current time
	pragma(get_time(Now)),
	% only include records that hold at least since
	% instant and at least until now
	pragma(time_scope(=<(Instant), >=(Now), Scope)),
	call_with_context(Statement, [query_scope(Scope)]).

since(Statement, Instant) ?>
	var(Instant),
	% get current time
	pragma(get_time(Now)),
	% only include records that still are thought to be true
	pragma(time_scope(>=(0), >=(Now), Scope)),
	call_with_context(Statement, [query_scope(Scope)]),
	% read computed fact scope
	assign(Instant, string('$v_scope.time.since')).

%% until(+Statement, ?Instant) is nondet.
%
% True for statements that hold (at least) until some time
% instant.
% until/2 is defined as an operator such that
% queries can be written as `Statement until Instant`.
% Instant is a unix timestamp represented as floating point number.
%
% @param Statement A language term.
% @param Interval A time interval, instant, or event.
%
until(Statement, Instant) ?>
	number(Instant),
	% only include records that hold at instant
	pragma(time_scope(=<(Instant), >=(Instant), Scope)),
	call_with_context(Statement, [query_scope(Scope)]).

until(Statement, Instant) ?>
	var(Instant),
	% include all records
	pragma(time_scope(
		>=(double(0)),
		=<(double('Infinity')),
		Scope
	)),
	call_with_context(Statement, [query_scope(Scope)]),
	assign(Instant, string('$v_scope.time.until')).

		 /*******************************
		 *	    UNIT TESTS	     		*
		 *******************************/

:- use_module(library('mongolog/mongolog_test')).
:- begin_mongolog_tests('mongolog_temporal','owl/test/swrl.owl').

:- sw_register_prefix(test, 'http://knowrob.org/kb/swrl_test#').

test('during(+Triple,+Interval)') :-
	assert_true(mongolog_assert(
		triple(test:'Lea', test:hasNumber, '+493455247'), dict{ since: 10, until: 34 })),
	assert_true(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+493455247') during [10,34])),
	assert_true(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+493455247') during [14,24])),
	assert_false(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+999999999') during [5,20])),
	assert_false(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+493455249') during [12,20])),
	assert_false(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+493455247') during [5,20])),
	assert_false(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+493455247') during [34,44])).

test('during(+Triple,+Overlapping)') :-
	% assert additional interval during which a statement holds that overlaps
	% with an existing interval
	assert_true(mongolog_assert(
		triple(test:'Lea', test:hasNumber, '+493455249'), dict{ since: 44, until: 84 })),
	assert_true(mongolog_assert(
		triple(test:'Lea', test:hasNumber, '+493455249'), dict{ since: 24, until: 54 })),
	assert_true(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+493455249') during [34,44])),
	assert_true(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+493455249') during [38,80])),
	assert_false(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+493455247') during [140,240])).

test('during(+Triple,[-Since,-Until])', [ blocked('variables in second argument of during') ]) :-
	assert_true(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+493455247') during [_,_])),
	(	mongolog_call(
			triple(test:'Lea', test:hasNumber, '+493455247') during [Since,Until])
	->	assert_equals([Since,Until], [10.0,34.0])
	;	true
	).

test('during(+Triple,-Interval)', [ blocked('variables in second argument of during') ]) :-
	assert_true(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+493455247') during _)),
	(	mongolog_call(
			triple(test:'Lea', test:hasNumber, '+493455247') during X)
	->	assert_equals(X,[10.0,34.0])
	;	true
	).

test('since(+Triple,+Instant)') :-
	assert_false(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+499955247') since 800)),
	assert_true(mongolog_assert(
		triple(test:'Lea', test:hasNumber, '+499955247'), dict{ since: 800 })),
	assert_true(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+499955247') since 800)),
	assert_true(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+499955247') since 1000)),
	assert_false(mongolog_call(
		triple(test:'Lea', test:hasNumber, '+499955247') since 600)).

test('until(+Triple,+Instant)') :-
	assert_false(mongolog_call(
		triple(test:'Fred', test:hasNumber, '+499955248') until 600)),
	assert_true(mongolog_assert(
		triple(test:'Fred', test:hasNumber, '+499955248'), dict{ until: 600 })),
	assert_true(mongolog_call(
		triple(test:'Fred', test:hasNumber, '+499955248') until 600)),
	assert_true(mongolog_call(
		triple(test:'Fred', test:hasNumber, '+499955248') until 100)),
	assert_false(mongolog_call(
		triple(test:'Fred', test:hasNumber, '+499955248') until 1000)).

:- end_mongolog_tests('mongolog_temporal').
