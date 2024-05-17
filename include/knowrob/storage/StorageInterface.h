/*
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#ifndef KNOWROB_STORAGE_INTERFACE_H
#define KNOWROB_STORAGE_INTERFACE_H

#include "knowrob/storage/QueryableStorage.h"
#include "StorageManager.h"
#include "Transaction.h"

namespace knowrob {
	/**
	 * A high-level interface to the storage manager.
	 * It includes methods for querying and modifying the extensional database.
	 */
	class StorageInterface {
	public:
		/**
		 * The type of a transaction.
		 */
		enum TransactionType {
			Insert, Remove
		};
		/**
		 * Determines how storages are selected for a transaction.
		 */
		enum BackendSelection {
			Including, Excluding
		};

		explicit StorageInterface(const std::shared_ptr<StorageManager> &backendManager)
				: backendManager_(backendManager) {}

		/**
		 * @return the vocabulary.
		 */
		auto &vocabulary() const { return backendManager_->vocabulary(); }

		/**
		 * @return the storage manager.
		 */
		auto &backendManager() const { return backendManager_; }

		/**
		 * Creates a new transaction.
		 * @param queryable a storage used to perform any queries needed to complete the transaction.
		 * @param type the type of the transaction.
		 * @param mode determines how storages are selected for the transaction.
		 * @param backends the storages to include or exclude from the transaction.
		 * @return the transaction.
		 */
		std::shared_ptr<transaction::Transaction> createTransaction(
				const QueryableBackendPtr &queryable,
				TransactionType type,
				BackendSelection mode = Excluding,
				const std::vector<std::shared_ptr<NamedBackend>> &backends = {});

		/**
		 * Removes all triples with a given origin from all storages.
		 * @param origin the origin of the triples to remove.
		 * @return true if the triples were removed from all storages, false otherwise.
		 */
		bool removeAllWithOrigin(std::string_view origin);

		/**
		 * Inserts a triple into the extensional database and merges it with existing ones
		 * if possible.
		 * @param backend the storage to modify.
		 * @param triple the triple to insert.
		 * @return true if the triple was inserted, false otherwise.
		 */
		bool mergeInsert(const QueryableBackendPtr &backend, const FramedTriple &triple);

		/**
		 * Checks if a triple is contained in the extensional database.
		 * @param backend the storage to query.
		 * @param triple the triple to check.
		 * @return true if the triple is contained in the extensional database, false otherwise.
		 */
		bool contains(const QueryableBackendPtr &backend, const FramedTriple &triple) const;

		/**
		 * Executes a visitor on all triples in the extensional database.
		 * @param backend the backend to query.
		 * @param visitor the visitor to execute.
		 */
		static void foreach(const QueryableBackendPtr &backend, const TripleVisitor &visitor);

		/**
		 * Executes a visitor on all triples in the extensional database.
		 * @param backend the backend to query.
		 * @param callback the visitor to execute.
		 */
		static void batch(const QueryableBackendPtr &backend, const TripleHandler &callback);

		/**
		 * Evaluates a query on the extensional database and executes a visitor on the results
		 * which are returned in the form of triples.
		 * @param backend the backend to query.
		 * @param query the query to evaluate.
		 * @param visitor the visitor to execute.
		 */
		void match(const QueryableBackendPtr &backend, const FramedTriplePattern &query,
				   const TripleVisitor &visitor) const;

		/**
		 * Evaluates a query on the extensional database and executes a visitor on the results
		 * which are returned in as bindings.
		 * @param backend the backend to query.
		 * @param q the query to evaluate.
		 * @param callback the visitor to execute.
		 */
		void query(const QueryableBackendPtr &backend, const GraphQueryPtr &q, const BindingsHandler &callback) const;

		/**
		 * Evaluates a query on the extensional database and fills a token buffer with the results
		 * in a separate thread.
		 * @param backend the backend to query.
		 * @param query the query to evaluate.
		 * @return the token buffer with the results.
		 */
		TokenBufferPtr getAnswerCursor(const QueryableBackendPtr &backend, const GraphPathQueryPtr &query);

	protected:
		std::shared_ptr<StorageManager> backendManager_;

		void pushIntoCursor(const QueryableBackendPtr &backend, const GraphPathQueryPtr &query,
							const TokenBufferPtr &resultStream) const;
	};

} // knowrob

#endif //KNOWROB_STORAGE_INTERFACE_H
