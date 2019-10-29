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

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/test/Structs.h>

#include <folly/Benchmark.h>
#include <folly/Optional.h>
#include <folly/portability/GFlags.h>
#include <glog/logging.h>

#include <vector>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace thrift::benchmark;

// The benckmark is to measure single struct use case, the iteration here is
// more like a benchmark artifact, so avoid doing optimizationon iteration
// usecase in this benchmark (e.g. move string definition out of while loop)

template <typename Serializer, typename Struct>
void writeBench(size_t iters) {
  BenchmarkSuspender susp;
  auto strct = create<Struct>();
  susp.dismiss();

  while (iters--) {
    IOBufQueue q;
    Serializer::serialize(strct, &q);
  }
  susp.rehire();
}

template <typename Serializer, typename Struct>
void readBench(size_t iters) {
  BenchmarkSuspender susp;
  auto strct = create<Struct>();
  IOBufQueue q;
  Serializer::serialize(strct, &q);
  auto buf = q.move();
  // coalesce the IOBuf chain to test fast path
  buf->coalesce();
  susp.dismiss();

  while (iters--) {
    Struct data;
    Serializer::deserialize(buf.get(), data);
  }
  susp.rehire();
}

#define X1(proto, rdwr, bench)                         \
  BENCHMARK(proto##Protocol_##rdwr##_##bench, iters) { \
    rdwr##Bench<proto##Serializer, bench>(iters);      \
  }

#define X2(proto, bench)  \
  X1(proto, write, bench) \
  X1(proto, read, bench)

#define X(proto)             \
  X2(proto, Empty)           \
  X2(proto, SmallInt)        \
  X2(proto, BigInt)          \
  X2(proto, SmallString)     \
  X2(proto, BigString)       \
  X2(proto, BigBinary)       \
  X2(proto, LargeBinary)     \
  X2(proto, Mixed)           \
  X2(proto, MixedInt)        \
  X2(proto, SmallListInt)    \
  X2(proto, BigListInt)      \
  X2(proto, BigListMixed)    \
  X2(proto, BigListMixedInt) \
  X2(proto, LargeListMixed)  \
  X2(proto, LargeMapInt)     \
  X2(proto, NestedMap)       \
  X2(proto, ComplexStruct)

X(Binary)
X(Compact)
X(Nimble)

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  runBenchmarks();
  return 0;
}
