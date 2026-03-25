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

#include <sys/resource.h>
#include <chrono>

#include <glog/logging.h>

#include <folly/Benchmark.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/Collect.h>
#include <folly/coro/Task.h>
#include <folly/init/Init.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/server/ServerModule.h>
#include <thrift/lib/cpp2/server/ServiceInterceptor.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/server/test/gen-cpp2/ServiceInterceptor_clients.h>
#include <thrift/lib/cpp2/server/test/gen-cpp2/ServiceInterceptor_handlers.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace apache::thrift;
using namespace apache::thrift::test;

DEFINE_int32(concurrent, 32, "Number of concurrent streaming RPCs");
DEFINE_int32(payloads, 100000, "Payloads per stream per benchmark iteration");
DEFINE_int32(io_threads, 2, "Number of IO threads for the server");
DEFINE_int32(cpu_threads, 4, "Number of CPU threads for the server");

namespace {

// ---------------------------------------------------------------------------
// CPU measurement
// ---------------------------------------------------------------------------

double getCpuSecs() {
  struct rusage ru{};
  getrusage(RUSAGE_SELF, &ru);
  return (ru.ru_utime.tv_sec + ru.ru_utime.tv_usec / 1e6) +
      (ru.ru_stime.tv_sec + ru.ru_stime.tv_usec / 1e6);
}

// ---------------------------------------------------------------------------
// Handler
// ---------------------------------------------------------------------------

struct BenchHandler : apache::thrift::ServiceHandler<ServiceInterceptorTest> {
  folly::coro::Task<void> co_noop() override { co_return; }

  folly::coro::Task<std::unique_ptr<std::string>> co_echo(
      std::unique_ptr<std::string> str) override {
    co_return std::move(str);
  }

  folly::coro::Task<std::unique_ptr<std::string>> co_requestArgs(
      std::int32_t,
      std::unique_ptr<std::string>,
      std::unique_ptr<RequestArgsStruct>) override {
    co_return std::make_unique<std::string>("v");
  }

  folly::coro::Task<std::unique_ptr<ResponseArgsStruct>> co_echoStruct(
      std::unique_ptr<RequestArgsStruct> req) override {
    auto r = std::make_unique<ResponseArgsStruct>();
    r->foo() = *req->foo();
    r->bar() = *req->bar();
    co_return r;
  }

  void async_eb_echo_eb(
      HandlerCallbackPtr<std::unique_ptr<std::string>> cb,
      std::unique_ptr<std::string> str) override {
    cb->result(*std::move(str));
  }

  folly::coro::Task<TileAndResponse<SampleInteractionIf, void>>
  co_createInteraction() override {
    class Impl : public SampleInteractionIf {
      folly::coro::Task<std::unique_ptr<std::string>> co_echo(
          std::unique_ptr<std::string> s) override {
        co_return std::move(s);
      }
    };
    co_return {std::make_unique<Impl>()};
  }

  std::unique_ptr<SampleInteraction2If> createSampleInteraction2() override {
    class Impl : public SampleInteraction2If {
      folly::coro::Task<std::unique_ptr<std::string>> co_echo(
          std::unique_ptr<std::string> s) override {
        co_return std::move(s);
      }
    };
    return std::make_unique<Impl>();
  }

  apache::thrift::ServerStream<std::int32_t> sync_iota(
      std::int32_t start) override {
    return folly::coro::co_invoke( // NOLINT(facebook-folly-coro-return-captures-local-var)
        [current =
             start]() mutable -> folly::coro::AsyncGenerator<std::int32_t&&> {
          while (true) {
            co_yield current++;
          }
        });
  }

  folly::coro::Task<void> co_fireAndForget(std::int32_t) override { co_return; }
};

// ---------------------------------------------------------------------------
// Interceptors
// ---------------------------------------------------------------------------

struct StreamAwareAsyncInterceptor
    : public ServiceInterceptor<folly::Unit, folly::Unit> {
  explicit StreamAwareAsyncInterceptor(std::string n) : name_(std::move(n)) {}
  std::string getName() const override { return name_; }
  bool supportsStreamInterception() const override { return true; }

  folly::coro::Task<std::optional<folly::Unit>> onRequest(
      folly::Unit*, RequestInfo) override {
    co_return std::nullopt;
  }

  folly::coro::Task<void> onStreamBegin(folly::Unit*, StreamInfo) override {
    co_return;
  }

  folly::coro::Task<void> onStreamPayload(
      folly::Unit*, StreamPayloadInfo) override {
    co_return;
  }

  folly::coro::Task<void> onStreamEnd(folly::Unit*, StreamEndInfo) override {
    co_return;
  }

 private:
  std::string name_;
};

using InterceptorList = std::vector<std::shared_ptr<ServiceInterceptorBase>>;

class BenchModule : public ServerModule {
 public:
  explicit BenchModule(InterceptorList interceptors)
      : interceptors_(std::move(interceptors)) {}

  std::string getName() const override { return "BenchModule"; }
  InterceptorList getServiceInterceptors() override { return interceptors_; }

