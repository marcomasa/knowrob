% configure how Prolog prints terms
:- set_prolog_flag(toplevel_print_anon, false).
:- set_prolog_flag(toplevel_print_options,
        [ quoted(true),
          portray(true),
          max_depth(0),
          attributes(portray)
        ]).
:- set_prolog_flag(float_format, '%.12g').

% make sure all messages are printed, even if tests succeed.
% else also error messages are suppressed which makes it really hard to debug.
:- plunit:set_test_options([output(always)]).

% load common Prolog libraries
:- use_module(library('semweb/rdf_db'), [rdf_meta/1, rdf_current_ns/2, rdf_register_prefix/3]).

% more fancy module declarations
:- use_module(library('ext/module')).
% message formatting and logging
:- use_module(library('messages')).
:- use_module(library('logging')).
% tooling around plunit
:- use_module(library('unittest')).

:- use_module(library('ext/filesystem')).
:- use_module(library('ext/functional')).
:- use_module(library('ext/algebra')).

% auto-load common models
:- use_module(library('ext/xsd')).
:- use_module(library('ext/qudt')).
