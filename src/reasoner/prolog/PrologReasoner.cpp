/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <memory>
#include <filesystem>

#include "knowrob/Logger.h"
#include "knowrob/reasoner/ReasonerManager.h"
#include "knowrob/reasoner/prolog/PrologReasoner.h"
#include "knowrob/reasoner/prolog/semweb.h"
#include "knowrob/terms/ListTerm.h"
#include "knowrob/formulas/Bottom.h"
#include "knowrob/queries/TokenQueue.h"
#include "knowrob/semweb/PrefixRegistry.h"
#include "knowrob/semweb/ImportHierarchy.h"
#include "knowrob/KnowledgeBase.h"
#include "knowrob/integration/prolog/PrologBackend.h"
#include "knowrob/reasoner/ReasonerError.h"
#include "knowrob/reasoner/prolog/semweb.h"
#include "knowrob/queries/AnswerNo.h"
#include "knowrob/terms/Numeric.h"
#include "knowrob/TimePoint.h"

using namespace knowrob;

static const int prologQueryFlags = PL_Q_CATCH_EXCEPTION | PL_Q_NODEBUG;

#define PROLOG_REASONER_EVAL(goal) PROLOG_ENGINE_EVAL(getReasonerQuery(goal))

// make reasoner type accessible
KNOWROB_BUILTIN_REASONER("Prolog", PrologReasoner)

bool PrologReasoner::isKnowRobInitialized_ = false;

static inline std::shared_ptr<PrologReasoner> getPrologReasoner(term_t t_reasonerManager, term_t t_reasonerModule) {
	auto definedReasoner = PrologReasoner::getDefinedReasoner(t_reasonerManager, t_reasonerModule);
	if (!definedReasoner) {
		KB_ERROR("unable to find reasoner with id '{}' (manager id: {}).",
				 *PrologTerm::toKnowRobTerm(t_reasonerModule),
				 *PrologTerm::toKnowRobTerm(t_reasonerManager));
		return {};
	}
	auto reasoner = definedReasoner->value();
	auto typedReasoner = std::dynamic_pointer_cast<PrologReasoner>(reasoner);
	if (!typedReasoner) {
		KB_ERROR("reasoner with id '{}' (manager id: {}) is not a mongolog reasoner.",
				 *PrologTerm::toKnowRobTerm(t_reasonerModule),
				 *PrologTerm::toKnowRobTerm(t_reasonerManager));
	}
	return typedReasoner;
}

namespace knowrob::prolog {
	static foreign_t
	reasoner_define_relation4(term_t t_reasonerManager, term_t t_reasonerModule, term_t t_relation, term_t t_arity) {
		auto reasoner = getPrologReasoner(t_reasonerManager, t_reasonerModule);
		char *relationName;
		int64_t arity;
		if (reasoner &&
			PL_get_atom_chars(t_relation, &relationName) &&
			PL_get_int64(t_arity, &arity)) {
			reasoner->defineRelation(PredicateIndicator(relationName, arity));
			return TRUE;
		} else {
			return FALSE;
		}
	}
}

PrologReasoner::PrologReasoner() : GoalDrivenReasoner() {
	addDataHandler("prolog", [this]
			(const DataSourcePtr &dataFile) { return consult(dataFile->uri()); });
}

PrologReasoner::~PrologReasoner() = default;

std::string_view PrologReasoner::callFunctor() {
	static const auto reasoner_call_f = "reasoner_call";
	return reasoner_call_f;
}

PrologTerm PrologReasoner::transformGoal(const PrologTerm &goal) {
	return PrologTerm(callFunctor(), goal, PrologTerm::nil());
}

bool PrologReasoner::initializeReasoner(const PropertyTree &cfg) {
	// call PL_initialize
	PrologEngine::initializeProlog();

	if (!isKnowRobInitialized_) {
		isKnowRobInitialized_ = true;

		PL_register_foreign("reasoner_define_relation_cpp", 4, (pl_function_t) prolog::reasoner_define_relation4, 0);
		PL_register_extensions_in_module("semweb", prolog::PL_extension_semweb);

		// auto-load some files into "user" module
		initializeGlobalPackages();

		// load init file from reasoner directory
		consult(std::filesystem::path("reasoner") / "prolog" / "__init__.pl", "user", false);

		// register RDF namespaces with Prolog.
		// in particular the ones specified in settings are globally registered with PrefixRegistry.
		static const auto register_prefix_i = "sw_register_prefix";
		for (auto &pair: PrefixRegistry::get()) {
			const auto &uri = pair.first;
			const auto &alias = pair.second;
			PROLOG_REASONER_EVAL(PrologTerm(register_prefix_i, alias, uri + "#"));
		}
	}

	// load properties into the reasoner module.
	// this is needed mainly for `reasoner_setting/2` that provides reasoner instance specific settings.
	for (auto &pair: cfg) {
		auto key_t = cfg.createKeyTerm(pair.first);
		setReasonerSetting(key_t, pair.second);
	}
	// load reasoner default packages. this is usually the code that implements the reasoner.
	initializeDefaultPackages();

	return true;
}

