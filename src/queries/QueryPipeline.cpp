/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include "knowrob/queries/QueryPipeline.h"
#include "knowrob/triples/GraphPathQuery.h"
#include "knowrob/queries/DisjunctiveBroadcaster.h"
#include "knowrob/queries/RedundantAnswerFilter.h"
#include "knowrob/queries/QueryTree.h"
#include "knowrob/formulas/ModalFormula.h"
#include "knowrob/queries/ModalStage.h"
#include "knowrob/queries/NegationStage.h"
#include "knowrob/queries/ConjunctiveBroadcaster.h"
#include "knowrob/queries/QueryError.h"
#include "knowrob/KnowledgeBase.h"

using namespace knowrob;

namespace knowrob {
	/**
	 * Comparator for sorting EDB predicates.
	 */
	struct EDBComparator {
		explicit EDBComparator(VocabularyPtr vocabulary) : vocabulary_(std::move(vocabulary)) {}
		bool operator()(const FramedTriplePatternPtr &a, const FramedTriplePatternPtr &b) const;
		VocabularyPtr vocabulary_;
	};

	/**
	 * Comparator for sorting IDB predicates.
	 */
	struct IDBComparator {
		explicit IDBComparator(VocabularyPtr vocabulary) : vocabulary_(std::move(vocabulary)) {}
		bool operator()(const RDFComputablePtr &a, const RDFComputablePtr &b) const;
		VocabularyPtr vocabulary_;
	};
}

QueryPipeline::QueryPipeline(KnowledgeBase *kb, const FormulaPtr &phi, const QueryContextPtr &ctx) {
	auto outStream = std::make_shared<TokenBuffer>();
	addStage(outStream);

	// decompose input formula into parts that are considered in disjunction,
	// and thus can be evaluated in parallel.
	QueryTree qt(phi);
	for (auto &path: qt) {
		// each node in a path is either a predicate, a negated predicate,
		// a modal formula, or the negation of a modal formula.

		std::vector<FramedTriplePatternPtr> posLiterals, negLiterals;
		std::vector<std::shared_ptr<ModalFormula>> posModals, negModals;

		for (auto &node: path.nodes()) {
			switch (node->type()) {
				case FormulaType::PREDICATE: {
					auto pat = std::make_shared<FramedTriplePattern>(
							std::static_pointer_cast<Predicate>(node), false);
					pat->setTripleFrame(ctx->selector);
					posLiterals.push_back(pat);
					break;
				}

				case FormulaType::MODAL:
					posModals.push_back(std::static_pointer_cast<ModalFormula>(node));
					break;

				case FormulaType::NEGATION: {
					auto negation = (Negation *) node.get();
					auto negated = negation->negatedFormula();
					switch (negated->type()) {
						case FormulaType::PREDICATE: {
							auto pat = std::make_shared<FramedTriplePattern>(
									std::static_pointer_cast<Predicate>(negated), true);
							pat->setTripleFrame(ctx->selector);
							negLiterals.push_back(pat);
							break;
						}
						case FormulaType::MODAL:
							negModals.push_back(std::static_pointer_cast<ModalFormula>(negated));
							break;
						default:
							throw QueryError("Unexpected negated formula type {} in QueryTree.", (int) negated->type());
					}
					break;
				}
				default:
					throw QueryError("Unexpected formula type {} in QueryTree.", (int) node->type());
			}
		}

		std::shared_ptr<TokenBroadcaster> lastStage;
		std::shared_ptr<TokenBuffer> firstBuffer;

		// first evaluate positive literals if any
		// note that the first stage is buffered, so that the next stage can be added to the pipeline
		// and only after stopping the buffering messages will be forwarded to the next stage.
		if (posLiterals.empty()) {
			// if there are none, we still need to indicate begin and end of stream for the rest of the pipeline.
			// so we just push `bos` (an empty substitution) followed by `eos` and feed these messages to the next stage.
			firstBuffer = std::make_shared<TokenBuffer>();
			lastStage = firstBuffer;
			auto channel = TokenStream::Channel::create(lastStage);
			channel->push(GenericYes());
			channel->push(EndOfEvaluation::get());
			addStage(lastStage);
		} else {
			auto pathQuery = std::make_shared<GraphPathQuery>(posLiterals, ctx);

			auto subPipeline = std::make_shared<QueryPipeline>(kb, pathQuery);
			firstBuffer = std::make_shared<AnswerBuffer_WithReference>(subPipeline);
			*subPipeline >> firstBuffer;
			subPipeline->stopBuffering();

			lastStage = firstBuffer;
			addStage(lastStage);
		}

		// --------------------------------------
		// Evaluate all positive modals in sequence.
		// TODO: compute dependency between modals, evaluate independent modals in parallel.
		//       they could also be independent in evaluation context only, we could check which variables receive
		//       grounding already before in posLiteral query!
		//       this is effectively what is done in submitQuery(pathQuery)
		// --------------------------------------
		for (auto &posModal: posModals) {
			// TODO: improve interaction between kb and modal stage.
			//        use of this pointer not nice.
			//        maybe a better way would be having another class generating pipelines with the ability
			//        to create reference pointer on a KnowledgeBase, or use a weak ptr here
			auto modalStage = std::make_shared<ModalStage>(kb, posModal, ctx);
			modalStage->selfWeakRef_ = modalStage;
			lastStage >> modalStage;
			lastStage = modalStage;
			addStage(lastStage);
		}

		// --------------------------------------
		// Evaluate all negative literals in parallel.
		// --------------------------------------
		if (!negLiterals.empty()) {
			// run a dedicated stage where negated literals can be evaluated in parallel
			auto negLiteralStage = std::make_shared<PredicateNegationStage>(
					kb, ctx, negLiterals);
			lastStage >> negLiteralStage;
			lastStage = negLiteralStage;
			addStage(lastStage);
		}

		// --------------------------------------
		// Evaluate all negative modals in parallel.
		// --------------------------------------
		if (!negModals.empty()) {
			// run a dedicated stage where negated modals can be evaluated in parallel
			auto negModalStage = std::make_shared<ModalNegationStage>(
					kb, ctx, negModals);
			lastStage >> negModalStage;
			lastStage = negModalStage;
			addStage(lastStage);
		}

		lastStage >> outStream;
		firstBuffer->stopBuffering();
		addStage(lastStage);
	}
	// At this point outStream could already contain solutions, but these are buffered
	// such that they won't be lost during pipeline creation.

	// if there were multiple paths, consolidate answers from them.
	// e.g. if one yields no and the other true, the no should be ignored.
	if (qt.numPaths() > 1) {
		auto consolidator = std::make_shared<DisjunctiveBroadcaster>();
		outStream >> consolidator;
		finalStage_ = consolidator;
	} else {
		finalStage_ = outStream;
	}
	bufferStage_ = outStream;
}

