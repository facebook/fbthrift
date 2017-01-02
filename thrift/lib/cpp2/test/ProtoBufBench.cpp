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

#include <thrift/lib/cpp2/test/ProtoBufBenchData.pb.h>

#include <gflags/gflags.h>
#include <glog/logging.h>
#include <folly/Benchmark.h>
#include <folly/Format.h>
#include <folly/Optional.h>

using namespace std;
using namespace folly;

// Only declaration, no definition.
// This will error if used with a type other than the explicit
// instantiations below.
template <typename Struct>
Struct create();

template <> Empty create<Empty>() {
  return Empty();
}

template <> SmallInt create<SmallInt>() {
  SmallInt i;
  i.set_smallint(5);
  return i;
}

template <> BigInt create<BigInt>() {
  BigInt i;
  i.set_bigint(0x1234567890abcdefL);
  return i;
}

template <> SmallString create<SmallString>() {
  SmallString s;
  s.set_smallstr("small string");
  return s;
}

template <> BigString create<BigString>() {
  BigString s;
  s.set_bigstr(string(10000, 'a'));
  return s;
}

template <> Mixed create<Mixed>() {
  Mixed m;
  m.set_i32(5);
  m.set_i64(12345);
  m.set_b(true);
  m.set_s("hello");
  return m;
}

template <> SmallListInt create<SmallListInt>() {
  SmallListInt l;
  for (int i = 0; i < 10; i++) {
    l.add_lst(5);
  }
  return l;
}

template <> BigListInt create<BigListInt>() {
  BigListInt l;
  for (int i = 0; i < 10000; i++) {
    l.add_lst(5);
  }
  return l;
}

template <> BigListMixed create<BigListMixed>() {
  BigListMixed l;
  for (int i = 0; i < 10000; i++) {
    *l.add_lst() = create<Mixed>();
  }
  return l;
}

template <> LargeListMixed create<LargeListMixed>() {
  LargeListMixed l;
  for (int i = 0; i < 1000000; i++) {
    *l.add_lst() = create<Mixed>();
  }
  return l;
}

template <typename Struct>
void writeBench(size_t iters) {
  BenchmarkSuspender susp;
  auto strct = create<Struct>();
  susp.dismiss();

  while (iters--) {
    string s;
    strct.SerializeToString(&s);
  }
  susp.rehire();
}

template <typename Struct>
void readBench(size_t iters) {
  BenchmarkSuspender susp;
  auto strct = create<Struct>();
  string s;
  strct.SerializeToString(&s);
  susp.dismiss();

  while (iters--) {
    Struct data;
    data.ParseFromString(s);
  }
  susp.rehire();
}

#define X1(proto, rdwr, bench) \
  BENCHMARK(proto ## _ ## rdwr ## _ ## bench, iters) { \
    rdwr ## Bench<bench>(iters); \
  }

#define X2(proto, bench) X1(proto, write, bench) \
                         X1(proto, read, bench)

#define X(proto) \
  X2(proto, Empty)  \
  X2(proto, SmallInt)  \
  X2(proto, BigInt)  \
  X2(proto, SmallString)  \
  X2(proto, BigString)  \
  X2(proto, Mixed)  \
  X2(proto, SmallListInt)  \
  X2(proto, BigListInt)  \
  X2(proto, BigListMixed)  \
  X2(proto, LargeListMixed)  \

X(ProtoBuf)

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  runBenchmarks();
  return 0;
}