void PrologReasoner::setDataBackend(const StoragePtr &backend) {
	knowledgeGraph_ = std::dynamic_pointer_cast<PrologBackend>(backend);
	if (!knowledgeGraph_) {
		throw ReasonerError("Unexpected data knowledgeGraph used for Prolog reasoner. PrologBackend must be used.");
	}
}

bool PrologReasoner::setReasonerSetting(const TermPtr &key, const TermPtr &valueString) {
	static auto set_setting_f = "reasoner_set_setting";
	return PROLOG_REASONER_EVAL(PrologTerm(set_setting_f, reasonerName(), key, valueString));
}

PrologTerm PrologReasoner::getReasonerQuery(const PrologTerm &goal) {
	static const auto b_setval_f = "b_setval";
	static const auto reasoner_module_a = "reasoner_module";
	static const auto reasoner_manager_a = "reasoner_manager";
	auto managerID = std::make_shared<Integer>(reasonerManager().managerID());
	return PrologTerm(b_setval_f, reasoner_module_a, reasonerName()) &
		   PrologTerm(b_setval_f, reasoner_manager_a, managerID) &
		   goal;
}

std::shared_ptr<NamedReasoner> PrologReasoner::getDefinedReasoner(
		const term_t &t_reasonerManager, const term_t &t_reasonerModule) {
	int i_reasonerManager;
	if (!PL_get_integer(t_reasonerManager, &i_reasonerManager)) return {};
	auto reasonerManager = ReasonerManager::getManager(i_reasonerManager);
	if (!reasonerManager) return {};

	char *reasonerModule;
	if (PL_get_atom_chars(t_reasonerModule, &reasonerModule)) {
		return reasonerManager->getPluginWithID(reasonerModule);
	} else {
		return {};
	}
}

bool PrologReasoner::consult(const std::filesystem::path &uri, const char *module, bool doTransformQuery) {
	static auto consult_f = "consult";
	auto path = PrologEngine::getPrologPath(uri);

	return PrologEngine::eval([&]() {
		PrologTerm plTerm(consult_f, path.native());
		if (module) plTerm.setModule(module);
		return getReasonerQuery(doTransformQuery ? transformGoal(plTerm) : plTerm);
	});
}

bool PrologReasoner::load_rdf_xml(const std::filesystem::path &rdfFile) {
	static auto load_rdf_xml_f = "load_rdf_xml";
	auto path = PrologEngine::getResourcePath(rdfFile);
	return PROLOG_REASONER_EVAL(PrologTerm(load_rdf_xml_f, path.native(), reasonerName()));
}

TokenBufferPtr PrologReasoner::submitQuery(const FramedTriplePatternPtr &literal, const QueryContextPtr &ctx) {
	// context term options:
	static const auto query_scope_f = "query_scope";
	static const auto solution_scope_f = "solution_scope";
	static const auto triple_f = "triple";

	auto answerBuffer = std::make_shared<TokenBuffer>();
	auto outputChannel = TokenStream::Channel::create(answerBuffer);

	// create runner that evaluates the goal in a thread with a Prolog engine
	auto runner = std::make_shared<ThreadPool::LambdaRunner>(
			[&](const ThreadPool::LambdaRunner::StopChecker &hasStopRequest) {
				PrologTerm queryFrame, answerFrame;
				putQueryFrame(queryFrame, ctx->selector);

				PrologTerm rdfGoal(*literal, triple_f);
				// :- ContextTerm = [query_scope(...), solution_scope(Variable)]
				PrologList contextTerm({
											   PrologTerm(query_scope_f, queryFrame),
											   PrologTerm(solution_scope_f, answerFrame)
									   });
				// :- QueryGoal = ( b_setval(reasoner_module, reasonerName()),
				//                  b_setval(reasoner_manager, managerID),
				//                  reasoner_call(LiteralGoal, ContextTerm) ).
				PrologTerm queryGoal = getReasonerQuery(
						PrologTerm(callFunctor(), rdfGoal, contextTerm));

				auto qid = queryGoal.openQuery(prologQueryFlags);
				bool hasSolution = false;
				while (!hasStopRequest() && queryGoal.nextSolution(qid)) {
					outputChannel->push(yes(literal, rdfGoal, answerFrame));
					hasSolution = true;
					if (ctx->queryFlags & QUERY_FLAG_ONE_SOLUTION) break;
				}
				PL_close_query(qid);
				if (!hasSolution) outputChannel->push(no(literal));
				outputChannel->push(EndOfEvaluation::get());
			});

	// push goal and return
	PrologEngine::pushGoal(runner, [literal, outputChannel](const std::exception &e) {
		KB_WARN("an exception occurred for prolog query ({}): {}.", *literal, e.what());
		outputChannel->close();
	});

	return answerBuffer;
}

