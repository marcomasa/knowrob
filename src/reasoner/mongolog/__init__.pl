
:- use_module(library('semweb/rdf_db'), [ rdf_meta/1 ]).
:- use_module(library('url')).

:- use_module(mongolog).
:- use_module(arithmetic).
:- use_module(atoms).
:- use_module(comparison).
:- use_module(control).
:- use_module(database).
:- use_module(findall).
:- use_module(fluents).
:- use_module(lists).
:- use_module(meta).
:- use_module(sgml).
:- use_module(terms).
:- use_module(typecheck).
:- use_module(unification).

:- use_module(scope).
:- use_module(annotation).
:- use_module(triple).
:- use_module(semweb).
:- use_module(holds).
:- use_module(temporal).

:- sw_register_prefix(dul,  'http://www.ontologydesignpatterns.org/ont/dul/DUL.owl#').
:- sw_register_prefix(soma, 'http://www.ease-crc.org/ont/SOMA.owl#').
:- sw_register_prefix(knowrob, 'http://knowrob.org/kb/knowrob.owl#').

:- use_module('DUL').
:- use_module('SOMA').
:- use_module(occurs).
