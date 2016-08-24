#define MONGO_LOG_DEFAULT_COMPONENT ::mongo::logger::LogComponent::kQuery

#include "mongo/platform/basic.h"
#include<iostream>

#include "mongo/s/query/cluster_client_cursor_impl_rtree_near.h"

#include "mongo/s/query/router_stage_limit.h"
#include "mongo/s/query/router_stage_merge.h"
#include "mongo/s/query/router_stage_mock.h"
#include "mongo/s/query/router_stage_remove_sortkey.h"
#include "mongo/s/query/router_stage_skip.h"
#include "mongo/stdx/memory.h"

namespace mongo {
    
//implementation of R-tree cursors
//----------------------------------------------------------------------------------------------------------------------------------------


ClusterClientCursorGuard RTreeNearClusterClientCursorImpl::make(executor::TaskExecutor* executor,
                                                       ClusterClientCursorParams&& params) {
    std::unique_ptr<ClusterClientCursor> cursor(
        new RTreeNearClusterClientCursorImpl(executor, std::move(params)));
    return ClusterClientCursorGuard(std::move(cursor));
}

//new make function here
ClusterClientCursorGuard RTreeNearClusterClientCursorImpl::make(OperationContext* txn,
                                                                 string DB_NAME,
                                                                 string COLLECTION_NAME,
                                                                 double ctx,
                                                                 double cty,
                                                                 double rMin,
                                                                 double rMax)
{
    std::unique_ptr<ClusterClientCursor> cursor(
        new RTreeNearClusterClientCursorImpl(txn,DB_NAME,COLLECTION_NAME,ctx,cty,rMin,rMax));
    return  ClusterClientCursorGuard(std::move(cursor));
}

RTreeNearClusterClientCursorImpl::RTreeNearClusterClientCursorImpl(executor::TaskExecutor* executor,
                                                 ClusterClientCursorParams&& params)
    : _isTailable(params.isTailable), _root(buildMergerPlan(executor, std::move(params))) {}

RTreeNearClusterClientCursorImpl::RTreeNearClusterClientCursorImpl(std::unique_ptr<RouterStageMock> root)
    : _root(std::move(root)) {}
    
RTreeNearClusterClientCursorImpl::RTreeNearClusterClientCursorImpl(OperationContext* txn,
                                                                 string DB_NAME,
                                                                 string COLLECTION_NAME,
                                                                 double ctx,
                                                                 double cty,
                                                                 double rMin,
                                                                 double rMax)
{
        _rtreeGeoNearCursor=IM.GeoSearchNear(txn,DB_NAME,COLLECTION_NAME,ctx,cty,rMin,rMax);
        _isRtreeCursorOK=true;     
}

StatusWith<boost::optional<BSONObj>> RTreeNearClusterClientCursorImpl::next() {
    // First return stashed results, if there are any.
    if (!_stash.empty()) {
        BSONObj front = std::move(_stash.front());
        _stash.pop();
        ++_numReturnedSoFar;
        return {front};
    }

    auto next = _rtreeGeoNearCursor->Next();//unlike _root, we use Next instead of next
    if (!next.isEmpty()) {
        ++_numReturnedSoFar;
    }
    return {next};
}

void RTreeNearClusterClientCursorImpl::kill() {
    if(_isRtreeCursorOK)
    {
        _isRtreeCursorOK=false;
        _rtreeGeoNearCursor->FreeCursor();
    }
}

bool RTreeNearClusterClientCursorImpl::isTailable() const {
    return _isTailable;
}

long long RTreeNearClusterClientCursorImpl::getNumReturnedSoFar() const {
    return _numReturnedSoFar;
}

void RTreeNearClusterClientCursorImpl::queueResult(const BSONObj& obj) {
    invariant(obj.isOwned());
    _stash.push(obj);
}

//used by r-tree
void RTreeNearClusterClientCursorImpl::setExhausted(bool isExhausted)
{
    _isExhausted = isExhausted;
}

bool RTreeNearClusterClientCursorImpl::remotesExhausted() {
     return _isExhausted;
}

Status RTreeNearClusterClientCursorImpl::setAwaitDataTimeout(Milliseconds awaitDataTimeout) {
    return _root->setAwaitDataTimeout(awaitDataTimeout);
}

std::unique_ptr<RouterExecStage> RTreeNearClusterClientCursorImpl::buildMergerPlan(
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