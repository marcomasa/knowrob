:- module(mongolog_test,
	[ begin_mongolog_tests/3,
	  begin_mongolog_tests/2,
	  end_mongolog_tests/1
	]).
/** <module> A test environment for running tests with RDF data.
A RDF file can be loaded in the set-up step, and all assertions
written into a special named graph that is deleted again in cleanup step.

Example:
```
:- begin_mongolog_tests('my_module', 'foo.rdf').

test('some test') :- fail.

:- end_tests('my_module').
```

@author Daniel Beßler
@license BSD
*/

:- use_module('triple',
	[ load_owl/1,
	  drop_graph/1
	]).
:- use_module(library('semweb'),
	[ sw_get_subgraphs/2,
	  sw_set_default_graph/1,
	  sw_url_graph/2
	]).

%% begin_mongolog_tests(+Name, +RDFFile, +Options) is det.
%
% Begin unit testing code section with RDF data.
% Load the RDF data during setup, and unload all asserted facts
% and RDF file on cleanup.
% Calls internally begin_tests/2.
%
%
begin_mongolog_tests(Name,RDFFile,Options0) :-
	(	select_option(namespace(URI_Prefix),Options0,Options1)
	->	true
	;	(	atom_concat(RDFFile,'#',URI_Prefix),
			Options1=Options0
		)
	),
	%%
	Setup   = mongolog_test:setup(RDFFile),
	Cleanup = mongolog_test:cleanup(RDFFile),
	add_option_goal(Options1, setup(Setup), Options2),
	add_option_goal(Options2, cleanup(Cleanup), Options3),
	%%
	begin_tests(Name,Options3),
	rdf_db:rdf_register_prefix(test,URI_Prefix,[force(true)]).

%% begin_mongolog_tests(+Name, +RDFFile) is det.
%
% Same as begin_rdf_tests/3 with empty options list.
%
begin_mongolog_tests(Name,RDFFile) :-
	begin_mongolog_tests(Name,RDFFile,[]).

%% end_mongolog_tests(+Name) is det.
%
% End unit testing code section with RDF data.
%
end_mongolog_tests(Name) :-
	end_tests(Name).

%%
setup(RDFFile) :-
	sw_set_default_graph(test),
	load_owl(RDFFile,[graph(test)]).

%%
cleanup(RDFFile) :-
	cleanup,
	sw_url_graph(RDFFile, OntoGraph),
	mongolog_triple:drop_graph(OntoGraph).

%%
cleanup :-
	sw_get_subgraphs(test,Subs),
	forall(
		member(string(Sub),Subs),
		drop_graph(Sub)
	),
	sw_set_default_graph(user).

%%
add_option_goal(OptionsIn,NewOpt,[MergedOpt|Rest]) :-
	NewOpt =.. [Key,NewGoal],
	OldOpt =.. [Key,OtherGoal],
	MergedOpt =.. [Key,Goal],
	(	select_option(OldOpt,OptionsIn,Rest)
	->	( Goal=(','(OtherGoal,NewGoal)) )
	;	( Goal=NewGoal, Rest=OptionsIn )
	).
