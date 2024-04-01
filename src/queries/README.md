\page querying Querying

A `Query` is evaluated to find out whether some statement
is true or false. If the statement has variables, then the query is true
if there is a substitution of the variables that makes the corresponding statement true.
Queries can be constructed using the `Formula` class, and are translated into query
pipelines internally. The result of an evaluation is an answer, which in the case of
KnowRob, can refer to a positive answer (i.e. "yes"), a negative answer (i.e. "no"),
or a neutral answer (i.e. "unknown"). Additionally, answers can be marked as "uncertain"
to allow a conflicting but certain answer to overrule the uncertain one.

### Query Pipelines

KnowRob represents query pipelines as a form of communication graph where `Tokens`
are passed between nodes.
The construction and evaluation of such pipelines is encapsulated in the `QueryPipeline`
class.
Each node in the graph is a query processor that reacts
on input tokens by producing output tokens. The graph is constructed by recursively
translating a query into a set of query stages. The query stages are
implemented as classes that inherit from the `TokenStream` class.
For example, the `TokenBroadcaster` class takes each input token and sends it to
all of its output streams.

Each token is either an `Answer` or a `ControlToken`.
An `Answer` indicates the result of a query, and can be positive, negative, or neutral.
A `ControlToken` is used to control the flow of the query pipeline.
Currently, only the `EndOfEvaluation` control token is used to indicate that the
evaluation of a query has finished.
This is needed as each stage can produce multiple output tokens for each input token,
so it must indicate when it has finished producing output tokens.

There is a special syntax available to create query pipelines using the C++
stream operator `>>`. The expression `s1 >> s2` creates a link between two query stages where
`s1` must be a broadcast stream (i.e. one with dedicated outputs), and `s2` is a stage that
follows `s1` (i.e. it receives outputs of `s1`as input).

### Query Answers

The `Answer` class is used to represent the result of a query evaluation.
It can be positive (`AnswerYes`), negative (`AnswerNo`), or neutral (`AnswerDontKnow`).
Answers can be well-founded or not. A well-founded answer is one that is based on
certain knowledge only, while a non-well founded answer is (at least partially) based
on assumptions.
For example, a negative answer can be well-founded if a sub-system can prove that
the answer cannot be positive.

### Query Parsing

The `QueryParser` class provides a simple interface to parse queries and terms.
Below table lists some examples:

| Query              | Description                                    |
|--------------------|------------------------------------------------|
| `p(?x,a)`          | Binary predicate with a variable.              |
| `p(a,b)`           | Binary predicate with constants.               |
| `p(a,b) & q(b,c)`  | Conjunction of two predicates.                 |
| `p(a,b) \| q(b,c)` | Disjunction of two predicates.                 |
| `~(p&q)`           | Negated conjunction.                           |
| `p(a,b) -> q(b,c)` | Implication.                                   |
| `(p\|q)&r`         | A query involving conjunction and disjunction. |
| `Bp`               | Modal query that includes uncertain knowledge. |
| `B (p,q)`          | Modal operator with more complex argument.     |

