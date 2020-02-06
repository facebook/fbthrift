/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/transport/rocket/client/RequestContextQueue.h>

#include <utility>

#include <folly/Likely.h>

namespace apache {
namespace thrift {
namespace rocket {

void RequestContextQueue::enqueueScheduledWrite(RequestContext& req) noexcept {
  DCHECK(req.state_ == State::WRITE_NOT_SCHEDULED);

  req.state_ = State::WRITE_SCHEDULED;
  writeScheduledQueue_.push_back(req);
  trackIfRequestResponse(req);
}

std::unique_ptr<folly::IOBuf>
RequestContextQueue::getNextScheduledWritesBatch() noexcept {
  std::unique_ptr<folly::IOBuf> batchBuf;

  while (!writeScheduledQueue_.empty()) {
    auto& req = writeScheduledQueue_.front();
    writeScheduledQueue_.pop_front();

    DCHECK(req.state_ == State::WRITE_SCHEDULED);
    req.state_ = State::WRITE_SENDING;
    writeSendingQueue_.push_back(req);

    if (writeScheduledQueue_.empty()) {
      req.lastInWriteBatch_ = true;
    }

    auto reqBuf = req.serializedChain();
    if (!batchBuf) {
      batchBuf = std::move(reqBuf);
    } else {
      batchBuf->prependChain(std::move(reqBuf));
    }
  }

  return batchBuf;
}

void RequestContextQueue::abortSentRequest(
    RequestContext& req,
    transport::TTransportException ex) noexcept {
  if (req.state_ == State::COMPLETE) {
    return;
  }
  DCHECK(req.state() == State::WRITE_SENT);
  DCHECK(!req.responsePayload_.hasException());
  req.responsePayload_ = folly::Try<Payload>(std::move(ex));
  markAsResponded(req);
}

// For REQUEST_RESPONSE, this is called on the read path once the entire
// response payload has arrived.
// For REQUEST_STREAM and REQUEST_FNF, this is called once the write to the
// socket has completed.
void RequestContextQueue::markAsResponded(RequestContext& req) noexcept {
  untrackIfRequestResponse(req);

  if (LIKELY(req.state() == State::WRITE_SENT)) {
    req.state_ = State::COMPLETE;
    writeSentQueue_.erase(writeSentQueue_.iterator_to(req));
    req.baton_.post();
  } else {
    // Response arrived before AsyncSocket WriteCallback fired; we let the write
    // complete. writeSuccess()/writeErr() are therefore responsible for
    // handling this request's final queue transition and posting the baton.
    DCHECK(req.isRequestResponse());
    DCHECK(req.state() == State::WRITE_SENDING);
    req.state_ = State::COMPLETE;
  }
}

void RequestContextQueue::failAllScheduledWrites(
    transport::TTransportException ex) {
  failQueue(
      writeScheduledQueue_,
      transport::TTransportException(
          transport::TTransportException::NOT_OPEN,
          fmt::format(
              "Dropping unsent request. Connection closed after: {}",
              ex.what())));
}

void RequestContextQueue::failAllSentWrites(transport::TTransportException ex) {
  failQueue(writeSentQueue_, std::move(ex));
}

void RequestContextQueue::failQueue(
    RequestContext::Queue& queue,
    transport::TTransportException ex) {
  while (!queue.empty()) {
    auto& req = queue.front();
    queue.pop_front();
    DCHECK(!req.responsePayload_.hasException());
    req.responsePayload_ = folly::Try<Payload>(ex);
    untrackIfRequestResponse(req);
    req.state_ = State::COMPLETE;
    req.baton_.post();
  }
}

RequestContext* RequestContextQueue::getRequestResponseContext(
    StreamId streamId) {
  const auto it = requestResponseContexts_.find(
      streamId,
      std::hash<StreamId>(),
      [](StreamId sid, const RequestContext& ctx) {
        return sid == ctx.streamId();
      });
  return it != requestResponseContexts_.end() ? &*it : nullptr;
}

void RequestContextQueue::growBuckets() {
  std::vector<RequestResponseSet::bucket_type> newBuckets(
      rrContextBuckets_.size() * 2);
  requestResponseContexts_.rehash(
      RequestResponseSet::bucket_traits(newBuckets.data(), newBuckets.size()));
  rrContextBuckets_.swap(newBuckets);
}

} // namespace rocket
} // namespace thrift
} // namespace apache
