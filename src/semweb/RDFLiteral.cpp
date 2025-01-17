//
// Created by daniel on 07.04.23.
//

#include <utility>

#include "knowrob/semweb/RDFLiteral.h"
#include "knowrob/terms/Constant.h"
#include "knowrob/Logger.h"
#include "knowrob/queries/QueryError.h"
#include "knowrob/modalities/BeliefModality.h"

using namespace knowrob;

static auto TRIPLE_INDICATOR =
        std::make_shared<PredicateIndicator>("triple",3);

RDFLiteral::RDFLiteral(
            const TermPtr &s,
            const TermPtr &p,
            const TermPtr &o,
            bool isNegated,
            const ModalityLabelPtr &label)
: Literal(getRDFPredicate(s,p,o), isNegated, label),
  subjectTerm_(s),
  propertyTerm_(p),
  objectTerm_(o),
  objectOperator_(EQ)
{
    if(label) {
        auto epistemicOperator = label->epistemicOperator();
        if(epistemicOperator) {
            auto epistemicModality = (const EpistemicModality*) &epistemicOperator->modality();
            auto o_agent = epistemicModality->agent();
            if(o_agent.has_value()) {
                agentTerm_ = std::make_shared<StringTerm>(o_agent.value());
            }
            if(epistemicOperator->isModalPossibility()) {
                auto beliefModality = (const BeliefModality*) epistemicModality;
                if(beliefModality->confidence().has_value()) {
                    confidenceTerm_ = std::make_shared<DoubleTerm>(beliefModality->confidence().value());
                }
            }
        }

        auto pastOperator = label->pastOperator();
        if(pastOperator) {
            auto pastModality = (const PastModality*) &pastOperator->modality();
            if(pastModality) {
                if(pastModality->timeInterval().has_value()) {
                    auto &ti = pastModality->timeInterval().value();
                    if(ti.since().has_value()) {
                        beginTerm_ = std::make_shared<DoubleTerm>(ti.since().value().value());
                    }
                    if(ti.until().has_value()) {
                        endTerm_ = std::make_shared<DoubleTerm>(ti.until().value().value());
                    }
                }
            }
            else {
                KB_WARN("unexpected temporal operator in graph query!");
            }
        }
    }
}

RDFLiteral::RDFLiteral(const StatementData &data)
: Literal(getRDFPredicate(data), false, std::make_shared<ModalityLabel>(data)),
  subjectTerm_(predicate_->arguments()[0]),
  propertyTerm_(predicate_->arguments()[1]),
  objectTerm_(predicate_->arguments()[2]),
  objectOperator_(EQ)
{
    switch(data.objectType) {
        case RDF_RESOURCE:
        case RDF_STRING_LITERAL:
            objectTerm_ = std::make_shared<StringTerm>(data.object);
            break;
        case RDF_DOUBLE_LITERAL:
            objectTerm_ = std::make_shared<DoubleTerm>(data.objectDouble);
            break;
        case RDF_BOOLEAN_LITERAL:
            objectTerm_ = std::make_shared<LongTerm>(data.objectInteger);
            break;
        case RDF_INT64_LITERAL:
            objectTerm_ = std::make_shared<Integer32Term>(data.objectInteger);
            break;
    }
    if(data.confidence.has_value()) {
        confidenceTerm_ = std::make_shared<DoubleTerm>(data.confidence.value());
    }
    if(data.begin.has_value()) {
        beginTerm_ = std::make_shared<DoubleTerm>(data.begin.value());
    }
    if(data.end.has_value()) {
        endTerm_ = std::make_shared<DoubleTerm>(data.end.value());
    }
    if(data.graph) {
        graphTerm_ = getGraphTerm(data.graph);
    }
    if(data.agent) {
        agentTerm_ = std::make_shared<StringTerm>(data.agent);
    }
}

RDFLiteral::RDFLiteral(const RDFLiteral &other, const Substitution &sub)
: Literal(other, sub),
  subjectTerm_(predicate_->arguments()[0]),
  propertyTerm_(predicate_->arguments()[1]),
  objectTerm_(predicate_->arguments()[2]),
  graphTerm_(other.graphTerm_),
  agentTerm_(other.agentTerm_),
  confidenceTerm_(other.confidenceTerm_),
  beginTerm_(other.beginTerm_),
  endTerm_(other.endTerm_),
  objectOperator_(EQ)
{
    // todo: substitute other variables of RDFLiteral too!
}

std::shared_ptr<RDFLiteral> RDFLiteral::fromLiteral(const LiteralPtr &literal)
{
    if(literal->arity()!=2) {
        throw QueryError("RDF literal can only be constructed from 2-ary predicates but {} is not.", *literal);
    }
    auto s = literal->predicate()->arguments()[0];
    // TODO: a copy of the name is held by Vocabulary class
    //  memory could be mapped into StringTerm but StringTerm does not support it yet.
    auto p = std::make_shared<StringTerm>(literal->functor());
    auto o = literal->predicate()->arguments()[1];
    return std::make_shared<RDFLiteral>(s,p,o,literal->isNegated(),literal->label());
}

std::shared_ptr<Term> RDFLiteral::getGraphTerm(const std::string_view &graphName)
{
    static std::map<std::string, TermPtr, std::less<>> graphTerms;
    if(!graphName.empty()) {
        auto it = graphTerms.find(graphName);
        if(it == graphTerms.end()) {
            auto graphTerm = std::make_shared<StringTerm>(graphName.data());
            graphTerms[graphName.data()] = graphTerm;
            return graphTerm;
        }
        else {
            return it->second;
        }
    }
    return {};
}

