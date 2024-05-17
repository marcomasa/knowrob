:- module(mongolog_comparison, []).
/** <module> Comparison of arbitrary terms in mongolog programs.

The following predicates are supported:

| Predicate             | Arguments |
| ---                   | ---       |
| ==/2                  | @Term1, @Term2 |
| \==/2                 | @Term1, @Term2 |

@author Daniel Beßler
@see https://www.swi-prolog.org/pldoc/man?section=standardorder
@license BSD
*/

:- use_module('mongolog').

%% mongolog:add_command
:- mongolog:add_command(==,2).
:- mongolog:add_command(\==,2).

%% @Term1 == @Term2
% True if Term1 is equivalent to Term2. A variable is only identical to a sharing variable
%
mongolog:step_compile(==(X,Y), _, []) :-
	ground([X,Y]),!,
	X == Y.

mongolog:step_compile(==(X,Y), Ctx,
		[['$match', ['$expr', ['$eq', array([X0,Y0])]]]]) :-
	mongolog:var_key_or_val(X,Ctx,X0),
	mongolog:var_key_or_val(Y,Ctx,Y0).

%% @Term1 \== @Term2
% Equivalent to \+Term1 == Term2.
%
mongolog:step_compile(\==(X,Y), _, []) :-
	ground([X,Y]),!,
	X \== Y.

mongolog:step_compile(\==(X,Y), Ctx,
		[['$match', ['$expr', ['$ne', array([X0,Y0])]]]]) :-
	mongolog:var_key_or_val(X,Ctx,X0),
	mongolog:var_key_or_val(Y,Ctx,Y0).

		 /*******************************
		 *    	  UNIT TESTING     		*
		 *******************************/

:- begin_tests('mongolog_comparison').

test('==(+Atom,+Term)'):-
	assert_true(mongolog:test_call(
		(Atom == '4.5'), Atom, '4.5')),
	assert_true(mongolog:test_call(
		(Atom == 'foo'), Atom, 'foo')),
	assert_false(mongolog:test_call(
		(Atom == 'foo'), Atom, '4.5')).

test('==(+Number,+Term)'):-
	assert_true(mongolog:test_call(
		(X == 4.5), X, 4.5)),
	assert_false(mongolog:test_call(
		(X == 4.5), X, '4.5')).
		
test('==(+Term1,+Term2)'):-
	assert_true(mongolog:test_call(
		(X == foo(2)), X, foo(2))),
	assert_true(mongolog:test_call(
		(X == foo(bar,2)), X, foo(bar,2))),
	assert_true(mongolog:test_call(
		(X == foo(Y)), X, foo(Y))),
	assert_false(mongolog:test_call(
		(X == foo(3)), X, foo(2))).

test('\\==(+Term1,+Term2)'):-
	assert_false(mongolog:test_call(
		(Atom \== 'foo'), Atom, 'foo')),
	assert_false(mongolog:test_call(
		(Atom \== 4.5), Atom, 4.5)),
	assert_true(mongolog:test_call(
		(Atom \== 'foo'), Atom, '4.5')),
	assert_true(mongolog:test_call(
		(Atom \== 4.5), Atom, '4.5')).

:- end_tests('mongolog_comparison').
