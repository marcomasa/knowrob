//
// Created by daniel on 01.04.23.
//

#ifndef KNOWROB_MONGO_DOCUMENT_H
#define KNOWROB_MONGO_DOCUMENT_H

#include <mongoc.h>

namespace knowrob::mongo {
    /**
     * A scoped bson document. The memory of the bson_t* is managed by this
     * object, and is freed when the object is destroyed.
     */
    class Document {
    public:
        explicit Document(bson_t *bson) : bson_(bson) {}

        Document(const Document&) = delete;

        ~Document() { bson_destroy(bson_); }

        /**
         * @return the managed bson document
         */
        bson_t* bson() const { return bson_; }
    protected:
        bson_t *bson_;
    };

} // knowrob

#endif //KNOWROB_MONGODOCUMENT_H
