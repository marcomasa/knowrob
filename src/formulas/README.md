\page formulas Formulas

A formula in KnowRob is an expression involving one or more predicates that are connected
via an operator, and which evaluates to either true or false. Formulas are used to represent
knowledge about the world, and can be used to query the knowledge base for information.

The base class for formulas is `Formula`, which is an abstract class that cannot be instantiated.
The `Formula` class provides a number of methods that are common to all formulas, such as
equality testing, and classification of a formula into sub-types.
The list of sub-types of formulas is as follows:

- `Atom`: a single n-ary predicate.
- `Conjunction`: the conjunction of two or more formulas.
- `Disjunction`: the disjunction of two or more formulas.
- `Negation`: the negation of a formula.
- `Implication`: an implication between two formulas.
- `ModalFormula`: a modal operator applied to a formula.

In addition, two null-ary predicates `Top` and `Bottom` are provided, which represent the
universal truth and the universal falsehood, respectively.

### Modal Formulas

A `ModalFormula` is a formula prefixed by a modal operator.
An example of a modal formula is `K p`, which states that the formula `p` is known to be true.
The modal operator
modifies the semantics of the formula in some way. The following modal operators are
supported:

- `K`: the formula is known to be true.
- `B`: the formula is believed to be true.
- `P`: the formula is occasionally true.
- `H`: the formula is always true.

The operators further support a number of parameters that can be used to specify the
context in which the formula is true. For example, the `B` operator can be used with
a parameter to specify that the formula is known to be true from the perspective of
a particular agent, or with a particular confidence. In query strings, parameters
of modal operators are specified in square brackets, e.g. `B[confidence=0.8] p` which
is equivalent to `B[0.8] p`.

### Syntax for Formulas

KnowRob provides a parser for formulas that can be used to parse formulas from strings
(see `QueryParser` class).
In addition, formulas can be constructed programmatically using the provided class constructors,
and some C++ operators dedicated to formula construction. The following operators are defined:

- `p & q`: conjunction of formulas `p` and `q`.
- `p | q`: disjunction of formulas `p` and `q`.
- `~p`: negation of formula `p`.

In each case `p` and `q` are smart pointers to formulas (i.e. `FormulaPtr` or `std::shared_ptr<Formula>`)
to avoid unneeded copies.
