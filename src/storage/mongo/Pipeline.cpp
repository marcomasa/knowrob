/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "string"
#include "knowrob/storage/mongo/Pipeline.h"
#include "knowrob/Logger.h"
#include "knowrob/URI.h"
#include "knowrob/triples/GraphPattern.h"
#include "knowrob/triples/GraphSequence.h"
#include "knowrob/triples/GraphBuiltin.h"
#include "knowrob/storage/mongo/MongoTerm.h"
#include "knowrob/storage/mongo/MongoTriplePattern.h"

using namespace knowrob::mongo;

Pipeline::Pipeline(bson_t *arrayDocument)
		: arrayDocument_(arrayDocument),
		  numStages_(0),
		  lastStage_(nullptr),
		  lastOperator_(nullptr) {
}

bson_t *Pipeline::appendStageBegin() {
	auto arrayKey = std::to_string(numStages_++);
	bson_wrapper &stage = stages_.emplace_back();
	BSON_APPEND_DOCUMENT_BEGIN(arrayDocument_, arrayKey.c_str(), &stage.bson);
	lastStage_ = &stage.bson;
	return &stage.bson;
}

bson_t *Pipeline::appendStageBegin(std::string_view stageOperatorString) {
	auto arrayKey = std::to_string(numStages_++);
	bson_wrapper &stage = stages_.emplace_back();
	bson_wrapper &stageOperator = stageOperators_.emplace_back();
	BSON_APPEND_DOCUMENT_BEGIN(arrayDocument_, arrayKey.c_str(), &stage.bson);
	BSON_APPEND_DOCUMENT_BEGIN(&stage.bson, stageOperatorString.data(), &stageOperator.bson);
	lastOperator_ = &stageOperator.bson;
	lastStage_ = &stage.bson;
	return &stageOperator.bson;
}

void Pipeline::appendStageEnd(bson_t *stage) {
	if (lastOperator_ == stage) {
		bson_append_document_end(lastStage_, lastOperator_);
		bson_append_document_end(arrayDocument_, lastStage_);
	} else {
		bson_append_document_end(arrayDocument_, stage);
	}
}

void Pipeline::append(const knowrob::FramedTriplePattern &query, const TripleStore &tripleStore) {
	// append lookup stages to pipeline
	TripleLookupData lookupData(&query);
	// indicate that no variables in tripleExpression may have been instantiated
	// by a previous step to allow for some optimizations.
	lookupData.mayHasMoreGroundings = false;
	lookupTriple(*this, tripleStore, lookupData);
}

void Pipeline::append(const knowrob::GraphTerm &query, const TripleStore &tripleStore) {
	std::set<std::string_view> groundedVariables;
	appendTerm_recursive(query, tripleStore, groundedVariables);
}

void Pipeline::appendTerm_recursive(const knowrob::GraphTerm &query, // NOLINT
									const TripleStore &tripleStore,
									std::set<std::string_view> &groundedVariables) {
	switch (query.termType()) {
		case knowrob::GraphTermType::Pattern: {
			auto &expr = ((const GraphPattern &) query).value();
			TripleLookupData lookupData(expr.get());
			// indicate that all previous groundings of variables are known
			lookupData.mayHasMoreGroundings = false;
			lookupData.knownGroundedVariables = groundedVariables;
			// remember variables in tripleExpression, they have a grounding in next step
			if (!expr->isOptional()) {
				for (auto &var: expr->getVariables()) {
					groundedVariables.insert(var->name());
				}
			}
			lookupTriple(*this, tripleStore, lookupData);
			break;
		}
		case knowrob::GraphTermType::Builtin:
			appendBuiltin((const knowrob::GraphBuiltin &) query);
			break;
		case knowrob::GraphTermType::Sequence:
			for (auto &elem: ((const knowrob::GraphSequence &) query).terms()) {
				appendTerm_recursive(*elem, tripleStore, groundedVariables);
			}
			break;
		case knowrob::GraphTermType::Union:
			appendUnion((const knowrob::GraphUnion &) query, tripleStore, groundedVariables);
			break;
	}
}

