KnowRob
=======

![CI](https://github.com/knowrob/knowrob/workflows/CI/badge.svg)


[KnowRob](http://www.knowrob.org) is a knowledge processing system designed for robots.
Its purpose is to equip robots with the capability to organize information in re-usable
knowledge chunks, and to perform reasoning in an expressive logic.
It further provides a set of tools for visualization and acquisition of knowledge,
aswell as a ROS interface for easy integration into custom projects.

## Outline

1. [Functionality]()
2. [Dependencies]()
3. [Install Instructions]()
4. [Building Knowrob]()
5. [Starting Knowrob]()
6. [Using Knowrob]()


## Functionality

Originally, KnowRob was implemented using the Prolog programming language.
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

For further information and details about how Knowrob functions, you can refer to the following pages:

- [Terms](src/terms/README.md)
- [Formulas](src/formulas/README.md)
- [Triples](src/triples/README.md)
- [Ontologies](src/ontologies/README.md)
- [Querying](src/queries/README.md)
- [Backends](src/storage/README.md)
- [Reasoner](src/reasoner/README.md)

## Dependencies

### Required

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

### Optional

Some features will only be conditionally compiled if the following dependencies are found:

- [ROS1](https://www.ros.org/) >= Melodic, ROS2 is not supported yet

### Install instructions

For more help during the installation of all dependencies, please refer to [these instructions](src/docs/install_instrutions.md).


## Building Knowrob

### Standalone Package

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

### ROS Package

To build knowrob, you will need to add this repository to your catkin workspace.
Then proceed by executing

```bash
catkin build knowrob
```

If you encounter errors regarding SWI Prolog or MongoDB versions,
redo the install process and try again.
Remember to 

```bash
catkin clean knowrob
```

If the package still does not build, try modifying the CMakeList.txt slightly, save and build again.
Your build process might still be compromised by old errors and this restarts the build from scratch.


## Starting Knowrob

Being a shared library, KnowRob cannot be launched directly but only in the context
of a program that uses it. We currently provide a standalone terminal application and a ROS1 node.


### Terminal Application

The terminal application allows the user to interact with KnowRob using a command line interface.


#### Launch command

To start the TA, use this command:

```
knowrob-terminal --config-file ~/knowrob/settings/default.json
```

The configuration file is a required argument, there is no fallback configuration file.
If you installed Knowrob anywhere else than under ~/knowrob,
please adjust your filepath in the command above.

#### Expected output

Once the terminal is up and running, you should see a greeting message and a prompt
which looks like this:

```
Welcome to KnowRob.
For online help and background, visit http://knowrob.org/

?- 
```

### ROS Node

#### Launch command
To launch the basic example, type

```bash
 roslaunch knowrob knowrob.launch
 ```

#### Expected Output

You should see something like

```bash
# ROS Startup
...

setting /run_id to 1fa113aa-887c-11ee-8552-00155d76860c
process[rosout-1]: started with pid [19352]
started core service [/rosout]
process[knowrob_ros-2]: started with pid [19355]
[15:41:59.227] [info] Using knowledge graph `mongodb` with type `MongoDB`.
[15:41:59.620] [info] dropping graph with name "user".
[15:41:59.621] [info] Using reasoner `mongolog` with type `Mongolog`.
[15:41:59.663] [info] common foreign Prolog modules have been registered.
## The following will only appear on the first startup or when u delete the mongodb base
[15:42:00.285] [info] Loading ontology at '/home/your_user/catkin_ws/src/knowrob/owl/rdf-schema.xml' with version "Mon Nov 20 11:56:29 2023" into graph "rdf-schema".
[15:42:00.304] [info] Loading ontology at '/home/your_user/catkin_ws/src/knowrob/owl/owl.rdf' with version "Mon Nov 20 11:56:54 2023" into graph "owl".
[15:42:00.329] [info] Loading ontology at '/home/your_user/catkin_ws/src/knowrob/owl/test/swrl.owl' with version "Mon Nov 20 11:56:54 2023" into graph "swrl".
 ```

### Troubleshooting

If your output looks more like this,
the mongoDB server is not running.

```bash
[15:44:50.789] [info] Using knowledge graph `mongodb` with type `MongoDB`.
[15:45:20.791] [error] failed to load a knowledgeGraph: mng_error(create_index_failed,No suitable servers found: `serverSelectionTimeoutMS` expired: [connection refused calling ismaster on 'localhost:27017'])
[15:45:20.791] [info] Using reasoner `mongolog` with type `Mongolog`.
[15:45:20.791] [error] failed to load a reasoner: Reasoner `mongolog` refers to unknown data-backend `mongodb`.
 ```


On native linux systems, you can enable the service using systemd:

```bash
# Try to run the mongodb service
sudo systemctl start mongod.service
# Check if the service is running properly
sudo systemctl status mongod.service
```

On a WSL2 machine you will need to manually launch a mongoDB server as .systemd does not exist in WSL2.
To launch a seperate mongoDB process,  execute the following commands in a new terminal 
(taken from [here](https://github.com/michaeltreat/Windows-Subsystem-For-Linux-Setup-Guide/blob/master/readmes/installs/MongoDB.md)):

 ```bash
# (once initially) create a directory for the mongoDB data
sudo mkdir -p ~/data/db

# launch mongoDB server with the created path as argument
sudo mongod --dbpath ~/data/db
 ```

If you now re-launch [this example](#launch-command),
you should see the [expected output](https://github.com/marcomasa/knowrob#expected-output).


## Using Knowrob

### Configuring your knowledge base

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


### Using the terminal application

Please refer to the [CLI](src/queries/README.md) for documentation about the syntax of queries
that can be typed into the terminal. Limited auto-completion is available. `exit/0` will
terminate the terminal.

### Using the ROS interface

#### Provided Action interfaces

Once knowrob is launched, it provides four actions to ask queries:
- askone
- askincremental
- askall
- tell

#### Client Interface libraries

For easier integration into your own project, we provide [client libraries](src/ros_client/README.md)
that you can use in your own C++ or Python application.


## Further Information

More documentation can be found in the following pages:

- [Python Integration](src/integration/python/README.md)
- [ROS Integration](src/integration/ros1/README.md)

In addition, the following resources are available:
- [API Documentation](https://knowrob.github.io/knowrob/)
- A blog and more wiki pages are available at [knowrob.org](http://www.knowrob.org)