QueryPipeline::QueryPipeline(KnowledgeBase *kb, const GraphPathQueryPtr &graphQuery) {
	auto &allLiterals = graphQuery->path();

	// --------------------------------------
	// split input literals into positive and negative literals.
	// negative literals are evaluated in parallel after all positive literals.
	// --------------------------------------
	std::vector<FramedTriplePatternPtr> positiveLiterals, negativeLiterals;
	for (auto &l: allLiterals) {
		if (l->isNegated()) negativeLiterals.push_back(l);
		else positiveLiterals.push_back(l);
	}

	// --------------------------------------
	// split positive literals into edb-only and computable.
	// also associate list of reasoner to computable literals.
	// --------------------------------------
	std::vector<FramedTriplePatternPtr> edbOnlyLiterals;
	std::vector<RDFComputablePtr> computableLiterals;
	for (auto &l: positiveLiterals) {
		auto property = l->propertyTerm();
		if (!property->isAtom()) {
			KB_WARN("Variable predicate in query not supported.");
			continue;
		}
		auto property_a = std::static_pointer_cast<Atomic>(property);
		auto l_reasoner = kb->reasonerManager()->getReasonerForRelation(
				PredicateIndicator(property_a->stringForm(), 2));
		if (l_reasoner.empty()) {
			edbOnlyLiterals.push_back(l);
			auto l_p = l->propertyTerm();
			if (l_p && l_p->termType() == TermType::ATOMIC &&
				!kb->isMaterializedInEDB(std::static_pointer_cast<Atomic>(l_p)->stringForm())) {
				// generate a "don't know" message and return.
				auto out = std::make_shared<TokenBuffer>();
				auto channel = TokenStream::Channel::create(out);
				auto dontKnow = std::make_shared<AnswerDontKnow>();
				KB_WARN("Predicate {} is neither materialized in EDB nor defined by a reasoner.", *l->predicate());
				channel->push(dontKnow);
				channel->push(EndOfEvaluation::get());
				finalStage_ = out;
				bufferStage_ = out;
				return;
			}
		} else {
			computableLiterals.push_back(std::make_shared<RDFComputable>(*l, l_reasoner));
		}
	}

	// --------------------------------------
	// sort positive literals.
	// --------------------------------------
	std::sort(edbOnlyLiterals.begin(), edbOnlyLiterals.end(), EDBComparator(kb->vocabulary()));

	// --------------------------------------
	// run EDB query with all edb-only literals.
	// --------------------------------------
	std::shared_ptr<TokenBuffer> edbOut;
	if (edbOnlyLiterals.empty()) {
		edbOut = std::make_shared<TokenBuffer>();
		auto channel = TokenStream::Channel::create(edbOut);
		channel->push(GenericYes());
		channel->push(EndOfEvaluation::get());
	} else {
		auto edb = kb->getBackendForQuery();
		edbOut = kb->edb()->getAnswerCursor(edb,
											std::make_shared<GraphPathQuery>(edbOnlyLiterals, graphQuery->ctx()));
	}
	addStage(edbOut);

	// --------------------------------------
	// handle positive IDB literals.
	// --------------------------------------
	std::shared_ptr<TokenBroadcaster> idbOut;
	if (computableLiterals.empty()) {
		idbOut = edbOut;
	} else {
		idbOut = std::make_shared<TokenBroadcaster>();
		addStage(idbOut);
		// --------------------------------------
		// Compute dependency groups of computable literals.
		// --------------------------------------
		DependencyGraph dg;
		dg.insert(computableLiterals.begin(), computableLiterals.end());

		// --------------------------------------
		// Construct a pipeline for each dependency group.
		// --------------------------------------
		if (dg.numGroups() == 1) {
			auto &literalGroup = *dg.begin();
			createComputationPipeline(
					kb,
					createComputationSequence(kb, literalGroup.member_),
					edbOut,
					idbOut,
					graphQuery->ctx());
		} else {
			// there are multiple dependency groups. They can be evaluated in parallel.

			// combines sub-answers computed in different parallel steps
			auto answerCombiner = std::make_shared<ConjunctiveBroadcaster>();
			// create a parallel step for each dependency group
			for (auto &literalGroup: dg) {
				// --------------------------------------
				// Construct a pipeline for each dependency group.
				// --------------------------------------
				createComputationPipeline(
						kb,
						createComputationSequence(kb, literalGroup.member_),
						edbOut,
						answerCombiner,
						graphQuery->ctx());
			}
			answerCombiner >> idbOut;
			addStage(answerCombiner);
		}
	}

	// --------------------------------------
	// Evaluate all negative literals in parallel.
	// --------------------------------------
	if (!negativeLiterals.empty()) {
		// run a dedicated stage where negated literals can be evaluated in parallel
		auto negStage = std::make_shared<PredicateNegationStage>(
				kb, graphQuery->ctx(), negativeLiterals);
		idbOut >> negStage;
		finalStage_ = negStage;
	} else {
		finalStage_ = idbOut;
	}
	bufferStage_ = edbOut;
}

