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

#include <folly/Benchmark.h>
#include <folly/Portability.h>
#include <folly/init/Init.h>

#include <thrift/lib/cpp2/transport/rocket/payload/ChecksumPayloadSerializerStrategy.h>
#include <thrift/lib/cpp2/transport/rocket/payload/DefaultPayloadSerializerStrategy.h>
#include <thrift/lib/cpp2/transport/rocket/payload/PayloadSerializer.h>

namespace apache::thrift::rocket {
namespace {

FOLLY_NOINLINE PayloadSerializer makeSerializer(bool supportsChecksum) {
  if (supportsChecksum) {
    return PayloadSerializer(
        ChecksumPayloadSerializerStrategy<DefaultPayloadSerializerStrategy>());
  }
  return PayloadSerializer(DefaultPayloadSerializerStrategy());
}

FOLLY_NOINLINE bool supportsChecksum(PayloadSerializer& serializer) {
  return serializer.supportsChecksum();
}

void benchmarkSupportsChecksum(size_t iters, bool expected) {
  folly::BenchmarkSuspender suspender;
  auto serializer = makeSerializer(expected);
  suspender.dismiss();

  while (iters-- != 0) {
    const bool actual = supportsChecksum(serializer);
    folly::doNotOptimizeAway(actual);
  }
}

BENCHMARK(supportsChecksumDefault, iters) {
  benchmarkSupportsChecksum(iters, false);
}

BENCHMARK(supportsChecksumChecksum, iters) {
  benchmarkSupportsChecksum(iters, true);
}

} // namespace
} // namespace apache::thrift::rocket

int main(int argc, char** argv) {
  folly::Init init(&argc, &argv);
  folly::runBenchmarks();
}
