:- dynamic user:defined_reasoner_setting/4.

% extensions for semantic web
:- use_module(library('semweb')).
:- use_module(library('semweb/rdf_db'),
		[ rdf_assert/4,
		  rdf_retractall/4,
		  rdf_transaction/1 ]).
% interface for PrologReasoner implementations
:- use_module(library('reasoner')).