QueryPipeline::~QueryPipeline() {
	for (auto &stage: stages_) {
		stage->close();
	}
	stages_.clear();
}

void QueryPipeline::addStage(const std::shared_ptr<TokenStream> &stage) {
	stages_.push_back(stage);
}

void QueryPipeline::operator>>(const std::shared_ptr<TokenStream> &stage) {
	finalStage_ >> stage;
}

void QueryPipeline::stopBuffering() {
	bufferStage_->stopBuffering();
}

namespace knowrob {
	// used to sort dependency nodes in a priority queue.
	// the nodes are considered to be dependent on each other through free variables.
	// the priority value is used to determine which nodes should be evaluated first.
	struct DependencyNodeComparator {
		bool operator()(const DependencyNodePtr &a, const DependencyNodePtr &b) const {
			// prefer node with fewer variables
			if (a->numVariables() != b->numVariables()) {
				return a->numVariables() > b->numVariables();
			}
			// prefer node with fewer neighbors
			if (a->numNeighbors() != b->numNeighbors()) {
				return a->numNeighbors() > b->numNeighbors();
			}
			return a < b;
		}
	};

	struct DependencyNodeQueue {
		const DependencyNodePtr node_;
		std::priority_queue<DependencyNodePtr, std::vector<DependencyNodePtr>, DependencyNodeComparator> neighbors_;

		explicit DependencyNodeQueue(const DependencyNodePtr &node) : node_(node) {
			// add all nodes to a priority queue
			for (auto &neighbor: node->neighbors()) {
				neighbors_.push(neighbor);
			}
		}
	};
}

