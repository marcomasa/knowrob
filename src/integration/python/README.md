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
Please further note that the integration is at an *early stage*, and some important
features might still be missing. If you encounter one, please report it via the issue tracker.

### No 1-to-1 Mapping

There are a couple of specifics that require special attention.

One of them is that there are problems when Python classes implement
multiple C++ interfaces. This could be useful to define a `Reasoner` which
is also a `Storage`, but it is not supported at the moment.

Another problematic case for the mapping occurs when a C++ class has multiple
methods with the same name. Python can only distinguish between them by their
number of arguments. Hence, such methods must be renamed in the Python code.
The convention is that if such a conflict occurs, then the method names in
Python are prefixed with the C++ method name, and a semantic string distinguishing
the variants, e.g. `insertAll` becomes `insertAllFromList` indicating it can be called
with a Python list as an argument.

Another limitation is that when overwriting a virtual method in Python, it seems
not possible to explicitly call the base class method.
However, as a workaround for most cases a wrapper class can be defined that also
calls the base class method.
