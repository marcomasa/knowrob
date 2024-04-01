\page triples Triples

Triples are the basic building blocks of the RDF data model.
A triple is a statement that consists of three parts: a subject, a predicate, and an object.
The subject is the resource being described, the predicate is the property of the resource,
and the object is the value of the property.
For example, the triple `ex:John ex:hasAge "25"^^xsd:integer` states that the resource
`ex:John` has the age of `25`.
A collection of triples forms an RDF graph, which is a set of subject-predicate-object triples.

While the term "triple" is widely used, most of the triple stores in fact support an additional
fourth element, usually the "graph" name or "origin" of the triple, which is used to distinguish
between different named graphs. In such a case, the triple is actually a quadruple and often
referred to as a "quad".

However, in KnowRob, we consider additional context parameters as part of the triple itself.
Such a triple with context parameters is referred to as `FramedTriple` within KnowRob.
This is done to cope with uncertain statements, and statements that are only valid in a certain context (or frame).
Nevertheless, KnowRob can make use of any quad store by internally *reifying* the contextualized triple
into a set of triples without context parameters (see `StorageInterface`).

### Context Parameters of Triples

The following table lists the context parameters that can be used in a `FramedTriple`:

| Parameter     | Description                                                                  |
|---------------|------------------------------------------------------------------------------|
| `graph`       | The graph or origin in which this triple is loaded  (default: user).         |
| `perspective` | The perspective from which this triple is valid (default: ego).              |
| `uncertain`   | A flag indicating if this triple is uncertain (default: false).              |
| `occasional`  | A flag indicating if this triple is only occasionally true (default: false). |
| `confidence`  | The (normalized) confidence that this triple is true (default: 1.0).         |
| `begin`       | The begin time of the validity of this triple (default: -inf).               |
| `end`         | The end time of the validity of this triple (default: +inf).                 |

Each of these parameters is optional in queries.
Some additional comments on the context parameters:
- A triple can be asserted into multiple origins. 
- The "perspective" parameter can be e.g. the IRI of another agent.
The default case is the "ego perspective" which is the perspective of the agent that runs the knowledge base.

### Triple Patterns

A triple pattern is a triple with some parts replaced by variables.
For example, the triple pattern `?x ex:hasAge ?y` matches all triples where the subject is a variable `?x`
and the predicate is `ex:hasAge`.
Triple patterns are used in queries to match triples in the knowledge base.
The query engine then binds the variables in the triple pattern to the actual values in the triples.

In KnowRob, triple patterns are represented by the `TriplePattern` class.
It uses terms to represent the subject, predicate, object, and context parameters of the triple.
Each of the terms can either be a variable or a constant term.
In addition, a few comparison filters can be specified to restrict matching triples.
Once a triple pattern is created, it can be used to query the knowledge base for matching triples.
Given the `Bindings` obtained from the query, the triple pattern can be instantiated to a `FramedTriple`.

### Graph Queries

More complex graph queries can be formed over triple patterns by introducing additional operators.
Triple stores usually support connective operators `UNION` and `PATH` to combine multiple triple patterns.
In KnowRob, the `GraphQuery` class is used to represent such more complex graph queries.
It has the subtypes `GraphUnion` and `GraphSequence` to represent the `UNION` and `PATH` operators, respectively.
In addition, the `GraphBuiltin` class is used to represent built-in operators that are not part of the RDF standard.
The builtin operators that must be supported by backends include the `BIND` operator for variable assignment and
the `FILTER` operator for filtering results. In addition, a `MAX` and `MIN` operator is needed that binds
a variable to the maximum or minimum value of a set of values, respectively.

The operators listed above are mainly required to support the context parameters of `FramedTriple` with
storage backends that cannot store them directly.
