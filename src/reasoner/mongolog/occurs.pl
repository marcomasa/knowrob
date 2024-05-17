:- module(mongolog_occurs, [ occurs(r) ]).
/** <module> The occurs predicate.

@author Daniel Beßler
@license BSD
*/

:- use_module('SOMA').

%% occurs(?Event) is nondet.
%
% True for all occurences (events).
%
% @param Event an event instance.
%
occurs(Evt) ?>
	% query event interval
	call_with_context(
		event_interval(Evt, EventBegin, EventEnd),
		[query_scope(dict{ time: dict{
			min: dict{ min: double(0),          max: double(0) },
			max: dict{ min: double('Infinity'), max: double('Infinity') }
		}})]),
	% read time interval from compile context
	context(query_scope(QScope)),
	pragma(mongolog_time_scope(QScope, Since0, Until0)),
	pragma(mng_strip_operator(Since0,_,Since1)),
	pragma(mng_strip_operator(Until0,_,Until1)),
	% only succeed if time intervals intersect each other
	max(EventBegin,Since1) =< min(EventEnd,Until1).

%%
during(Query, Event) ?>
	atom(Event),
	pragma(time_scope(=<(Since), >=(Until), Scope)),
	event_interval(Event, Since, Until),
	call_with_context(Query, [query_scope(Scope)]).

since(Query, Event) ?>
	atom(Event),
	event_interval(Event, Time, _),
	call_with_context(Query, [query_scope(dict{
		time: dict{ min: dict{ max: Time } }
	})]).

until(Query, Event) ?>
	atom(Event),
	event_interval(Event, Time, _),
	call_with_context(Query, [query_scope(dict{
		time: dict{ max: dict{ min: Time } }
	})]).
