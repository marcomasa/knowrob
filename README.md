KnowRob
=======

![CI](https://github.com/knowrob/knowrob/workflows/CI/badge.svg)

The purpose of KnowRob is to equip robots with explicit knowledge about the world.
Originally, it was implemented using the Prolog programming language.
In its second iteration, KnowRob is implemented in C++, but still supports Prolog
for rule-based reasoning.

The core of KnowRob is a shared library that implements a *hybrid* knowledge base.
With *hybrid*, we mean that different reasoning engines can be combined in
KowRob's query evaluation process. To this end, KnowRob defines a querying language
and manages which parts of a query are evaluated by which reasoner or storage backend.
Both reasoners and storage backends are configurable, and can be extended by plugins
either in written in C++ or Python.
There are a few applications shipped with this repository including a terminal application
that allows to interact with KnowRob using a command line interface, and a ROS node
that exposes KnowRob's querying interface to the ROS ecosystem.

## Getting Started

These instructions will get you a copy of KnowRob up and running on your local machine.

### Dependencies

The following list of software is required to build KnowRob:

- [SWI Prolog](https://www.swi-prolog.org/) >= 8.2.4
- [mongo DB server](https://www.mongodb.com/de-de) >= 4.4 and libmongoc
- [Redland and Raptor2](https://librdf.org)
- [spdlog](https://github.com/gabime/spdlog.git)
- [fmt](https://github.com/fmtlib/fmt)
- [Eigen 3](https://eigen.tuxfamily.org/index.php?title=Main_Page)
- [Boost](https://www.boost.org/) with the following components:
  - python
  - program options
  - serialization
- [GTest](https://github.com/google/googletest)

#### Optional Dependencies

Some features will only be conditionally compiled if the following dependencies are found:

- [ROS1](https://www.ros.org/) >= Melodic, ROS2 is not supported yet

### Installation

KnowRob uses *CMake* as build system. The following steps will guide you through the installation process.
Assuming you have cloned the repository to `~/knowrob`:

```Bash
cd ~/knowrob
mkdir build
cd build
cmake ..
make
sudo make install
```

You may further need to set the *SWI_HOME_DIR* environment variable to the installation location of *swipl*:

```
export SWI_HOME_DIR=/usr/lib/swi-prolog
```

Alternatively, you may clone the KnowRob repository into a ROS workspace and build it using *catkin*.
Please refer to the [ROS](src/ros/README.md) documentation for further information.

### Development

Any IDE with proper CMake and C++ language support should be able to load the project.
For example, you can use *CLion* or *Visual Studio Code*.
But support for Prolog code is usually quite limited or not existent.

### Configuration

KnowRob uses a configuration file to set up the knowledge base.
Internally, boost's property tree is used to parse the configuration file.
Hence, JSON is one of the supported formats.
The configuration file specifies the storage backends,
the reasoner, and the ontologies that are loaded into the knowledge base.

An example configuration file is provided in `settings/default.json`:

```json
  "data-sources": [
    {
      "path": "owl/test/swrl.owl",
      "language": "owl",
      "format": "xml"
    }
  ],
  "data-backends": [
    {
      "type": "MongoDB",
      "name": "mongodb",
      "host": "localhost",
      "port": 27017,
      "db": "mongolog1",
      "read-only": false
    }
  ],
  "reasoner": [
    {
      "type": "Mongolog",
      "name": "mongolog",
      "data-backend": "mongodb"
    }
  ]
```

For more information about storage backends, please refer to the [Backends](src/storage/README.md) documentation,
and for more information about reasoners, please refer to the [Reasoner](src/reasoner/README.md) documentation.

### Launching

Being a shared library, KnowRob cannot be launched directly but only in the context
of a program that uses it.

One such program is the terminal application that allows to interact with KnowRob
using a command line interface.
It can be launched as follows:

```
knowrob-terminal --config-file ~/knowrob/settings/default.json
```

The configuration file is a required argument, there is no fallback configuration file.

Once the terminal is up and running, you should see a greeting message and a prompt
which looks like this:

```
Welcome to KnowRob.
For online help and background, visit http://knowrob.org/

?- 
```

Please refer to the [CLI](src/queries/README.md) for documentation about the syntax of queries
that can be typed into the terminal. Limited auto-completion is available. `exit/0` will
terminate the terminal.

Alternatively, you can expose the KnowRob querying interface as a ROS service.
Please refer to the [ROS](src/ros/README.md) documentation for further information.

## Getting Familiar

Here we provide an overview about functionality of KnowRob.

### Querying

The core of KnowRob is a querying interface that is built around a custom querying language.
Its syntax is similar to Prolog, but it is more simplified and not Turing-complete like Prolog is.

For more information on querying in KnowRob, please have a look
[here](src/queries/README.md).

### Ontologies

KnowRob structures knowledge using ontologies. Ontologies are formal models of a domain that are
used to describe the concepts in the domain and the relationships between them.
In KnowRob, ontologies are generally represented as RDF graphs using the RDFS and OWL vocabularies.

Ontologies are organized in a hierarchy where each ontology is a specialization of another ontology.
A common distinction is made between foundational (or top-level) ontologies, domain ontologies, 
and application ontologies.
A foundational ontology fixes the basic concepts and relationships that are used
across different domains.
In KnowRob, we define a domain ontology for the robotics domain, and align it with a
common foundational ontology.
Applications can then import the domain ontology and extend it with application-specific concepts
to cover the specific requirements of the application.

For more information on ontologies in KnowRob, please have a look
[here](src/ontologies/README.md).

### Triple Store and Data Access

Knowledge is represented in form of contextualized triples --
each subject-predicate-object triple has additional fields
that contextualize the triple.
A configurable storage backend is used to store and retrieve triples --
currently, triple stores based on Prolog, MongoDB and Redland are supported.

One important aspect in knowledge representation for robots is that
a lot of knowledge is *implicitly* encoded in the control structures
of the robot. Hence, one goal is to make this implicit knowledge *explicit*.
This is done by mapping data to symbols in an ontology.
In case of querying, this is often referred to as *Ontology-based Data Access* (OBDA).

For more information on storages in KnowRob, please have a look
[here](src/storage/README.md).

### Reasoning

KnowRob uses an ensemble of reasoners approach where inferences
of different reasoners are combined into correlated knowledge pieces.
The reason for choosing this approach is that there is no single
formalism that is suited for every reasoning tasks.
Instead, given a problem, one should decide what the most suitable
formalism is to tackle it.
Consequently, KnowRob can be configured to solve specific problems
by loading corresponding reasoning modules that implement a common interface.

For more information on reasoning in KnowRob, please have a look
[here](src/reasoner/README.md).

## Further Information

More documentation can be found in the following pages:

- [Terms](src/terms/README.md)
- [Formulas](src/formulas/README.md)
- [Triples](src/triples/README.md)
- [Ontologies](src/ontologies/README.md)
- [Querying](src/queries/README.md)
- [Backends](src/storage/README.md)
- [Reasoner](src/reasoner/README.md)
- [Python Integration](src/integration/python/README.md)
- [ROS Integration](src/integration/ros1/README.md)

In addition, the following resources are available:
- [API Documentation](https://knowrob.github.io/knowrob/)
- A blog and more wiki pages are available at [knowrob.org](http://www.knowrob.org)
