/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include "string"
#include "knowrob/db/mongo/Pipeline.h"
#include "knowrob/Logger.h"
#include "knowrob/URI.h"

using namespace knowrob::mongo::aggregation;

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

bson_t *Pipeline::appendStageBegin(const char *stageOperatorString) {
	auto arrayKey = std::to_string(numStages_++);
	bson_wrapper &stage = stages_.emplace_back();
	bson_wrapper &stageOperator = stageOperators_.emplace_back();
	BSON_APPEND_DOCUMENT_BEGIN(arrayDocument_, arrayKey.c_str(), &stage.bson);
	BSON_APPEND_DOCUMENT_BEGIN(&stage.bson, stageOperatorString, &stageOperator.bson);
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

void Pipeline::limit(uint32_t maxDocuments) {
	auto unwindStage = appendStageBegin();
	BSON_APPEND_INT32(unwindStage, "$limit", maxDocuments);
	appendStageEnd(unwindStage);
}

void Pipeline::unwind(const std::string_view &field) {
	auto unwindStage = appendStageBegin();
	BSON_APPEND_UTF8(unwindStage, "$unwind", field.data());
	appendStageEnd(unwindStage);
}

void Pipeline::unset(const std::string_view &field) {
	auto unwindStage = appendStageBegin();
	BSON_APPEND_UTF8(unwindStage, "$unset", field.data());
	appendStageEnd(unwindStage);
}

void Pipeline::replaceRoot(const std::string_view &newRootField) {
	auto unwindStage = appendStageBegin("$replaceRoot");
	BSON_APPEND_UTF8(unwindStage, "newRoot", newRootField.data());
	appendStageEnd(unwindStage);
}

void Pipeline::sortAscending(const std::string_view &field) {
	auto sortStage = appendStageBegin("$sort");
	BSON_APPEND_INT32(sortStage, field.data(), 1);
	appendStageEnd(sortStage);
}

void Pipeline::sortDescending(const std::string_view &field) {
	auto sortStage = appendStageBegin("$sort");
	BSON_APPEND_INT32(sortStage, field.data(), -1);
	appendStageEnd(sortStage);
}

void Pipeline::merge(const std::string_view &collection) {
	auto unwindStage = appendStageBegin("$merge");
	BSON_APPEND_UTF8(unwindStage, "into", collection.data());
	BSON_APPEND_UTF8(unwindStage, "on", "_id");
	BSON_APPEND_UTF8(unwindStage, "whenMatched", "merge");
	appendStageEnd(unwindStage);
}

void Pipeline::project(const std::string_view &field) {
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

void Pipeline::setUnion(const std::string_view &field, const std::vector<std::string_view> &sets) {
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

void Pipeline::addToArray(const std::string_view &key, const std::string_view &arrayKey,
						  const std::string_view &elementKey) {
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

void replaceAll(std::string &str, const std::string &from, const std::string &to) {
	size_t startPos = 0;
	while ((startPos = str.find(from, startPos)) != std::string::npos) {
		str.replace(startPos, from.length(), to);
		startPos += to.length(); // Handles case where 'to' is a substring of 'from'
	}
}

bson_t* Pipeline::loadFromJSON(std::string_view filename, const std::map<std::string, std::string> &parameters) {
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
