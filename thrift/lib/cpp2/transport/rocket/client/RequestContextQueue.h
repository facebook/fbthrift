/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <vector>

#include <folly/ExceptionWrapper.h>
#include <folly/Likely.h>

#include <thrift/lib/cpp2/transport/rocket/client/RequestContext.h>

namespace apache {
namespace thrift {
namespace rocket {

class RequestContextQueue {
 public:
  RequestContextQueue() = default;

  void enqueueScheduledWrite(RequestContext& req) noexcept;

  RequestContext& markNextScheduledWriteAsSending() noexcept;
  size_t scheduledWriteQueueSize() const noexcept {
    return writeScheduledQueue_.size();
  }

  RequestContext& markNextSendingAsSent() noexcept;
  RequestContext& peekNextSending() noexcept {
    return writeSendingQueue_.front();
  }

  void abortSentRequest(RequestContext& req) noexcept;

  void markAsResponded(RequestContext& req) noexcept;

  bool hasInflightRequests() const noexcept {
    return !writeSendingQueue_.empty() || !writeSentQueue_.empty();
  }

  void failAllScheduledWrites(folly::exception_wrapper ew);
  void failAllSentWrites(folly::exception_wrapper ew);

  void trackAsExpectingResponse(RequestContext& req);
  RequestContext* getTrackedContext(StreamId streamId);

 private:
  using ExpectingResponseSet = RequestContext::UnorderedSet;

  static constexpr size_t kDefaultNumBuckets = 100;
  std::vector<ExpectingResponseSet::bucket_type> trackedContextBuckets_{
      kDefaultNumBuckets};

  // Only contexts for requests expecting a matching response are ever
  // inserted/looked up in this set. This includes REQUEST_RESPONSE requests as
  // well as REQUEST_STREAM requests for which a timely first payload is
  // expected, as dictated by usage of ctx.waitForResponse().
  ExpectingResponseSet trackedContexts_{
      ExpectingResponseSet::bucket_traits{trackedContextBuckets_.data(),
                                          trackedContextBuckets_.size()}};

  using State = RequestContext::State;

  // Requests for which AsyncSocket::writev() has not been called yet
  RequestContext::Queue writeScheduledQueue_;
  // Requests for which AsyncSocket::writev() has been called but completion
  // of the write to the underlying transport (successful or otherwise) is
  // still pending.
  RequestContext::Queue writeSendingQueue_;
  // Once the attempt to write a request to the socket is complete, the
  // request moves to this queue. Note that the request flows into this queue
  // even if the write to the socket failed, i.e., regardless of whether
  // writeSuccess() or writeErr() was called.
  RequestContext::Queue writeSentQueue_;

  void failQueue(RequestContext::Queue& queue, folly::exception_wrapper ew);

  void untrackIfExpectingResponse(RequestContext& req);
  void growBuckets();
};

} // namespace rocket
} // namespace thrift
} // namespace apache
