/* 
 * Copyright (c) 2020, Daniel Beßler
 * All rights reserved.
 * 
 * This file is part of KnowRob, please consult
 * https://github.com/knowrob/knowrob for license details.
 */

#include <knowrob/mongodb/Collection.h>
#include <knowrob/mongodb/MongoException.h>
#include <knowrob/mongodb/Document.h>
#include <iostream>

using namespace knowrob::mongo;

static const mongoc_insert_flags_t INSERT_NO_VALIDATE_FLAG =
		(mongoc_insert_flags_t)MONGOC_INSERT_NO_VALIDATE;
static const mongoc_update_flags_t UPDATE_NO_VALIDATE_FLAG =
		(mongoc_update_flags_t)MONGOC_UPDATE_NO_VALIDATE;


Collection::Collection(
	    const std::shared_ptr<Connection> &connection,
	    const std::string_view &databaseName,
	    const std::string_view &collectionName)
: connection_(connection),
  name_(collectionName),
  dbName_(databaseName),
  session_(nullptr)
{
	client_ = mongoc_client_pool_pop(connection_->pool_);
	coll_ = mongoc_client_get_collection(client_, dbName_.c_str(), name_.c_str());
    db_ = mongoc_client_get_database(client_, dbName_.c_str());
}

Collection::~Collection()
{
    if(session_) {
        mongoc_client_session_destroy(session_);
        session_ = nullptr;
    }
    mongoc_database_destroy(db_);
	mongoc_collection_destroy(coll_);
    mongoc_client_pool_push(connection_->pool_, client_);
}

mongoc_client_session_t* Collection::session()
{
    if(!session_) {
        bson_error_t error;
        session_ = mongoc_client_start_session (client_, nullptr, &error);
    }
    return session_;
}

void Collection::appendSession(bson_t *opts)
{
    auto s = session();
	if(s!=nullptr) {
		bson_error_t error;
		if(!mongoc_client_session_append(s, opts, &error)) {
		    throw MongoException("append_session", error);
		}
	}
}

void Collection::drop()
{
	bson_error_t err;
	if(!mongoc_collection_drop(coll_,&err)) {
		throw MongoException("drop_failed", err);
	}
}

void Collection::storeOne(const Document &document)
{
    bson_error_t err;
    if(!mongoc_collection_insert(
            coll_,
            INSERT_NO_VALIDATE_FLAG,
            document.bson(),
            nullptr,
            &err))
    {
        throw MongoException("insert_failed", err);
    }
}

void Collection::removeAll(const Document &document)
{
    remove(document, MONGOC_REMOVE_NONE);
}

void Collection::removeOne(const bson_oid_t &oid)
{
    removeOne(Document(BCON_NEW("_id", BCON_OID(&oid))));
}

void Collection::removeOne(const Document &document)
{
    remove(document, MONGOC_REMOVE_SINGLE_REMOVE);
}

void Collection::remove(const Document &document, mongoc_remove_flags_t flag)
{
    bson_error_t err;
    if(!mongoc_collection_remove(
            coll_,
            flag,
            document.bson(),
            nullptr,
            &err))
    {
        throw MongoException("collection_remove", err);
    }
}


void Collection::update(const Document &query, const Document &update, bool upsert)
{
    bson_error_t err;

    int flags = UPDATE_NO_VALIDATE_FLAG;
    if(upsert) flags |= MONGOC_UPDATE_UPSERT;

    if(!mongoc_collection_update(
            coll_,
            (mongoc_update_flags_t)flags,
            query.bson(),
            update.bson(),
            nullptr,
            &err))
    {
        throw MongoException("update_failed", err);
    }
}

void Collection::evalAggregation(const bson_t *pipeline)
{
    auto cursor = mongoc_collection_aggregate(
            coll_,
            MONGOC_QUERY_NONE,
            pipeline,
            nullptr,
            nullptr);
    // make sure cursor has no error after creation
    bson_error_t err;
    if(mongoc_cursor_error(cursor, &err)) {
        mongoc_cursor_destroy(cursor);
        throw MongoException("cursor_error", err);
    }
    // process the query (ignore results)
    const bson_t *cursor_doc;
    while(mongoc_cursor_next(cursor,&cursor_doc)) {}
    mongoc_cursor_destroy(cursor);
}

std::shared_ptr<BulkOperation> Collection::createBulkOperation()
{
    bson_t opts = BSON_INITIALIZER;
    BSON_APPEND_BOOL(&opts, "ordered", false);
    // create the bulk operation
    mongoc_bulk_operation_t *bulk =
            mongoc_collection_create_bulk_operation_with_opts(coll_, &opts);
    bson_destroy(&opts);
    return std::make_shared<BulkOperation>(bulk);
}

void Collection::createIndex_internal(const bson_t &keys)
{
    bson_error_t err;
    bson_t reply;

    char *index_name = mongoc_collection_keys_to_index_string(&keys);
    bson_t *cmd = BCON_NEW ("createIndexes", BCON_UTF8(mongoc_collection_get_name(coll_)),
                            "indexes", "[", "{",
                            "key",  BCON_DOCUMENT(&keys),
                            "name", BCON_UTF8(index_name),
                            "}",  "]");
    bool success = mongoc_database_write_command_with_opts (
            db_,
            cmd,
            nullptr /* opts */,
            &reply,
            &err);

    bson_free(index_name);
    bson_destroy(&reply);
    bson_destroy(cmd);
    if(!success) {
        throw MongoException("create_index_failed", err);
    }
}

void Collection::createAscendingIndex(const std::vector<const char*> &keys)
{
    bson_t b_keys;
    bson_init(&b_keys);
    for(auto key : keys) BSON_APPEND_INT32(&b_keys, key, 1);
    createIndex_internal(b_keys);
}

void Collection::createIndex(const std::vector<IndexKey> &keys)
{
    bson_t b_keys;
    bson_init(&b_keys);
    for(auto key : keys) {
        if(key.ascending) {
            BSON_APPEND_INT32(&b_keys, key.value, 1);
        }
        else {
            BSON_APPEND_INT32(&b_keys, key.value, -1);
        }
    }
    createIndex_internal(b_keys);
}

bool Collection::empty()
{
    bson_t* opts = BCON_NEW("limit", BCON_INT64(1));
    bson_t filter = BSON_INITIALIZER;
    bson_error_t error;
    int64_t count;

    count = mongoc_collection_count_documents(
            coll_,
            &filter,
            opts,
            nullptr,
            nullptr,
            &error);
    bson_destroy(opts);
    bson_destroy(&filter);

    return count==0;
}

