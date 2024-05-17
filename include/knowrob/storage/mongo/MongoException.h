#ifndef KNOWROB_MONGO_EXCEPTION_H
#define KNOWROB_MONGO_EXCEPTION_H

#include <mongoc.h>
#include <knowrob/storage/StorageError.h>

namespace knowrob::mongo {
    /**
     * A runtime exception that occurred when interacting with Mongo DB.
     */
    class MongoException : public StorageError {
    public:
        MongoException(const char *msg, const bson_error_t &err)
                : contextMessage_(msg),
				  bsonMessage_(err.message),
				  StorageError("[mongo] {}: {}.", msg, err.message) {}
        const std::string bsonMessage_;
        const std::string contextMessage_;
    };
}

#endif //KNOWROB_MONGO_EXCEPTION_H
