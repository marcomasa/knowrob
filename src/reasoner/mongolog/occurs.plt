:- use_module(library('mongolog/mongolog_test')).
:- use_module(library('mongolog/temporal')).

:- begin_mongolog_tests(mongolog_occurs, 'owl/test/events.owl').

:- use_module(library('semweb'), [sw_register_prefix/2]).
:- use_module(library('mongolog/mongolog')).
:- use_module('occurs').

:- sw_register_prefix(test, 'http://knowrob.org/kb/test_events.owl#').

test('during(occurs(+),+Interval)') :-
	assert_true(during(occurs(test:'Short4'), [1377777009, 1377777011])),
	assert_true(during(occurs(test:'Short4'), [1377777010, 1377777011])),
	assert_true(during(occurs(test:'Short4'), [1377777001, 1377777009])),
	assert_true(during(occurs(test:'Short4'), [1377777011, 1377777019])),
	assert_false(during(occurs(test:'Short4'), [1377777012, 1377777019])),
	assert_false(during(occurs(test:'Short4'), [1377777001, 1377777004])).

test('during(occurs(+),+Event)') :-
	assert_true(during(occurs(test:'Short1'), test:'Short1')),
	assert_true(during(occurs(test:'Long'), test:'Short1')),
	assert_true(during(occurs(test:'Long'), test:'Short3')),
	assert_true(during(occurs(test:'Long'), test:'Short4')),
	assert_false(during(occurs(test:'Short1'), test:'Short3')),
	assert_false(during(occurs(test:'Short1'), test:'Short4')).

:- end_mongolog_tests(mongolog_occurs).
