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

#include <atomic>
#include <limits>
#include <memory>

#include <folly/concurrency/UnboundedQueue.h>
#include <folly/io/async/AtomicNotificationQueue.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/server/RequestPileInterface.h>

namespace apache::thrift {

class RoundRobinRequestPile : public RequestPileInterface {
 public:
  using PileSelectionFunction =
      std::function<std::pair<unsigned, unsigned>(const ServerRequest&)>;

  struct Options {
    inline constexpr static unsigned kDefaultNumPriorities = 5;
    inline constexpr static unsigned kDefaultNumBuckets = 1;

    // numBucket = numBuckets_[priority]
    std::vector<unsigned> numBucketsPerPriority;

    // 0 means no limit
    uint64_t numMaxRequests{0};

    // default implementation
    PileSelectionFunction pileSelectionFunction{
        [](const ServerRequest& request) -> std::pair<unsigned, unsigned> {
          auto ctx = request.requestContext();
          auto callPriority = ctx->getCallPriority();
          auto res = callPriority == concurrency::N_PRIORITIES
              ? concurrency::NORMAL
              : callPriority;
          return std::make_pair(static_cast<uint64_t>(res), 0);
        }};

    Options() {
      numBucketsPerPriority.reserve(kDefaultNumPriorities);
      for (unsigned i = 0; i < kDefaultNumPriorities; ++i) {
        numBucketsPerPriority.emplace_back(kDefaultNumBuckets);
      }
    }

    void setNumPriorities(unsigned numPri) {
      numBucketsPerPriority.resize(numPri);
    }

    void setBucketSizeForPriority(unsigned numPri, unsigned numBucket) {
      numBucketsPerPriority[numPri] = numBucket;
    }

    // This function sets the shape of the RequestPile
    // e.g. This is setting the number of buckets per priority
    // If your RequestPile has 3 priorities and each has 2 buckets
    // you should pass in {2,2,2}
    void setShape(std::vector<unsigned> shape) {
      numBucketsPerPriority = std::move(shape);
    }

    void setPileSelectionFunction(PileSelectionFunction func) {
      pileSelectionFunction = std::move(func);
    }
  };

  // The default number of buckets for each priority is 1
  explicit RoundRobinRequestPile(Options opts);

  ~RoundRobinRequestPile() override = default;

  std::optional<ServerRequestRejection> enqueue(
      ServerRequest&& request) override;

  std::pair<
      std::optional<ServerRequest>,
      std::optional<RequestPileInterface::UserData>>
  dequeue() override;

  uint64_t requestCount() const override;

  void onRequestFinished(intptr_t /* userData */) override {}

 private:
  using RetrievalIndexQueue =
      folly::UMPMCQueue<unsigned, /* MayBlock  */ false>;

  // The reason why we chose AtomicNotificationQueue instead of two UMPMCQueue
  // is that UMPMCQueue does not provide an enqueue() interface that indicates
  // which one is the first enqueue to the empty queue
  using RequestQueue = folly::AtomicNotificationQueue<ServerRequest>;

  // the consumer class used by the AtomicNotificationQueue
  class Consumer {
   public:
    // this operation simply put the retrieved item into the temporary
    // carrier for the caller to extract
    void operator()(
        ServerRequest&& req, std::shared_ptr<folly::RequestContext>&&);

    ServerRequest carrier_;
  };

  Options opts_;
  // The reason why we chose unique_ptr managed array
  // instead of std containers like vector is that
  // the queues are not movable / move constructable
  // which is necessary for standard container
  std::unique_ptr<std::unique_ptr<RequestQueue[]>[]> requestQueues_;
  std::unique_ptr<RetrievalIndexQueue[]> retrievalIndexQueues_;

  ServerRequest dequeueImpl(unsigned pri, unsigned bucket);
};

// RoundRobinRequestPile is the default request pile used by most of
// the thrift servers
using StandardRequestPile = RoundRobinRequestPile;

} // namespace apache::thrift
