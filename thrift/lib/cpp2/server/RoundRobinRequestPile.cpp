/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <utility>

#include <thrift/lib/cpp/concurrency/Thread.h>
#include <thrift/lib/cpp2/server/RoundRobinRequestPile.h>

namespace apache::thrift {

void RoundRobinRequestPile::Consumer::operator()(
    ServerRequest&& req, std::shared_ptr<folly::RequestContext>&&) {
  carrier_ = std::move(req);
}

RoundRobinRequestPile::RoundRobinRequestPile(Options opts)
    : opts_(std::move(opts)) {
  const auto& numBucketsPerPriority = opts_.numBucketsPerPriority;
  requestQueues_.reset(
      new std::unique_ptr<RequestQueue[]>[numBucketsPerPriority.size()]);
  for (unsigned i = 0; i < numBucketsPerPriority.size(); ++i) {
    requestQueues_[i].reset(new RequestQueue[numBucketsPerPriority.at(i)]);
    for (unsigned j = 0; j < numBucketsPerPriority.at(i); ++j) {
      // because we are doing round robin
      // for each round we only want to consumer
      // one item from the queue
      requestQueues_[i][j].setMaxReadAtOnce(1);
      requestQueues_[i][j].arm();
    }
  }

  retrievalIndexQueues_.reset(
      new RetrievalIndexQueue[numBucketsPerPriority.size()]);
}

ServerRequest RoundRobinRequestPile::dequeueImpl(
    unsigned pri, unsigned bucket) {
  Consumer consumer;
  auto& queue = requestQueues_[pri][bucket];
  queue.drive(consumer);

  // Schedule a callback from the consumer thread iff
  // there is something remaining in the queue
  // we only store the index of next queue
  if (!queue.arm()) {
    retrievalIndexQueues_[pri].enqueue(bucket);
  }

  return std::move(consumer.carrier_);
}

std::optional<ServerRequestRejection> RoundRobinRequestPile::enqueue(
    ServerRequest&& request) {
  unsigned pri = 0, bucket = 0;
  if (opts_.pileSelectionFunction) {
    std::tie(pri, bucket) = opts_.pileSelectionFunction(request);
  }
  DCHECK_LT(pri, opts_.numBucketsPerPriority.size());
  DCHECK_LT(bucket, opts_.numBucketsPerPriority[pri]);

  if (opts_.numMaxRequests != 0) {
    auto res = requestQueues_[pri][bucket].tryPush(
        std::move(request), opts_.numMaxRequests);
    switch (res) {
      case RequestQueue::TryPushResult::FAILED_LIMIT_REACHED: {
        ServerRequestRejection rej(AppServerException(
            "AppServerException",
            "RequestPile enqueue error : reached max limit"));
        return rej;
      }
      case RequestQueue::TryPushResult::SUCCESS_AND_ARMED: {
        retrievalIndexQueues_[pri].enqueue(bucket);
        break;
      }
      default:
        break;
    }
  } else {
    if (requestQueues_[pri][bucket].push(std::move(request))) {
      // Schedule a pull iff we are the first one to push into
      // the request queue
      retrievalIndexQueues_[pri].enqueue(bucket);
    }
  }
  return std::nullopt;
}

std::pair<std::optional<ServerRequest>, std::optional<intptr_t>>
RoundRobinRequestPile::dequeue() {
  for (unsigned i = 0; i < opts_.numBucketsPerPriority.size(); ++i) {
    if (auto bucket = retrievalIndexQueues_[i].try_dequeue()) {
      return std::make_pair(dequeueImpl(i, *bucket), std::nullopt);
    }
  }
  return std::make_pair(std::nullopt, std::nullopt);
}

uint64_t RoundRobinRequestPile::requestCount() const {
  uint64_t res = 0;
  for (size_t i = 0; i < opts_.numBucketsPerPriority.size(); ++i) {
    if (!retrievalIndexQueues_[i].empty()) {
      for (size_t j = 0; j < opts_.numBucketsPerPriority[i]; ++j) {
        res += requestQueues_[i][j].size();
      }
    }
  }

  return res;
}

} // namespace apache::thrift
