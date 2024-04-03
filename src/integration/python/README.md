
One of the problematic case for the mapping occurs when a C++ class has multiple
methods with the same name. Python can only distinguish between them by their
number of arguments. Hence, such methods must be renamed in the Python.
The convention is that if such a conflict occurs, then the method names in
Python are prefixed with the C++ method name, and a semantic string distinguishing
the variants, e.g. `insertAll` becomes `insertAllFromList` indicating it can be called
with a Python list as an argument.
