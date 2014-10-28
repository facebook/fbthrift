/*
 * Copyright 2014 Facebook, Inc.
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

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/test/gen-cpp2/ProtocolBenchData_types.h>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <folly/Benchmark.h>
#include <folly/Format.h>
#include <folly/Optional.h>

#include <vector>

using namespace std;
using namespace folly;;
using namespace apache::thrift;
using namespace ::cpp2;

// Only declaration, no definition.
// This will error if used with a type other than the explicit
// instantiations below.
template <typename Struct>
Struct create();

template <> Empty create<Empty>() {
  return Empty();
}

template <> SmallInt create<SmallInt>() {
  return SmallInt(FRAGILE, 5);
}

template <> BigInt create<BigInt>() {
  return BigInt(FRAGILE, 0x1234567890abcdefL);
}

template <> SmallString create<SmallString>() {
  return SmallString(FRAGILE, "small string");
}

template <> BigString create<BigString>() {
  return BigString(FRAGILE, string(10000, 'a'));
}

template <> BigBinary create<BigBinary>() {
  auto buf = folly::IOBuf::create(10000);
  buf->append(10000);
  return BigBinary(FRAGILE, std::move(buf));
}

template <> Mixed create<Mixed>() {
  return Mixed(FRAGILE, 5, 12345, true, "hello");
}

template <> SmallListInt create<SmallListInt>() {
  return SmallListInt(FRAGILE, vector<int>(10, 5));
}

template <> BigListInt create<BigListInt>() {
  return BigListInt(FRAGILE, vector<int>(10000, 5));
}

template <> BigListMixed create<BigListMixed>() {
  return BigListMixed(FRAGILE, vector<Mixed>(10000, create<Mixed>()));
}

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
  susp.dismiss();

  while (iters--) {
    Struct data;
    Serializer::deserialize(buf.get(), data);
  }
  susp.rehire();
}

#define X1(proto, rdwr, bench) \
  BENCHMARK(proto ## Protocol_ ## rdwr ## _ ## bench, iters) { \
    rdwr ## Bench<proto##Serializer, bench>(iters); \
  }

#define X2(proto, bench) X1(proto, write, bench) \
                         X1(proto, read, bench)

#define X(proto) \
  X2(proto, Empty)  \
  X2(proto, SmallInt)  \
  X2(proto, BigInt) \
  X2(proto, SmallString) \
  X2(proto, BigString) \
  X2(proto, BigBinary) \
  X2(proto, Mixed) \
  X2(proto, SmallListInt) \
  X2(proto, BigListInt) \
  X2(proto, BigListMixed) \

X(Binary)
X(Compact)

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  runBenchmarks();
  return 0;
}
