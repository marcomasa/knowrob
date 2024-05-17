/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_MONGO_KNOWLEDGE_GRAPH_H
#define KNOWROB_MONGO_KNOWLEDGE_GRAPH_H

#include <optional>
#include <list>
#include "boost/property_tree/ptree.hpp"
#include "knowrob/storage/Storage.h"
#include "knowrob/storage/QueryableStorage.h"
#include "knowrob/storage/mongo/Collection.h"
#include "knowrob/queries/TokenBuffer.h"
#include "knowrob/formulas/FirstOrderLiteral.h"
#include "BindingsCursor.h"
#include "MongoTaxonomy.h"
#include "TripleCursor.h"

namespace knowrob {
	/**
	 * A knowledge graph implemented with MongoDB.
	 */
	class MongoKnowledgeGraph : public QueryableStorage {
	public:
		static const std::string DB_URI_DEFAULT;
		static const std::string DB_NAME_KNOWROB;
		static const std::string DB_NAME_TESTS;
		static const std::string COLL_NAME_TRIPLES;
		static const std::string COLL_NAME_TESTS;

		MongoKnowledgeGraph();

		/**
		 * Initialize the knowledge graph with a MongoDB URI.
		 * @param db_uri the URI string used to connect to the database.
		 * @param db_name the name of the database.
		 * @param collectionName the name of the collection for triples.
		 * @return true on success
		 */
		bool initializeBackend(std::string_view db_uri,
							   std::string_view db_name = "knowrob",
							   std::string_view collectionName = "triples");

		// Override DataBackend
		bool initializeBackend(const PropertyTree &config) override;

		/**
		 * @return the name of the database.
		 */
		const std::string &dbName() const { return tripleCollection_->dbName(); }

		/**
		 * @return the URI string used to connect to the database.
		 */
		const std::string &dbURI() const { return tripleCollection_->dbURI(); }

		/**
		 * @return true if only read operations are allowed.
		 */
		bool isReadOnly() const { return isReadOnly_; }

		/**
		 * @return the collection for triples.
		 */
		auto tripleCollection() { return tripleCollection_; }

		/**
		 * Delete all statements in the database.
		 * Note: ths will also delete all indices which need to be re-created afterwards.
		 */
		void drop();

		/**
		 * Lookup up all matching triples.
		 * @param query a triple pattern
		 * @return a getAnswerCursor over matching triples
		 */
		mongo::BindingsCursorPtr lookup(const FramedTriplePattern &query);

		/**
		 * Lookup up a path of matching triples.
		 * The lookup pipeline includes a step for each expression in the vector
		 * in the same order as the expressions are ordered in the vector.
		 * @param query a path query
		 * @return a getAnswerCursor over matching triples
		 */
		mongo::BindingsCursorPtr lookup(const GraphTerm &query);

		// Override DataBackend
		bool insertOne(const FramedTriple &triple) override;

		// Override DataBackend
		bool insertAll(const TripleContainerPtr &triples) override;

		// Override DataBackend
		bool removeOne(const FramedTriple &triple) override;

		// Override DataBackend
		bool removeAll(const TripleContainerPtr &triples) override;

		// Override DataBackend
		bool removeAllWithOrigin(std::string_view origin) override;

		// Override QueryableBackend
		bool isPersistent() const override { return true; }

		// Override QueryableBackend
		void foreach(const TripleVisitor &callback) const override;

		// Override QueryableBackend
		void batch(const TripleHandler &callback) const override;

		// override QueryableBackend
		void batchOrigin(std::string_view origin, const TripleHandler &callback) override;

		// Override QueryableBackend
		void match(const FramedTriplePattern &query, const TripleVisitor &visitor) override;

		// Override QueryableBackend
		void query(const GraphQueryPtr &query, const BindingsHandler &callback) override;

		// Override QueryableBackend
		void count(const ResourceCounter &callback) const override;

	protected:
		std::shared_ptr<mongo::Collection> tripleCollection_;
		std::shared_ptr<mongo::Collection> oneCollection_;
		std::shared_ptr<mongo::MongoTaxonomy> taxonomy_;
		bool isReadOnly_;

		static void iterate(mongo::TripleCursor &cursor, const TripleVisitor &visitor);

		void initializeMongo();

		static std::shared_ptr<mongo::Collection> connect(const boost::property_tree::ptree &config);

		static std::shared_ptr<mongo::Collection>
		connect(std::string_view uri, std::string_view db, std::string_view collection);

		static std::string getDBName(const boost::property_tree::ptree &config);

		static std::string getCollectionName(const boost::property_tree::ptree &config);

		static std::string getURI(const boost::property_tree::ptree &config);

		bool dropOrigin(std::string_view origin);
	};

} // knowrob::mongo

#endif //KNOWROB_MONGO_KNOWLEDGE_GRAPH_H