void Pipeline::appendUnion(const knowrob::GraphUnion &unionTerm,
						   const TripleStore &tripleStore,
						   std::set<std::string_view> &groundedVariables) {
	// First run a $lookup operation for each branch of the union.
	for (uint32_t i = 0; i < unionTerm.terms().size(); i++) {
		auto branchVars = groundedVariables;
		// construct inner pipelines, one for each branch of the union
		Document pipelineDoc(bson_new());
		Pipeline nestedPipeline(pipelineDoc.bson());
		nestedPipeline.setIsNested(true);
		nestedPipeline.appendTerm_recursive(*unionTerm.terms()[i], tripleStore, branchVars);
		auto pipelineArray = nestedPipeline.arrayDocument();

		// construct a lookup
		bson_t letDoc;
		auto lookupStage = appendStageBegin("$lookup");
		BSON_APPEND_UTF8(lookupStage, "from", tripleStore.oneCollection->name().data());
		BSON_APPEND_UTF8(lookupStage, "as", ("next" + std::to_string(i)).data());
		BSON_APPEND_DOCUMENT_BEGIN(lookupStage, "let", &letDoc);
		BSON_APPEND_UTF8(&letDoc, "v_VARS", isNested_ ? "$$v_VARS" : "$v_VARS");
		bson_append_document_end(lookupStage, &letDoc);
		BSON_APPEND_ARRAY_BEGIN(lookupStage, "pipeline", pipelineArray);
		bson_append_array_end(lookupStage, pipelineArray);
		appendStageEnd(lookupStage);
	}

	// concatenate individual results
	bson_t concatDoc, concatArray;
	auto setConcatStage = appendStageBegin("$set");
	BSON_APPEND_DOCUMENT_BEGIN(setConcatStage, "next", &concatDoc);
	BSON_APPEND_ARRAY_BEGIN(&concatDoc, "$concatArrays", &concatArray);
	for (uint32_t i = 0; i < unionTerm.terms().size(); i++) {
		BSON_APPEND_UTF8(&concatArray, std::to_string(i).c_str(), ("$next" + std::to_string(i)).data());
	}
	bson_append_array_end(&concatDoc, &concatArray);
	bson_append_document_end(setConcatStage, &concatDoc);
	appendStageEnd(setConcatStage);
	// delete individual results
	for (uint32_t i = 0; i < unionTerm.terms().size(); i++) {
		unset("next" + std::to_string(i));
	}
	// unwind the concatenated array
	unwind("$next");
	// project the bindings of one of the branches into v_VARS field
	bson_t setDoc, mergeArray;
	auto setMergedStage = appendStageBegin("$set");
	BSON_APPEND_DOCUMENT_BEGIN(setMergedStage, "v_VARS", &setDoc);
	BSON_APPEND_ARRAY_BEGIN(&setDoc, "$mergeObjects", &mergeArray);
	BSON_APPEND_UTF8(&mergeArray, "0", "$next.v_VARS");
	BSON_APPEND_UTF8(&mergeArray, "1", "$v_VARS");
	bson_append_array_end(&setDoc, &mergeArray);
	bson_append_document_end(setMergedStage, &setDoc);
	appendStageEnd(setMergedStage);
	// and finally unset the next field
	unset("next");
}

void Pipeline::appendBuiltin(const knowrob::GraphBuiltin &builtin) {
	switch (builtin.builtinType()) {
		case knowrob::GraphBuiltinType::Bind:
			bindValue(builtin);
			break;
		case knowrob::GraphBuiltinType::Max:
			setAccumulated(builtin, "$max");
			break;
		case knowrob::GraphBuiltinType::Min:
			setAccumulated(builtin, "$min");
			break;
		case knowrob::GraphBuiltinType::LessOrEqual:
			matchBinary(builtin, "$lte");
			break;
		case knowrob::GraphBuiltinType::Less:
			matchBinary(builtin, "$lt");
			break;
		case knowrob::GraphBuiltinType::Greater:
			matchBinary(builtin, "$gt");
			break;
		case knowrob::GraphBuiltinType::GreaterOrEqual:
			matchBinary(builtin, "$gte");
			break;
		case knowrob::GraphBuiltinType::Equal:
			matchBinary(builtin, "$eq");
			break;
	}
}

