\page python Python Integration

KnowRob integrates with the Python language through the boost::python library.
The way it works is that the C++ classes are wrapped in Python classes
using the `boost::python` library.
The build process then creates a module file called `knowrob.so` that can be imported
in Python using e.g. `import knowrob`.

There are two main intended use cases:
1. Python applications that internally manage a `KnowledgeBase` object.
2. KnowRob plugins for `Storage` and `Reasoner` that are written in Python: 
  you can write a Python class that inherits from `Storage` or `Reasoner`
  and implement the virtual methods.

Currently, only the C++ classes which are needed for these use-cases are available
in Python, i.e. the classes themselves and the types that appear in their public
interfaces.

### No 1-to-1 Mapping

One of the problematic case for the mapping occurs when a C++ class has multiple
methods with the same name. Python can only distinguish between them by their
number of arguments. Hence, such methods must be renamed in the Python.
The convention is that if such a conflict occurs, then the method names in
Python are prefixed with the C++ method name, and a semantic string distinguishing
the variants, e.g. `insertAll` becomes `insertAllFromList` indicating it can be called
with a Python list as an argument.
