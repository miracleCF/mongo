#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kQuery

#include "mongo/platform/basic.h"
#include<iostream>

#include "mongo/s/query/cluster_client_cursor_impl_rtree_range.h"

#include "mongo/s/query/router_stage_limit.h"
#include "mongo/s/query/router_stage_merge.h"
#include "mongo/s/query/router_stage_mock.h"
#include "mongo/s/query/router_stage_remove_sortkey.h"
#include "mongo/s/query/router_stage_skip.h"
#include "mongo/stdx/memory.h"

namespace mongo {
    
//implementation of R-tree cursors
//----------------------------------------------------------------------------------------------------------------------------------------


ClusterClientCursorGuard RTreeRangeClusterClientCursorImpl::make(executor::TaskExecutor* executor,
                                                       ClusterClientCursorParams&& params) {
    std::unique_ptr<ClusterClientCursor> cursor(
        new RTreeRangeClusterClientCursorImpl(executor, std::move(params)));
    return ClusterClientCursorGuard(std::move(cursor));
}

//new make function here
ClusterClientCursorGuard RTreeRangeClusterClientCursorImpl::make(OperationContext* txn,
                                                                 string DB_NAME,
                                                                 string COLLECTION_NAME,
                                                                 mongo::BSONObj InputGeometry,
                                                                 int queryType)
{
    std::unique_ptr<ClusterClientCursor> cursor(
        new RTreeRangeClusterClientCursorImpl(txn,DB_NAME,COLLECTION_NAME,InputGeometry,queryType));
    return  ClusterClientCursorGuard(std::move(cursor));
}

RTreeRangeClusterClientCursorImpl::RTreeRangeClusterClientCursorImpl(executor::TaskExecutor* executor,
                                                 ClusterClientCursorParams&& params)
    : _isTailable(params.isTailable), _root(buildMergerPlan(executor, std::move(params))) {}

RTreeRangeClusterClientCursorImpl::RTreeRangeClusterClientCursorImpl(std::unique_ptr<RouterStageMock> root)
    : _root(std::move(root)) {}
    
RTreeRangeClusterClientCursorImpl::RTreeRangeClusterClientCursorImpl(OperationContext* txn,
                                                                      string DB_NAME,
                                                                      string COLLECTION_NAME,
                                                                      mongo::BSONObj InputGeometry,
                                                                      int queryType)
{
    if(queryType==0)//$geoIntersects
    {
        _rtreeRangeQueryCursor=IM.GeoSearchIntersects(txn,DB_NAME,COLLECTION_NAME,InputGeometry);
        _isRtreeCursorOK=true;
    }
    if(queryType==1)//$geoWithin
    {
        _rtreeRangeQueryCursor=IM.GeoSearchWithin(txn,DB_NAME,COLLECTION_NAME,InputGeometry);
        _isRtreeCursorOK=true;
    }
}

StatusWith<boost::optional<BSONObj>> RTreeRangeClusterClientCursorImpl::next() {
    // First return stashed results, if there are any.
    if (!_stash.empty()) {
        BSONObj front = std::move(_stash.front());
        _stash.pop();
        ++_numReturnedSoFar;
        return {front};
    }

    auto next = _rtreeRangeQueryCursor->Next();//unlike _root, we use Next instead of next
    if (!next.isEmpty()) {
        ++_numReturnedSoFar;
    }
    else
    {
        if(_isRtreeCursorOK)
        {
            _isRtreeCursorOK=false;
            _rtreeRangeQueryCursor->FreeCursor();
        }
    }
    return {next};
}

void RTreeRangeClusterClientCursorImpl::kill() {
    if(_isRtreeCursorOK)
    {
        _rtreeRangeQueryCursor->FreeCursor();
    }
}

bool RTreeRangeClusterClientCursorImpl::isTailable() const {
    return _isTailable;
}

long long RTreeRangeClusterClientCursorImpl::getNumReturnedSoFar() const {
    return _numReturnedSoFar;
}

void RTreeRangeClusterClientCursorImpl::queueResult(const BSONObj& obj) {
    invariant(obj.isOwned());
    _stash.push(obj);
}

//used by r-tree
void RTreeRangeClusterClientCursorImpl::setExhausted(bool isExhausted)
{
    _isExhausted = isExhausted;
}

bool RTreeRangeClusterClientCursorImpl::remotesExhausted() {
     return _isExhausted;
}

Status RTreeRangeClusterClientCursorImpl::setAwaitDataTimeout(Milliseconds awaitDataTimeout) {
    return _root->setAwaitDataTimeout(awaitDataTimeout);
}

std::unique_ptr<RouterExecStage> RTreeRangeClusterClientCursorImpl::buildMergerPlan(
    executor::TaskExecutor* executor, ClusterClientCursorParams&& params) {
    const auto skip = params.skip;
    const auto limit = params.limit;
    const bool hasSort = !params.sort.isEmpty();

    // The first stage is always the one which merges from the remotes.
    std::unique_ptr<RouterExecStage> root =
        stdx::make_unique<RouterStageMerge>(executor, std::move(params));

    if (skip) {
        root = stdx::make_unique<RouterStageSkip>(std::move(root), *skip);
    }

    if (limit) {
        root = stdx::make_unique<RouterStageLimit>(std::move(root), *limit);
    }

    if (hasSort) {
        root = stdx::make_unique<RouterStageRemoveSortKey>(std::move(root));
    }

    return root;
}


//----------------------------------------------------------------------------------------------------------------------------------------





}