AnswerYesPtr PrologReasoner::yes(const FramedTriplePatternPtr &literal,
								 const PrologTerm &rdfGoal,
								 const PrologTerm &answerFrameTerm) {
	KB_DEBUG("Prolog has a next solution.");

	// create bindings object
	auto bindings = std::make_shared<Bindings>();
	for (const auto &kv: rdfGoal.vars()) {
		auto grounding = PrologTerm::toKnowRobTerm(kv.second);
		if (grounding && grounding->termType() != TermType::VARIABLE) {
			bindings->set(std::make_shared<Variable>(kv.first), PrologTerm::toKnowRobTerm(kv.second));
		}
	}

	// create an answer object given the bindings
	auto yes = std::make_shared<AnswerYes>(bindings);
	yes->setReasonerTerm(reasonerName());

	// set the solution scope, if reasoner specified it
	auto answerFrame_rw = createAnswerFrame(answerFrameTerm);
	GraphSelectorPtr answerFrame_ro;
	if (answerFrame_rw) {
		yes->setFrame(answerFrame_rw);
		answerFrame_ro = answerFrame_rw;
	} else {
		answerFrame_ro = DefaultGraphSelector();
	}

	// store instantiation of literal
	auto p = literal->predicate();
	auto p_instance = applyBindings(p, *yes->substitution());
	yes->addGrounding(std::static_pointer_cast<Predicate>(p_instance), answerFrame_ro, literal->isNegated());

	return yes;
}

AnswerNoPtr PrologReasoner::no(const FramedTriplePatternPtr &rdfLiteral) {
	KB_DEBUG("Prolog has no solution.");
	// if no solution was found, indicate that via a NegativeAnswer.
	auto negativeAnswer = std::make_shared<AnswerNo>();
	negativeAnswer->setReasonerTerm(reasonerName());
	// however, as Prolog cannot proof negations such an answer is always not well-founded
	// and can be overruled by a well-founded one.
	negativeAnswer->setIsUncertain(true, std::nullopt);
	// as the query goal was only a single literal, we know that exactly this literal cannot be proven.
	negativeAnswer->addUngrounded(
			std::static_pointer_cast<Predicate>(rdfLiteral->predicate()),
			rdfLiteral->isNegated());
	return negativeAnswer;
}


bool PrologReasoner::putQueryFrame(PrologTerm &frameTerm, const GraphSelector &frame) {
	// Map an GraphSelector to a Prolog dict term.
	// The term has the form:
	// { epistemicMode: knowledge|belief,
	//   temporalMode: sometimes|always,
	//   [agent: $name,]
	//   [since: $time,]
	//   [until: $time] }

	int numFrameKeys = 2;
	if (frame.perspective) numFrameKeys += 1;
	if (frame.begin.has_value()) numFrameKeys += 1;
	if (frame.end.has_value()) numFrameKeys += 1;

	// option: frame($dictTerm)
	atom_t scopeKeys[numFrameKeys];
	auto scopeValues = PL_new_term_refs(numFrameKeys);

	// epistemicMode: knowledge|belief
	static const auto epistemicMode_a = PL_new_atom("epistemicMode");
	static const auto knowledge_a = PL_new_atom("knowledge");
	static const auto belief_a = PL_new_atom("belief");

	bool isAboutBelief = (frame.uncertain);
	int keyIndex = 0;
	scopeKeys[keyIndex] = epistemicMode_a;
	if (!PL_put_atom(scopeValues, isAboutBelief ? belief_a : knowledge_a)) return false;

	// temporalMode: sometimes|always
	static const auto temporalMode_a = PL_new_atom("temporalMode");
	static const auto sometimes_a = PL_new_atom("sometimes");
	static const auto always_a = PL_new_atom("always");

	bool isAboutSomePast = (frame.occasional);
	scopeKeys[++keyIndex] = temporalMode_a;
	if (!PL_put_atom(scopeValues + keyIndex, isAboutSomePast ? sometimes_a : always_a)) return false;

	// agent: $name
	if (frame.perspective) {
		static const auto agent_a = PL_new_atom("agent");
		scopeKeys[++keyIndex] = agent_a;
		auto agent_iri = frame.perspective->iri();
		if (!PL_put_atom_chars(scopeValues + keyIndex, agent_iri.data())) return false;
	}

	// since: $name
	if (frame.begin.has_value()) {
		static const auto since_a = PL_new_atom("since");
		scopeKeys[++keyIndex] = since_a;
		if (!PL_put_float(scopeValues + keyIndex, frame.begin.value())) return false;
	}
	// until: $name
	if (frame.end.has_value()) {
		static const auto until_a = PL_new_atom("until");
		scopeKeys[++keyIndex] = until_a;
		if (!PL_put_float(scopeValues + keyIndex, frame.end.value())) return false;
	}

	return PL_put_dict(frameTerm(), 0, numFrameKeys, scopeKeys, scopeValues);
}