 private:
  InterceptorList interceptors_;
};

// ---------------------------------------------------------------------------
// Benchmark helper
// ---------------------------------------------------------------------------

// Holds a running server + clients. Created once per benchmark, reused
// across folly::Benchmark iterations.
struct BenchFixture {
  std::shared_ptr<BenchHandler> handler;
  std::unique_ptr<ScopedServerInterfaceThread> runner;
  std::vector<std::unique_ptr<apache::thrift::Client<ServiceInterceptorTest>>>
      clients;

  explicit BenchFixture(InterceptorList interceptors) {
    handler = std::make_shared<BenchHandler>();
    runner = std::make_unique<ScopedServerInterfaceThread>(
        handler, [&](ThriftServer& server) {
          server.setNumIOWorkerThreads(FLAGS_io_threads);
          server.setNumCPUWorkerThreads(FLAGS_cpu_threads);
          if (!interceptors.empty()) {
            server.addModule(
                std::make_unique<BenchModule>(std::move(interceptors)));
          }
        });

    for (int i = 0; i < FLAGS_concurrent; ++i) {
      clients.push_back(
          runner->newClient<apache::thrift::Client<ServiceInterceptorTest>>(
              nullptr, [](folly::AsyncSocket::UniquePtr socket) {
                return RocketClientChannel::newChannel(std::move(socket));
              }));
    }

    // Warmup
    folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
      std::vector<folly::coro::Task<void>> warmups;
      warmups.reserve(FLAGS_concurrent);
      for (int i = 0; i < FLAGS_concurrent; ++i) {
        warmups.push_back([](auto* client) -> folly::coro::Task<void> {
          auto stream = (co_await client->co_iota(0)).toAsyncGenerator();
          for (int p = 0; p < 100; ++p) {
            co_await stream.next();
          }
        }(clients[i].get()));
      }
      co_await folly::coro::collectAllRange(std::move(warmups));
    }());
  }

  // Run a sustained burst: FLAGS_payloads per stream across
  // FLAGS_concurrent streams. Returns CPU time consumed (seconds).
  double run() {
    double cpuBefore = getCpuSecs();
    folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
      std::vector<folly::coro::Task<void>> tasks;
      tasks.reserve(FLAGS_concurrent);
      for (int i = 0; i < FLAGS_concurrent; ++i) {
        tasks.push_back(
            [](auto* client, int numPayloads) -> folly::coro::Task<void> {
              auto stream = (co_await client->co_iota(0)).toAsyncGenerator();
              for (int p = 0; p < numPayloads; ++p) {
                co_await stream.next();
              }
            }(clients[i].get(), FLAGS_payloads));
      }
      co_await folly::coro::collectAllRange(std::move(tasks));
    }());
    return getCpuSecs() - cpuBefore;
  }
};

} // namespace

// ---------------------------------------------------------------------------
// folly::Benchmark definitions
//
// Each iteration runs FLAGS_concurrent streams x FLAGS_payloads payloads.
// The "cpu_ns" counter reports CPU time (getrusage) per payload,
// which captures interceptor overhead that wall time misses.
// ---------------------------------------------------------------------------

BENCHMARK_COUNTERS(StreamBaseline, counters, n) {
  BenchFixture* fixture = nullptr;
  BENCHMARK_SUSPEND {
    static BenchFixture f({});
    fixture = &f;
  }
  double cpuTotal = 0;
  for (unsigned i = 0; i < n; ++i) {
    cpuTotal += fixture->run();
  }
  int64_t totalPayloads =
      static_cast<int64_t>(n) * FLAGS_concurrent * FLAGS_payloads;
  counters["cpu/p"] = folly::UserMetric(
      cpuTotal / totalPayloads, folly::UserMetric::Type::TIME);
}

BENCHMARK_COUNTERS_RELATIVE(StreamAsyncInterceptorX1, counters, n) {
  BenchFixture* fixture = nullptr;
  BENCHMARK_SUSPEND {
    static BenchFixture f(
        {std::make_shared<StreamAwareAsyncInterceptor>("async-1")});
    fixture = &f;
  }
  double cpuTotal = 0;
  for (unsigned i = 0; i < n; ++i) {
    cpuTotal += fixture->run();
  }
  int64_t totalPayloads =
      static_cast<int64_t>(n) * FLAGS_concurrent * FLAGS_payloads;
  counters["cpu/p"] = folly::UserMetric(
      cpuTotal / totalPayloads, folly::UserMetric::Type::TIME);
}

BENCHMARK_COUNTERS_RELATIVE(StreamAsyncInterceptorX3, counters, n) {
  BenchFixture* fixture = nullptr;
  BENCHMARK_SUSPEND {
    static BenchFixture f(
        {std::make_shared<StreamAwareAsyncInterceptor>("async-1"),
         std::make_shared<StreamAwareAsyncInterceptor>("async-2"),
         std::make_shared<StreamAwareAsyncInterceptor>("async-3")});
    fixture = &f;
  }
  double cpuTotal = 0;
  for (unsigned i = 0; i < n; ++i) {
    cpuTotal += fixture->run();
  }
  int64_t totalPayloads =
      static_cast<int64_t>(n) * FLAGS_concurrent * FLAGS_payloads;
  counters["cpu/p"] = folly::UserMetric(
      cpuTotal / totalPayloads, folly::UserMetric::Type::TIME);
}

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
