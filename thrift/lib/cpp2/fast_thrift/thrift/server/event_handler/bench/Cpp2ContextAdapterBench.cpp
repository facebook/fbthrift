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

#include <thrift/lib/cpp2/fast_thrift/thrift/server/event_handler/Cpp2ContextAdapter.h>

#include <cstddef>
#include <optional>
#include <string>
#include <utility>

#include <folly/Benchmark.h>
#include <folly/init/Init.h>

namespace apache::thrift::fast_thrift::thrift::server {
namespace {

const ExtensionLayout& bridgeLayout() {
  static const ExtensionLayout layout = [] {
    ExtensionLayoutBuilder builder;
    builder.add(Cpp2BridgeExtension::kId);
    return std::move(builder).build();
  }();
  return layout;
}

class AdapterBenchFixture {
 public:
  explicit AdapterBenchFixture(std::size_t headerCount) {
    request_.installExtensions(bridgeLayout());
    ThriftRequestContext::HeaderMap headers;
    headers.reserve(headerCount);
    for (std::size_t i = 0; i < headerCount; ++i) {
      headers.emplace(
          "benchmark_header_" + std::to_string(i),
          "benchmark_value_that_exceeds_sso_" + std::to_string(i));
    }
    request_.setHeaders(std::move(headers));
  }

  void run() {
    cpp2Request_.emplace(&connection_, &header_, "get");
    {
      Cpp2RequestContextAdapter adapter(*cpp2Request_, header_, request_);
      folly::doNotOptimizeAway(adapter.get().getHeader()->getHeaders().size());
    }
    request_.setState<Cpp2BridgeExtension>(nullptr);
    cpp2Request_.reset();
  }

 private:
  apache::thrift::Cpp2ConnContext connection_;
  apache::thrift::transport::THeader header_;
  ThriftRequestContext request_;
  std::optional<apache::thrift::Cpp2RequestContext> cpp2Request_;
};

void runAdapterBenchmark(std::size_t iters, std::size_t headerCount) {
  folly::BenchmarkSuspender suspender;
  AdapterBenchFixture fixture(headerCount);
  suspender.dismiss();

  while (iters-- > 0) {
    fixture.run();
  }
}

BENCHMARK(Adapter_NoHeaders, iters) {
  runAdapterBenchmark(iters, 0);
}

BENCHMARK(Adapter_OneHeader, iters) {
  runAdapterBenchmark(iters, 1);
}

BENCHMARK(Adapter_FourHeaders, iters) {
  runAdapterBenchmark(iters, 4);
}

BENCHMARK(Adapter_SixteenHeaders, iters) {
  runAdapterBenchmark(iters, 16);
}

} // namespace
} // namespace apache::thrift::fast_thrift::thrift::server

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);
  folly::runBenchmarks();
  return 0;
}