std::shared_ptr<GraphSelector> PrologReasoner::createAnswerFrame(const PrologTerm &plTerm) {
	std::shared_ptr<GraphSelector> frame;

	if (PL_is_variable(plTerm())) {
		// reasoner did not specify a solution scope
		return {};
	} else if (PL_is_dict(plTerm())) {
		// the solution scope was generated as a Prolog dictionary
		auto scope_val = PL_new_term_ref();

		frame = std::make_shared<GraphSelector>();

		// read "time" key, the value is expected to be a predicate `range(Since,Until)`
		static const auto time_key = PL_new_atom("time");
		if (PL_get_dict_key(time_key, plTerm(), scope_val)) {
			term_t arg = PL_new_term_ref();
			std::optional<TimePoint> v_since, v_until;
			double val = 0.0;

			if (PL_get_arg(1, scope_val, arg) &&
				PL_term_type(arg) == PL_FLOAT &&
				PL_get_float(arg, &val) &&
				val > 0.001) { frame->begin = val; }

			if (PL_get_arg(2, scope_val, arg) &&
				PL_term_type(arg) == PL_FLOAT &&
				PL_get_float(arg, &val)) { frame->end = val; }
		}

		static const auto uncertain_key = PL_new_atom("uncertain");
		if (PL_get_dict_key(uncertain_key, plTerm(), scope_val)) {
			atom_t flagAtom;
			if (PL_term_type(scope_val) == PL_ATOM && PL_get_atom(scope_val, &flagAtom)) {
				if (flagAtom == PrologTerm::ATOM_true()) {
					frame->uncertain = true;
				}
			}
		}

		static const auto confidence_key = PL_new_atom("confidence");
		if (PL_get_dict_key(confidence_key, plTerm(), scope_val)) {
			double confidenceValue = 1.0;
			if (PL_term_type(scope_val) == PL_FLOAT && PL_get_float(scope_val, &confidenceValue)) {
				frame->confidence = confidenceValue;
				if (confidenceValue > 0.999) {
					frame->uncertain = false;
				} else {
					frame->uncertain = true;
				}
			}
		}

		static const auto occasional_key = PL_new_atom("occasional");
		if (PL_get_dict_key(occasional_key, plTerm(), scope_val)) {
			atom_t flagAtom;
			if (PL_term_type(scope_val) == PL_ATOM && PL_get_atom(scope_val, &flagAtom)) {
				frame->occasional = (flagAtom == PrologTerm::ATOM_true());
			}
		}

		static const auto agent_key = PL_new_atom("agent");
		if (PL_get_dict_key(agent_key, plTerm(), scope_val)) {
			atom_t agentAtom;
			if (PL_term_type(scope_val) == PL_ATOM && PL_get_atom(scope_val, &agentAtom)) {
				frame->perspective = Perspective::get(PL_atom_chars(agentAtom));
			}
		}

		PL_reset_term_refs(scope_val);
	} else {
		KB_WARN("solution scope has an unexpected type (should be dict).");
	}

	if (frame) {
		return frame;
	} else {
		return {};
	}
}

std::list<TermPtr> PrologReasoner::runTests(const std::string &target) {
	static const auto xunit_var = std::make_shared<Variable>("Term");
	static const auto silent_flag = Atom::Tabled("silent");
	static const auto xunit_opt = std::make_shared<Function>(Function("xunit_term", {xunit_var}));

	TermPtr pred = std::make_shared<Function>(Function(
			"test_and_report", {
					// unittest target
					Atom::Tabled(target),
					// options
					std::make_shared<ListTerm>(ListTerm({xunit_opt, silent_flag}))
			}));
	auto solutions = PROLOG_ENGINE_ALL_SOL(getReasonerQuery(PrologTerm(pred)));

	std::list<TermPtr> output;
	for (auto &solution: solutions) {
		if (solution.count(xunit_var->name()) > 0) {
			output.push_back(solution[xunit_var->name()]);
		} else {
			KB_WARN("Solution has no key '{}'.", xunit_var->name());
		}
	}
	return output;
}