std::vector<RDFComputablePtr> QueryPipeline::createComputationSequence(
		KnowledgeBase *kb,
		const std::list<DependencyNodePtr> &dependencyGroup) {
	// Pick a node to start with.
	auto comparator = IDBComparator(kb->vocabulary());
	DependencyNodePtr first;
	RDFComputablePtr firstComputable;
	for (auto &n: dependencyGroup) {
		auto computable_n =
				std::static_pointer_cast<RDFComputable>(n->literal());
		if (!first || comparator(firstComputable, computable_n)) {
			first = n;
			firstComputable = computable_n;
		}
	}

	// remember visited nodes, needed for circular dependencies
	// all nodes added to the queue should also be added to this set.
	std::set<DependencyNode *> visited;
	visited.insert(first.get());

	std::vector<RDFComputablePtr> sequence;
	sequence.push_back(firstComputable);

	// start with a FIFO queue only containing first node
	std::deque<std::shared_ptr<DependencyNodeQueue>> queue;
	auto qn0 = std::make_shared<DependencyNodeQueue>(first);
	queue.push_front(qn0);

	// loop until queue is empty and process exactly one successor of
	// the top element in the FIFO in each step. If an element has no
	// more successors, it can be removed from queue.
	// Each successor creates an additional node added to the top of the FIFO.
	while (!queue.empty()) {
		auto front = queue.front();

		// get top successor node that has not been visited yet
		DependencyNodePtr topNext;
		while (!front->neighbors_.empty()) {
			auto topNeighbor = front->neighbors_.top();
			front->neighbors_.pop();

			if (visited.count(topNeighbor.get()) == 0) {
				topNext = topNeighbor;
				break;
			}
		}
		// pop element from queue if all neighbors were processed
		if (front->neighbors_.empty()) {
			queue.pop_front();
		}

		if (topNext) {
			// push a new node onto FIFO
			auto qn_next = std::make_shared<DependencyNodeQueue>(topNext);
			queue.push_front(qn_next);
			sequence.push_back(std::static_pointer_cast<RDFComputable>(topNext->literal()));
			visited.insert(topNext.get());
		}
	}

	return sequence;
}

void QueryPipeline::createComputationPipeline(
		KnowledgeBase *kb,
		const std::vector<RDFComputablePtr> &computableLiterals,
		const std::shared_ptr<TokenBroadcaster> &pipelineInput,
		const std::shared_ptr<TokenBroadcaster> &pipelineOutput,
		const QueryContextPtr &ctx) {
	// This function generates a query pipeline for literals that can be computed
	// (EDB-only literals are processed separately). The literals are part of one dependency group.
	// They are sorted, and also evaluated in this order. For each computable literal there is at
	// least one reasoner that can compute the literal. However, instances of the literal may also
	// occur in the EDB. Hence, computation results must be combined with results of an EDB query
	// for each literal.

	auto lastOut = pipelineInput;

	for (auto &lit: computableLiterals) {
		auto stepInput = lastOut;
		auto stepOutput = std::make_shared<TokenBroadcaster>();
		addStage(stepOutput);

		// --------------------------------------
		// Construct a pipeline that grounds the literal in the EDB.
		// But only add an EDB stage if the predicate was materialized in EDB before,
		// or if the predicate is a variable.
		// --------------------------------------
		bool isEDBStageNeeded = true;
		if (lit->propertyTerm() && lit->propertyTerm()->termType() == TermType::ATOMIC) {
			isEDBStageNeeded = kb->isMaterializedInEDB(
					std::static_pointer_cast<Atomic>(lit->propertyTerm())->stringForm());
		}
		if (isEDBStageNeeded) {
			auto edb = kb->getBackendForQuery();
			auto edbStage = std::make_shared<TypedQueryStage<FramedTriplePattern>>(
					ctx,
					lit,
					[&](const FramedTriplePatternPtr &q) {
						return kb->edb()->getAnswerCursor(edb, std::make_shared<GraphPathQuery>(q, ctx));
					});
			edbStage->selfWeakRef_ = edbStage;
			stepInput >> edbStage;
			edbStage >> stepOutput;
			addStage(edbStage);
		}

		// --------------------------------------
		// Construct a pipeline that grounds the literal in the IDB.
		// To this end add an IDB stage for each reasoner that defines the literal.
		// --------------------------------------
		for (auto &r: lit->reasonerList()) {
			auto idbStage = std::make_shared<TypedQueryStage<FramedTriplePattern>>(
					ctx, lit,
					[&r, &ctx](const FramedTriplePatternPtr &q) {
						return r->submitQuery(q, ctx);
					});
			idbStage->selfWeakRef_ = idbStage;
			stepInput >> idbStage;
			idbStage >> stepOutput;
			addStage(idbStage);
			// TODO: what about the materialization of the predicate in EDB?
		}

		// add a stage that consolidates the results of the EDB and IDB stages.
		// in particular the case needs to be handled where none of the stages return
		// 'true'. Also print a warning if two stages disagree but state they are confident.
		auto consolidator = std::make_shared<DisjunctiveBroadcaster>();
		stepOutput >> consolidator;
		// TODO: are all these addStage calls really needed?
		addStage(consolidator);

		// --------------------------------------
		// Optionally add a stage to the pipeline that drops all redundant result.
		// The filter is applied here to remove redundancies early on directly after IDB and EDB
		// results are combined.
		// --------------------------------------
		if (ctx->queryFlags & QUERY_FLAG_UNIQUE_SOLUTIONS) {
			auto filterStage = std::make_shared<RedundantAnswerFilter>();
			stepOutput >> filterStage;
			lastOut = filterStage;
		} else {
			lastOut = stepOutput;
		}
	}

	lastOut >> pipelineOutput;
}

