/*
 * Copyright (c) 2023, Daniel Beßler
 * All rights reserved.
 *
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <gtest/gtest.h>

#include "knowrob/queries/DependencyGraph.h"
#include "knowrob/queries/QueryParser.h"
#include "knowrob/modalities/KnowledgeModality.h"

using namespace knowrob;

static inline bool has_intersection(const VariableSet &a, const VariableSet &b)
{
    // TODO: is there a more standard way of doing this?
    const VariableSet &smaller = (a.size()<b.size() ? a : b);
    const VariableSet &larger = ((&smaller == &a) ? b : a);
    return std::any_of(smaller.cbegin(), smaller.cend(),
                       [larger](auto *v){ return larger.count(v)>0; });
}

void DependencyGraph::operator+=(const DependencyNodePtr &node)
{
    insert(node);
}

void DependencyGraph::insert(const std::list<LiteralPtr> &literals, const ModalityLabelPtr &label)
{
    insert(std::make_shared<ModalDependencyNode>(literals, label));
}

void DependencyGraph::insert(const LiteralPtr &literal)
{
    insert(std::make_shared<LiteralDependencyNode>(literal));
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



LiteralDependencyNode::LiteralDependencyNode(const LiteralPtr &literal)
        : DependencyNode(),
          literal_(literal)
{
}

ModalDependencyNode::ModalDependencyNode(
        const std::list<LiteralPtr> &literals, const ModalityLabelPtr &label)
        : DependencyNode(),
          literals_(literals),
          label_(label)
{
    for(auto &literal : literals) {
        auto &literalVariables = literal->predicate()->getVariables();
        // remember variables that appear in the literal
        variables_.insert(literalVariables.begin(), literalVariables.end());
    }
}


// fixture class for testing
class DependencyGraphTest : public ::testing::Test {
protected:
    LiteralPtr p_, q_, r_, s_;
    ModalityLabelPtr m1_, m2_;
    void SetUp() override {
        p_ = std::make_shared<Literal>(QueryParser::parsePredicate("p(a,X)"), false);
        q_ = std::make_shared<Literal>(QueryParser::parsePredicate("q(X,Y)"), false);
        r_ = std::make_shared<Literal>(QueryParser::parsePredicate("r(Y,z)"), false);
        s_ = std::make_shared<Literal>(QueryParser::parsePredicate("s(b,a)"), false);

        auto x = std::make_shared<ModalIteration>();
        *x += KnowledgeModality::K();
        m2_ = std::make_shared<ModalityLabel>(x);
    }
    void TearDown() override {}
};

TEST_F(DependencyGraphTest, SingleLiteral)
{
    DependencyGraph dg;
    dg.insert({p_}, m1_);
    ASSERT_EQ(dg.numNodes(),1);
    ASSERT_EQ(dg.numGroups(),1);
}

TEST_F(DependencyGraphTest, DependantLiterals)
{
    DependencyGraph dg;
    dg.insert({p_, q_}, m2_);
    ASSERT_EQ(dg.numNodes(),1);
    ASSERT_EQ(dg.numGroups(),1);
}

TEST_F(DependencyGraphTest, IndependantLiterals)
{
    DependencyGraph dg;
    dg.insert({p_, r_}, m1_);
    ASSERT_EQ(dg.numNodes(),1);
    ASSERT_EQ(dg.numGroups(),1);
}

TEST_F(DependencyGraphTest, ChainAndOne)
{
    DependencyGraph dg;
    dg.insert({p_, q_, r_, s_}, m1_);
    ASSERT_EQ(dg.numNodes(),1);
    ASSERT_EQ(dg.numGroups(),1);
}

TEST_F(DependencyGraphTest, MultiModalDependant)
{
    DependencyGraph dg;
    dg.insert({p_, r_, s_}, m1_);
    dg.insert({q_}, m2_);
    ASSERT_EQ(dg.numNodes(),2);
    ASSERT_EQ(dg.numGroups(),1);
}

TEST_F(DependencyGraphTest, MultiModalIndependant)
{
    DependencyGraph dg;
    dg.insert({p_, r_, q_}, m1_);
    dg.insert({s_}, m2_);
    ASSERT_EQ(dg.numNodes(),2);
    ASSERT_EQ(dg.numGroups(),2);
}
