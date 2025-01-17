//
// Created by daniel on 31.08.23.
//

#ifndef KNOWROB_IDB_STAGE_H
#define KNOWROB_IDB_STAGE_H

#include "QueryStage.h"
#include "knowrob/reasoner/Reasoner.h"

namespace knowrob {

    class IDBStage : public QueryStage {
    public:
        IDBStage(const std::shared_ptr<Reasoner> &reasoner,
                 const RDFLiteralPtr &literal,
                 const std::shared_ptr<ThreadPool> &threadPool,
                 int queryFlags);

    protected:
        std::shared_ptr<Reasoner> reasoner_;
        std::shared_ptr<ThreadPool> threadPool_;

        AnswerBufferPtr submitQuery(const RDFLiteralPtr &literal) override;
    };

} // knowrob

#endif //KNOWROB_IDB_STAGE_H
