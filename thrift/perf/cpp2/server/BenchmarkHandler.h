/*
 * Copyright 2017-present Facebook, Inc.
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

#include <folly/system/ThreadName.h>
#include <rsocket/internal/ScheduledSubscriber.h>
#include <thrift/perf/cpp2/if/gen-cpp2/StreamBenchmark.h>
#include <thrift/perf/cpp2/util/QPSStats.h>

DEFINE_uint32(chunk_size, 1024, "Number of bytes per chunk");
DEFINE_uint32(batch_size, 16, "Flow control batch size");

namespace facebook {
namespace thrift {
namespace benchmarks {

using apache::thrift::HandlerCallback;
using apache::thrift::HandlerCallbackBase;

class BenchmarkHandler : virtual public StreamBenchmarkSvIf {
 public:
  explicit BenchmarkHandler(QPSStats* stats) : stats_(stats) {
    stats_->registerCounter(kNoop_);
    stats_->registerCounter(kSum_);
    stats_->registerCounter(kTimeout_);
    stats->registerCounter(kDownload_);
    stats->registerCounter(kUpload_);
    stats_->registerCounter(ks_Download_);
    stats_->registerCounter(ks_Upload_);

    chunk_.data.unshare();
    chunk_.data.reserve(0, FLAGS_chunk_size);
    auto buffer = chunk_.data.writableData();
    // Make it real data to eliminate network optimizations on sending all 0's.
    srand(time(nullptr));
    for (uint32_t i = 0; i < FLAGS_chunk_size; ++i) {
      buffer[i] = (uint8_t)(rand() % 26 + 'A');
    }
    chunk_.data.append(FLAGS_chunk_size);
  }

  void async_eb_noop(std::unique_ptr<HandlerCallback<void>> callback) override {
    stats_->add(kNoop_);
    callback->done();
  }

  void async_eb_onewayNoop(std::unique_ptr<HandlerCallbackBase>) override {
    stats_->add(kNoop_);
  }

  // Make the async/worker thread sleep
  void timeout() override {
    std::this_thread::sleep_for(std::chrono::milliseconds(1));
    stats_->add(kTimeout_);
  }

  void async_eb_sum(
      std::unique_ptr<HandlerCallback<std::unique_ptr<TwoInts>>> callback,
      std::unique_ptr<TwoInts> input) override {
    stats_->add(kSum_);
    auto result = std::make_unique<TwoInts>();
    result->x = input->x + input->y;
    result->__isset.x = true;
    result->y = input->x - input->y;
    result->__isset.y = true;
    callback->result(std::move(result));
  }

  void download(::facebook::thrift::benchmarks::Chunk2& result) override {
    stats_->add(kUpload_);
    result = chunk_;
  }

  void upload(std::unique_ptr<Chunk2>) override {
    stats_->add(kDownload_);
  }

  apache::thrift::Stream<Chunk2> streamUploadDownload(
      apache::thrift::Stream<Chunk2> input) override {
    input->subscribe( // next
        [this](const Chunk2&) { stats_->add(ks_Download_); },
        FLAGS_batch_size);

    class Subscription : public yarpl::flowable::Subscription {
     public:
      Subscription(QPSStats* stats) : stats_(stats) {
        stats_->registerCounter(ks_Request_);
      }

      void request(int64_t cnt) override {
        // not the amount of requests but number of requests!
        stats_->add(ks_Request_);
        requested_ += cnt;
      }
      void cancel() override {
        requested_ = -1;
      }

      std::atomic<int32_t> requested_{0};
      std::string ks_Request_ = "s_request";
      QPSStats* stats_;
    };

    return yarpl::flowable::Flowable<Chunk2>::fromPublisher(
        [this, input = std::move(input)](auto subscriber) mutable {
          if (FLAGS_chunk_size > 0) {
            auto subscription = yarpl::make_ref<Subscription>(stats_);
            subscriber->onSubscribe(subscription);

            subscriber = yarpl::make_ref<rsocket::ScheduledSubscriber<Chunk2>>(
                subscriber, *folly::EventBaseManager::get()->getEventBase());
            std::thread([subscriber, subscription, this]() {
              int32_t requested = 0;
              while ((requested = subscription->requested_) != -1) {
                if (requested == 0) {
                  std::this_thread::sleep_for(std::chrono::milliseconds(1));
                } else {
                  subscriber->onNext(chunk_);
                  --subscription->requested_;
                  stats_->add(ks_Upload_);
                }
              }
              subscriber->onComplete();
            })
                .detach();
          }
        });
  }

 private:
  QPSStats* stats_;
  std::string kNoop_ = "noop";
  std::string kSum_ = "sum";
  std::string kTimeout_ = "timeout";
  std::string kDownload_ = "download";
  std::string kUpload_ = "upload";
  std::string ks_Download_ = "s_download";
  std::string ks_Upload_ = "s_upload";
  Chunk2 chunk_;
};

} // namespace benchmarks
} // namespace thrift
} // namespace facebook