void Pipeline::bindValue(const knowrob::GraphBuiltin &builtin) {
	// e.g. `{ $set: { "begin": "$next.begin" } }`
	if (!builtin.bindVar()) {
		KB_ERROR("No variable to bind in $min/$max operation");
		return;
	}
	if (builtin.arguments().size() != 1) {
		KB_ERROR("Bind operation requires one argument");
		return;
	}
	static const std::string varPrefix = "v_VARS.";
	static const std::string varSuffix = ".val";
	auto setStage = appendStageBegin("$set");
	auto varKey = varPrefix + std::string(builtin.bindVar()->name()) + varSuffix;
	MongoTerm::appendWithVars(setStage, varKey.c_str(), builtin.arguments()[0]);
	appendStageEnd(setStage);
}

void Pipeline::setAccumulated(const knowrob::GraphBuiltin &builtin, std::string_view predicate) {
	// e.g. `{ $set:  "begin", { $max: ["$begin", "$next.begin"] } }`
	// NOTE: `$min [null,2]` -> 2 and `$max [null,2]` -> 2
	if (!builtin.bindVar()) {
		KB_ERROR("No variable to bind in $min/$max operation");
		return;
	}
	static const std::string varPrefix = "v_VARS.";
	static const std::string varSuffix = ".val";
	bson_t accumulatedDoc, inputArray;
	auto setStage = appendStageBegin("$set");
	auto varKey = varPrefix + std::string(builtin.bindVar()->name()) + varSuffix;
	BSON_APPEND_DOCUMENT_BEGIN(setStage, varKey.c_str(), &accumulatedDoc);
	BSON_APPEND_ARRAY_BEGIN(&accumulatedDoc, predicate.data(), &inputArray);
	for (uint32_t i = 0; i < builtin.arguments().size(); i++) {
		MongoTerm::appendWithVars(&inputArray, std::to_string(i).c_str(), builtin.arguments()[i]);
	}
	bson_append_array_end(&accumulatedDoc, &inputArray);
	bson_append_document_end(setStage, &accumulatedDoc);
	appendStageEnd(setStage);
}

void Pipeline::matchBinary(const knowrob::GraphBuiltin &builtin, std::string_view predicate) {
	// e.g.: `{ $match: { $expr: { $lte: ["$v_scope.begin", "$v_scope.end"] } } }`
	if (builtin.arguments().size() != 2) {
		KB_ERROR("Binary operation requires two arguments");
		return;
	}
	bson_t exprDoc, ltDoc;
	auto matchStage = appendStageBegin("$match");
	BSON_APPEND_DOCUMENT_BEGIN(matchStage, "$expr", &exprDoc);
	BSON_APPEND_ARRAY_BEGIN(&exprDoc, predicate.data(), &ltDoc);
	MongoTerm::appendWithVars(&ltDoc, "0", builtin.arguments()[0]);
	MongoTerm::appendWithVars(&ltDoc, "1", builtin.arguments()[1]);
	bson_append_array_end(&exprDoc, &ltDoc);
	bson_append_document_end(matchStage, &exprDoc);
	appendStageEnd(matchStage);
}

void Pipeline::limit(uint32_t maxDocuments) {
	auto unwindStage = appendStageBegin();
	BSON_APPEND_INT32(unwindStage, "$limit", maxDocuments);
	appendStageEnd(unwindStage);
}

void Pipeline::unwind(std::string_view field, bool preserveNullAndEmptyArrays) {
	if (preserveNullAndEmptyArrays) {
		auto unwindStage = appendStageBegin("$unwind");
		BSON_APPEND_BOOL(unwindStage, "preserveNullAndEmptyArrays", 1);
		appendStageEnd(unwindStage);
	} else {
		auto unwindStage = appendStageBegin();
		BSON_APPEND_UTF8(unwindStage, "$unwind", field.data());
		appendStageEnd(unwindStage);
	}
}

void Pipeline::unset(std::string_view field) {
	auto unwindStage = appendStageBegin();
	BSON_APPEND_UTF8(unwindStage, "$unset", field.data());
	appendStageEnd(unwindStage);
}

void Pipeline::replaceRoot(std::string_view newRootField) {
	auto unwindStage = appendStageBegin("$replaceRoot");
	BSON_APPEND_UTF8(unwindStage, "newRoot", newRootField.data());
	appendStageEnd(unwindStage);
}

void Pipeline::sortAscending(std::string_view field) {
	auto sortStage = appendStageBegin("$sort");
	BSON_APPEND_INT32(sortStage, field.data(), 1);
	appendStageEnd(sortStage);
}

