/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <gtest/gtest.h>
#include "knowrob/formulas/DependencyGraph.h"
#include "knowrob/queries/QueryParser.h"

using namespace knowrob;

static inline bool has_intersection(const std::set<std::string_view> &a, const std::set<std::string_view> &b)
{
    const std::set<std::string_view> &smaller = (a.size()<b.size() ? a : b);
    const std::set<std::string_view> &larger = ((&smaller == &a) ? b : a);
    return std::any_of(smaller.cbegin(), smaller.cend(),
                       [larger](auto &v){ return larger.count(v)>0; });
}

void DependencyGraph::operator+=(const DependencyNodePtr &node)
{
    insert(node);
}

void DependencyGraph::insert(const std::vector<FirstOrderLiteralPtr> &literals)
{
    for(auto &l : literals)
        insert(std::make_shared<DependencyNode>(l));
}

void DependencyGraph::insert(const FirstOrderLiteralPtr &literal)
{
    insert(std::make_shared<DependencyNode>(literal));
}

void DependencyGraph::insert(const DependencyNodePtr &newNode)
{
    nodes_.push_back(newNode);

    // find the modal node groups that have a shared variable with newNode.
    std::list<std::list<DependencyGroup>::iterator > dependencies;
    for(auto it=groups_.begin(); it!=groups_.end(); ++it) {
        if(has_intersection(it->variables_, newNode->variables())) {
            dependencies.push_back(it);
        }
    }

    DependencyGroup *newNodeGroup = nullptr;
    if(dependencies.empty()) {
        // the literal does not have a shared variable with an existing group.
        // create a new one with just the literal as a member.
        auto &newGroup = groups_.emplace_back();
        newNodeGroup = &newGroup;
    }
    else if(dependencies.size()==1) {
        // add to group
        auto &group = *dependencies.front();
        newNodeGroup = &group;
    }
    else {
        // merge multiple groups into one
        auto &newGroup = groups_.emplace_back();
        for(auto groupIterator : dependencies) {
            newGroup += *groupIterator;
            groups_.erase(groupIterator);
        }
        newNodeGroup = &newGroup;
    }
    // update neighbor relation based on shared variables with other nodes
    for(auto &x : newNodeGroup->member_) {
        if(has_intersection(x->variables(), newNode->variables())) {
            x->neighbors_.push_back(newNode);
            newNode->neighbors_.push_back(x);
        }
    }
    // finally add the new node to the group
    newNodeGroup->member_.push_back(newNode);
    newNodeGroup->variables_.insert(newNode->variables().begin(), newNode->variables().end());
}


DependencyNode::DependencyNode(const FirstOrderLiteralPtr &literal)
        : literal_(literal)
{
}

void DependencyNode::addDependency(const std::shared_ptr<DependencyNode> &other)
{
    neighbors_.push_back(other);
}
