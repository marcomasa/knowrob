
% this is a global initialization for Prolog-based
% reasoning in KnowRob. Several reasoner instances
% are able to access what is imported here.

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

:- dynamic user:defined_reasoner_setting/4.

:- use_module(library('ext/filesystem')).
:- use_module(library('ext/functional')).
:- use_module(library('ext/algebra')).

% extensions for semantic web
:- use_module(library('semweb')).
:- use_module(library('semweb/rdf_db'),
		[ rdf_assert/4,
		  rdf_retractall/4,
		  rdf_transaction/1 ]).
% interface for PrologReasoner implementations
:- use_module(library('reasoner')).

:- sw_register_prefix(dul,  'http://www.ontologydesignpatterns.org/ont/dul/DUL.owl#').
:- sw_register_prefix(soma, 'http://www.ease-crc.org/ont/SOMA.owl#').
:- sw_register_prefix(knowrob, 'http://knowrob.org/kb/knowrob.owl#').

% auto-load common models
:- use_module(library('ext/xsd')).
:- use_module(library('ext/qudt')).