void Pipeline::sortDescending(std::string_view field) {
	auto sortStage = appendStageBegin("$sort");
	BSON_APPEND_INT32(sortStage, field.data(), -1);
	appendStageEnd(sortStage);
}

void Pipeline::merge(std::string_view collection) {
	auto unwindStage = appendStageBegin("$merge");
	BSON_APPEND_UTF8(unwindStage, "into", collection.data());
	BSON_APPEND_UTF8(unwindStage, "on", "_id");
	BSON_APPEND_UTF8(unwindStage, "whenMatched", "merge");
	appendStageEnd(unwindStage);
}

void Pipeline::project(std::string_view field) {
	auto projectStage = appendStageBegin("$project");
	BSON_APPEND_INT32(projectStage, field.data(), 1);
	appendStageEnd(projectStage);
}

void Pipeline::project(const std::vector<std::string_view> &fields) {
	auto projectStage = appendStageBegin("$project");
	for (auto field: fields) {
		BSON_APPEND_INT32(projectStage, field.data(), 1);
	}
	appendStageEnd(projectStage);
}

void Pipeline::setUnion(std::string_view field, const std::vector<std::string_view> &sets) {
	bson_t unionOperator, unionArray;
	uint32_t numElements = 0;
	auto setStage = appendStageBegin("$set");
	BSON_APPEND_DOCUMENT_BEGIN(setStage, field.data(), &unionOperator);
	{
		BSON_APPEND_ARRAY_BEGIN(&unionOperator, "$setUnion", &unionArray);
		{
			for (auto setString: sets) {
				auto arrayKey = std::to_string(numElements++);
				BSON_APPEND_UTF8(&unionArray, arrayKey.c_str(), setString.data());
			}
		}
		bson_append_array_end(&unionOperator, &unionArray);
	}
	bson_append_document_end(setStage, &unionOperator);
	appendStageEnd(setStage);
}

void Pipeline::addToArray(std::string_view key, std::string_view arrayKey, std::string_view elementKey) {
	bson_t concatOperator, concatArray, concatArray1;
	auto setStage1 = appendStageBegin("$set");
	BSON_APPEND_DOCUMENT_BEGIN(setStage1, key.data(), &concatOperator);
	{
		BSON_APPEND_ARRAY_BEGIN(&concatOperator, "$concatArrays", &concatArray);
		{
			BSON_APPEND_UTF8(&concatArray, "0", arrayKey.data());
			BSON_APPEND_ARRAY_BEGIN(&concatArray, "1", &concatArray1);
			{
				BSON_APPEND_UTF8(&concatArray1, "0", elementKey.data());
			}
			bson_append_array_end(&concatArray, &concatArray1);
		}
		bson_append_array_end(&concatOperator, &concatArray);
	}
	bson_append_document_end(setStage1, &concatOperator);
	appendStageEnd(setStage1);
}

void Pipeline::matchEmptyArray(std::string_view arrayKey) {
	bson_t emptyArray;
	auto matchStage = appendStageBegin("$match");
	BSON_APPEND_ARRAY_BEGIN(matchStage, arrayKey.data(), &emptyArray);
	bson_append_array_end(matchStage, &emptyArray);
	appendStageEnd(matchStage);
}

void replaceAll(std::string &str, const std::string &from, const std::string &to) {
	size_t startPos = 0;
	while ((startPos = str.find(from, startPos)) != std::string::npos) {
		str.replace(startPos, from.length(), to);
		startPos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
}

bson_t *Pipeline::loadFromJSON(std::string_view filename, const std::map<std::string, std::string> &parameters) {
	auto resolved = URI::resolve(filename);
	// Load JSON file
	boost::property_tree::ptree pt;
	boost::property_tree::read_json(resolved, pt);

	// Convert JSON to string
	std::stringstream ss;
	boost::property_tree::write_json(ss, pt);

	// Replace placeholders with actual values
	std::string pipeline = ss.str();
	for (const auto &param: parameters) {
		replaceAll(pipeline, "${" + param.first + "}", param.second);
	}

	// Convert JSON to BSON
	bson_error_t error;
	bson_t *bson = bson_new_from_json((const uint8_t *) pipeline.c_str(), pipeline.size(), &error);

	if (!bson) {
		KB_ERROR("Error loading pipeline: {}", error.message);
		return nullptr;
	}

	return bson;
}