std::shared_ptr<Predicate> RDFLiteral::getRDFPredicate(const TermPtr &s, const TermPtr &p, const TermPtr &o)
{
    return std::make_shared<Predicate>(TRIPLE_INDICATOR, std::vector<TermPtr>({s,p,o}));
}

std::shared_ptr<Predicate> RDFLiteral::getRDFPredicate(const StatementData &data)
{
    TermPtr s,p,o;
    s = std::make_shared<StringTerm>(data.subject);
    p = std::make_shared<StringTerm>(data.predicate);
    switch(data.objectType) {
        case RDF_RESOURCE:
        case RDF_STRING_LITERAL:
            o = std::make_shared<StringTerm>(data.object);
            break;
        case RDF_DOUBLE_LITERAL:
            o = std::make_shared<DoubleTerm>(data.objectDouble);
            break;
        case RDF_BOOLEAN_LITERAL:
            o = std::make_shared<LongTerm>(data.objectInteger);
            break;
        case RDF_INT64_LITERAL:
            o = std::make_shared<Integer32Term>(data.objectInteger);
            break;
    }
    return std::make_shared<Predicate>(TRIPLE_INDICATOR, std::vector<TermPtr>({s,p,o}));
}

std::shared_ptr<Term> RDFLiteral::subjectTerm() const
{
    return subjectTerm_;
}

std::shared_ptr<Term> RDFLiteral::objectTerm() const
{
    return objectTerm_;
}

RDFLiteral::OperatorType RDFLiteral::objectOperator() const
{
    return objectOperator_;
}

std::shared_ptr<Term> RDFLiteral::propertyTerm() const
{
    return propertyTerm_;
}

std::shared_ptr<Term> RDFLiteral::graphTerm() const
{
    return graphTerm_;
}

void RDFLiteral::setGraphName(const std::string_view &graphName)
{
    graphTerm_ = getGraphTerm(graphName);
}

std::shared_ptr<Term> RDFLiteral::agentTerm() const
{
    return agentTerm_;
}

std::shared_ptr<Term> RDFLiteral::confidenceTerm() const
{
    return confidenceTerm_;
}

std::shared_ptr<Term> RDFLiteral::beginTerm() const
{
    return beginTerm_;
}

std::shared_ptr<Term> RDFLiteral::endTerm() const
{
    return endTerm_;
}

uint32_t RDFLiteral::numVariables() const
{
    int varCounter=0;
    for(auto &t : {
        subjectTerm_,
        propertyTerm_,
        objectTerm_,
        graphTerm_,
        agentTerm_,
        beginTerm_,
        endTerm_,
        confidenceTerm_ })
    {
        if(t && t->type() == TermType::VARIABLE) varCounter+=1;
    }
    return varCounter;
}

static inline const char* readStringConstant(const TermPtr &term)
{ return std::static_pointer_cast<StringTerm>(term)->value().c_str(); }

static inline double readDoubleConstant(const TermPtr &term)
{ return std::static_pointer_cast<DoubleTerm>(term)->value(); }

StatementData RDFLiteral::toStatementData() const
{
    StatementData data;
    if(numVariables()>0) {
        throw QueryError("Only ground literals can be mapped to StatementData, but "
                         "the literal '{}' has variables.", *this);
    }
    if(isNegated()) {
        throw QueryError("Only positive literals can be mapped to StatementData, but "
                         "the literal '{}' is negative.", *this);
    }
    data.subject = readStringConstant(subjectTerm_);
    data.predicate = readStringConstant(propertyTerm_);
    switch(objectTerm_->type()) {
        case TermType::STRING:
            data.object = ((StringTerm*)objectTerm_.get())->value().c_str();
            // TODO: RDF_RESOURCE or RDF_STRING_LITERAL?
            data.objectType = RDF_RESOURCE;
            break;
        case TermType::DOUBLE:
            data.objectDouble = ((DoubleTerm*)objectTerm_.get())->value();
            data.objectType = RDF_DOUBLE_LITERAL;
            break;
        case TermType::INT32:
            data.objectInteger = ((Integer32Term*)objectTerm_.get())->value();
            data.objectType = RDF_INT64_LITERAL;
            break;
        case TermType::LONG:
            data.objectInteger = ((LongTerm*)objectTerm_.get())->value();
            data.objectType = RDF_INT64_LITERAL;
            break;
        case TermType::VARIABLE:
            KB_WARN("Literal has a variable as an argument in '{}' which is not allowed.", *this);
            break;
        case TermType::PREDICATE:
            KB_WARN("Literal has a predicate as an argument in '{}' which is not allowed.", *this);
            break;
        case TermType::LIST:
            KB_WARN("Literal has a list as an argument in '{}' which is not allowed.", *this);
            break;
    }
    if(graphTerm_) data.graph = readStringConstant(graphTerm_);

    // handle epistemic modality
    if(label_->epistemicOperator()) {
        if(label_->epistemicOperator()->isModalPossibility()) {
            data.epistemicOperator = EpistemicOperator::BELIEF;
        }
    }
    if(agentTerm_) {
        data.agent = readStringConstant(agentTerm_);
    }
    if(confidenceTerm_) {
        data.confidence = readDoubleConstant(confidenceTerm_);
    }

    // handle temporal modality
    data.temporalOperator = TemporalOperator::ALWAYS;
    if(label_->pastOperator()) {
        if(label_->pastOperator()->isModalPossibility()) {
            data.temporalOperator = TemporalOperator::SOMETIMES;
        }
    }
    if(beginTerm_) {
        data.begin = readDoubleConstant(beginTerm_);
    }
    if(endTerm_) {
        data.end = readDoubleConstant(endTerm_);
    }

    return data;
}
