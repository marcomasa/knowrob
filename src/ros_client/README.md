KnowRob Client Library
=======

# Using the Knowrob ROS Client Library

## Provided interfaces by the Knowrob node

Once knowrob is launched, it provides four actions to ask queries:
- askone
- askincremental
- askall
- tell

## Client Interface libraries

We provide both a C++ (and Python) library for you to include in your own project.
They implement the action client interface to access knowrob data.

### C++ Interface

To include the library in your own project, just add 'knowrob' to your CMakeLists.txt.
Remember to also include the 'target_link_libraries' tag for your executable.

```bash
find_package(catkin REQUIRED COMPONENTS
roscpp
...
knowrob)
```

Then you should be able to create a KnowrobClient instance and use the provided functions:

```cpp
// include the library
#include "knowrob/ros_client/cpp/knowrob_client.hpp"

// create the KnowrobClient instance
ros::Nodehandle nh;
KnowrobClient m_kc;
kc.initialize(nh);  

// prepare a query
KnowrobQuery single_query = kc.getDefaultQueryMessage();
single_query.queryString = std::string("test:hasSon(A,B)");

// send and get the answer
std::vector<KnowrobAnswer> multi_answer;
bool com_success = kc.askAll(single_query, multi_answer);

// inspect the answer
...
```

To retrieve the answer from knowrob, you should use the type tag to identify 
the correct value field. See this example implementation from the KnowrobClient:

```cpp
std::string KnowrobClient::getAnswerText(const KnowrobAnswer& answer) const
{
    std::stringstream ss;

    int i = 0;
    for(auto& elem : answer.substitution)
    {
        ss << "[" << i << "]" << "\t" 
           << "[Key]=" << elem.key << "\t"
           << "[Value]=";

        switch(elem.type)
        {
            case knowrob::KeyValuePair::TYPE_STRING:
            {
                ss << elem.value_string;
            }
            break;
            case knowrob::KeyValuePair::TYPE_FLOAT:
            {
                ss << elem.value_float;
            }
            break;
            case knowrob::KeyValuePair::TYPE_INT:
            {
                ss << elem.value_int;
            }
            break;
            case knowrob::KeyValuePair::TYPE_LONG:
            {
                ss << elem.value_long;
            }
            break;
            case knowrob::KeyValuePair::TYPE_VARIABLE:
            {
                ss << elem.value_variable;
            }
            break;
            case knowrob::KeyValuePair::TYPE_PREDICATE:
            {
                ss << elem.value_predicate;
            }
            break;
            case knowrob::KeyValuePair::TYPE_LIST:
            {
                ss << elem.value_list;
            }
            break;    
            default:
                ROS_ERROR_STREAM(m_logger_prefix << "Unknown Answer Substitution Type!");
                return std::string("ERROR");
                break;
        }
    
        ss << std::endl;
    }
    
    return ss.str();
}

```

### Example Client Node

We also provide an [example node](src/ros_client/client_node.cpp) that asks and tells queries to knowrob.
This demonstrates how the client interface can and should be used.
With [knowrob launched](),
you should receive the following output when starting the client node.

```
user@machine: rosrun knowrob client-node 
[ INFO] [1701440166.477513145]: [KCN]   Starting the knowrob client node..
[ INFO] [1701440167.479320945]: [KR-RC] Verbose enabled!
[ INFO] [1701440167.479469534]: [KR-RC] Initializing client interfaces..
[ INFO] [1701440167.479821324]: [KR-RC] Query timeout is set to 0s
[ INFO] [1701440167.488575257]: [KR-RC] Waiting for actionserver to start.. Topic = knowrob/askone
[ INFO] [1701440167.739357113]: [KR-RC] Server is ready!
[ INFO] [1701440167.747524053]: [KR-RC] Waiting for actionserver to start.. Topic = knowrob/askall
[ INFO] [1701440168.032018335]: [KR-RC] Server is ready!
[ INFO] [1701440168.042977031]: [KR-RC] Waiting for actionserver to start.. Topic = knowrob/tell
[ INFO] [1701440168.316651395]: [KR-RC] Server is ready!
[ INFO] [1701440168.316746464]: [KR-RC] Init complete!
[ INFO] [1701440168.316788252]: [KCN]   Sending AskAll Query: test:hasSon(A,B)
[ INFO] [1701440168.318804522]: [KR-RC] Action finished!
[ WARN] [1701440168.318879392]: [KR-RC] Ask All - Answers vector is empty!
[ERROR] [1701440168.318955555]: [KCN]   AskAll false!
[ INFO] [1701440168.319020356]: [KCN]   Sending Tell Query: test:hasSon(a,b)
[ INFO] [1701440169.320373169]: [KR-RC] Action finished!
[ INFO] [1701440169.320440014]: [KCN]   Tell true!
[ INFO] [1701440169.320481001]: [KCN]   Sending AskAll Query: test:hasSon(A,B)
[ INFO] [1701440170.322107486]: [KR-RC] Action finished!
[ INFO] [1701440170.322165765]: [KCN]   AskAll true!
[ INFO] [1701440170.322221109]: [KCN]   Best Answer is: [0]     [Key]=A [Value]=a
[1]     [Key]=B [Value]=b
```