bool knowrob::EDBComparator::operator()(const FramedTriplePatternPtr &a, const FramedTriplePatternPtr &b) const {
	// - prefer evaluation of literals with fewer variables
	auto numVars_a = a->numVariables();
	auto numVars_b = b->numVariables();
	if (numVars_a != numVars_b) return (numVars_a > numVars_b);

	// - prefer literals with grounded predicate
	bool hasProperty_a = (a->propertyTerm() && a->propertyTerm()->termType() == TermType::ATOMIC);
	bool hasProperty_b = (b->propertyTerm() && b->propertyTerm()->termType() == TermType::ATOMIC);
	if (hasProperty_a != hasProperty_b) return (hasProperty_a < hasProperty_b);

	// - prefer properties that appear less often in the EDB
	if (hasProperty_a) {
		auto numAsserts_a = vocabulary_->frequency(
				std::static_pointer_cast<Atomic>(a->propertyTerm())->stringForm());
		auto numAsserts_b = vocabulary_->frequency(
				std::static_pointer_cast<Atomic>(b->propertyTerm())->stringForm());
		if (numAsserts_a != numAsserts_b) return (numAsserts_a > numAsserts_b);
	}

	return (a < b);
}

bool knowrob::IDBComparator::operator()(const RDFComputablePtr &a, const RDFComputablePtr &b) const {
	// - prefer evaluation of literals with fewer variables
	auto numVars_a = a->numVariables();
	auto numVars_b = b->numVariables();
	if (numVars_a != numVars_b) return (numVars_a > numVars_b);

	// - prefer literals with grounded predicate
	bool hasProperty_a = (a->propertyTerm() && a->propertyTerm()->termType() == TermType::ATOMIC);
	bool hasProperty_b = (b->propertyTerm() && b->propertyTerm()->termType() == TermType::ATOMIC);
	if (hasProperty_a != hasProperty_b) return (hasProperty_a < hasProperty_b);

	// - prefer literals with EDB assertions over literals without
	if (hasProperty_a) {
		auto hasEDBAssertion_a = vocabulary_->isDefinedProperty(
				std::static_pointer_cast<Atomic>(a->propertyTerm())->stringForm());
		auto hasEDBAssertion_b = vocabulary_->isDefinedProperty(
				std::static_pointer_cast<Atomic>(b->propertyTerm())->stringForm());
		if (hasEDBAssertion_a != hasEDBAssertion_b) return (hasEDBAssertion_a < hasEDBAssertion_b);
	}

	// - prefer properties that appear less often in the EDB
	if (hasProperty_a) {
		auto numAsserts_a = vocabulary_->frequency(
				std::static_pointer_cast<Atomic>(a->propertyTerm())->stringForm());
		auto numAsserts_b = vocabulary_->frequency(
				std::static_pointer_cast<Atomic>(b->propertyTerm())->stringForm());
		if (numAsserts_a != numAsserts_b) return (numAsserts_a > numAsserts_b);
	}

	// - prefer literals with more reasoner
	auto numReasoner_a = a->reasonerList().size();
	auto numReasoner_b = b->reasonerList().size();
	if (numReasoner_a != numReasoner_b) return (numReasoner_a < numReasoner_b);

	return (a < b);
}
