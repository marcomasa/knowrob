:- module(kb_messages, []).

% a failure during term expansion. Usually caused
% by referring to a predicate that is not known (yet).
prolog:message(lang(expansion_failed(Failed),_Context)) -->
	[ '[prolog] query expansion failed at `~w`.'-[Failed] ].

prolog:message(lang(_Failed, compilation_failed(Term, _Context))) -->
	[ '[prolog] Query compilation failed at command `~w`.'-[Term] ].

prolog:message(lang(Failed, assertion_failed(Body))) -->
	[ '[prolog] Query assertion of rule `~w` failed for clause `~w`.'-[Failed,Body] ].

% OWL file has been loaded before
prolog:message(db(ontology_detected(Ontology,Version))) -->
	[ '[prolog] detected "~w" ontology version ~w.'-[Ontology,Version] ].

% OWL file has been loaded
prolog:message(db(ontology_loaded(Ontology,Version))) -->
	[ '[prolog] loaded "~w" ontology version ~w.'-[Ontology,Version] ].

prolog:message(db(read_only(Predicate))) -->
	[ '[prolog] Predicate `~w` tried to write despite read only access.'-[Predicate] ].
