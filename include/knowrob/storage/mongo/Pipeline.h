/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_MONGO_PIPELINE_H
#define KNOWROB_MONGO_PIPELINE_H

#include <mongoc.h>
#include <list>
#include <map>
#include <string_view>
#include "bson-helper.h"
#include "knowrob/triples/GraphTerm.h"
#include "knowrob/triples/FramedTriplePattern.h"
#include "knowrob/triples/GraphBuiltin.h"
#include "TripleStore.h"
#include "knowrob/triples/GraphUnion.h"

namespace knowrob::mongo {
	/**
	 * A aggregation pipeline.
	 */
	class Pipeline {
	public:
		/**
		 * @param arrayDocument an initialized array document.
		 */
		explicit Pipeline(bson_t *arrayDocument = nullptr);

		/**
		 * @return the pipeline array.
		 */
		auto *arrayDocument() const { return arrayDocument_; }

		auto isNested() const { return isNested_; }

		void setIsNested(bool isNested) { isNested_ = isNested; }

		bson_t *appendStageBegin();

		bson_t *appendStageBegin(std::string_view stageOperator);

		void appendStageEnd(bson_t *stage);

		void append(const knowrob::GraphTerm &query, const TripleStore &tripleStore);

		void append(const knowrob::FramedTriplePattern &query, const TripleStore &tripleStore);

		void appendBuiltin(const knowrob::GraphBuiltin &builtin);

		/**
		 * Append a $limit stage.
		 * @param maxDocuments limit of resulting documents.
		 */
		void limit(uint32_t maxDocuments);

		/**
		 * Append a $unwind stage.
		 * @param field an array value
		 * @param preserveNullAndEmptyArrays if true, if the field is null or empty, $unwind outputs the document.
		 */
		void unwind(std::string_view field, bool preserveNullAndEmptyArrays = false);

		/**
		 * Append a $unset stage.
		 * @param field a document field
		 */
		void unset(std::string_view field);

		/**
		 * Append a $project stage.
		 * @param field a document field to include in output documents
		 */
		void project(std::string_view field);

		/**
		 * Append a $project stage.
		 * @param fields a list of document fields to include in output documents
		 */
		void project(const std::vector<std::string_view> &fields);

		/**
		 * Append a $replaceRoot stage.
		 * @param newRootField a document field
		 */
		void replaceRoot(std::string_view newRootField);

		/**
		 * Append a $merge stage.
		 * @param collection the output collection
		 */
		void merge(std::string_view collection);

		/**
		 * Append a $sort stage with ascending sort order.
		 * @param field a document field
		 */
		void sortAscending(std::string_view field);

		/**
		 * Append a $sort stage with descending sort order.
		 * @param field a document field
		 */
		void sortDescending(std::string_view field);

		/**
		 * Append a ($set o $setUnion) stage.
		 * @param field a field to store the union of array
		 * @param sets list of array values
		 */
		void setUnion(std::string_view field, const std::vector<std::string_view> &sets);

		/**
		 * Add an element to an array.
		 * @param key the output filed
		 * @param arrayKey input array field
		 * @param elementKey field of an additional element
		 */
		void addToArray(std::string_view key, std::string_view arrayKey, std::string_view elementKey);

		/**
		 * Match an empty array.
		 * @param arrayKey the array field
		 */
		void matchEmptyArray(std::string_view arrayKey);

		/**
		 * Load a pipeline from a JSON file.
		 * @param filename the file name
		 * @param parameters a map of parameters
		 * @return a pipeline
		 */
		static bson_t *loadFromJSON(std::string_view filename, const std::map<std::string, std::string> &parameters);

	protected:
		bson_t *arrayDocument_;
		uint32_t numStages_;
		std::list<bson_wrapper> stages_;
		std::list<bson_wrapper> stageOperators_;
		bool isNested_ = false;

		bson_t *lastStage_;
		bson_t *lastOperator_;

		void appendTerm_recursive(const knowrob::GraphTerm &query, const TripleStore &tripleStore,
								  std::set<std::string_view> &groundedVariables);

		void matchBinary(const knowrob::GraphBuiltin &builtin, std::string_view predicate);

		void setAccumulated(const knowrob::GraphBuiltin &builtin, std::string_view predicate);

		void bindValue(const knowrob::GraphBuiltin &builtin);

		void appendUnion(const knowrob::GraphUnion &unionTerm,
						 const TripleStore &tripleStore,
						 std::set<std::string_view> &groundedVariables);
	};

} // knowrob

#endif //KNOWROB_MONGO_PIPELINE_H
