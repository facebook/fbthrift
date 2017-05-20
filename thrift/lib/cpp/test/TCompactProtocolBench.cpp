/*
 * Copyright 2004-present Facebook, Inc.
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

#include <thrift/lib/cpp/protocol/TCompactProtocol.h>

#include <thrift/lib/cpp/test/gen-cpp/TCompactProtocolBenchData_types.h>

#include <iostream>
#include <vector>
#include <gflags/gflags.h>
#include <glog/logging.h>
#include <folly/Benchmark.h>
#include <folly/Format.h>
#include <folly/Optional.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>

using namespace std;
using namespace folly;;
using namespace apache::thrift;

using apache::thrift::protocol::TCompactProtocolT;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::util::ThriftSerializerCompact;

const size_t kMultExp = 0;

Shallow makeShallow() {
  Shallow data;
  data.one = 750;
  data.two = 750L << 23;
  return data;
}

Deep makeDeep(size_t triplesz) {
  Deep data;
  for (size_t i = 0; i < triplesz; ++i) {
    Deep1 data1;
    for (size_t j = 0; j < triplesz; ++j) {
      Deep2 data2;
      for (size_t k = 0; k < triplesz; ++k) {
        data2.datas.push_back(sformat("omg[{}, {}, {}]", i, j, k));
      }
      data1.deeps.push_back(move(data2));
    }
    data.deeps.push_back(move(data1));
  }
  return data;
}

BENCHMARK(TCompactProtocol_ctor, kiters) {
  BenchmarkSuspender braces;
  size_t iters = kiters << kMultExp;
  auto trans = make_shared<TMemoryBuffer>(1 << 12);
  vector<Optional<TCompactProtocolT<TMemoryBuffer>>> protos(iters);
  braces.dismiss();
  while (iters--) {
    protos[iters].emplace(trans);
  }
  braces.rehire();
}

BENCHMARK(TCompactProtocol_dtor, kiters) {
  BenchmarkSuspender braces;
  size_t iters = kiters << kMultExp;
  auto trans = make_shared<TMemoryBuffer>(1 << 12);
  vector<Optional<TCompactProtocolT<TMemoryBuffer>>> protos(iters);
  for (auto& proto : protos) {
    proto.emplace(trans);
  }
  braces.dismiss();
  while (iters--) {
    protos[iters].clear();
  }
  braces.rehire();
}

BENCHMARK(TCompactProtocol_deserialize_empty, kiters) {
  BenchmarkSuspender braces;
  size_t iters = kiters << kMultExp;
  Empty data;
  ThriftSerializerCompact<> ser;
  string buf;
  ser.serialize(data, &buf);
  braces.dismiss();
  while (iters--) {
    ThriftSerializerCompact<> s;
    Empty empty;
    s.deserialize(buf, &empty);
  }
  braces.rehire();
}

BENCHMARK(TCompactProtocol_serialize_empty, kiters) {
  BenchmarkSuspender braces;
  size_t iters = kiters << kMultExp;
  Empty data;
  braces.dismiss();
  while (iters--) {
    ThriftSerializerCompact<> ser;
    string str;
    ser.serialize(data, &str);
  }
  braces.rehire();
}

BENCHMARK(TCompactProtocol_deserialize_shallow, kiters) {
  BenchmarkSuspender braces;
  size_t iters = kiters << kMultExp;
  Shallow data = makeShallow();
  ThriftSerializerCompact<> ser;
  string buf;
  ser.serialize(data, &buf);
  braces.dismiss();
  while (iters--) {
    ThriftSerializerCompact<> s;
    Deep data;
    s.deserialize(buf, &data);
  }
  braces.rehire();
}

BENCHMARK(TCompactProtocol_serialize_shallow, kiters) {
  BenchmarkSuspender braces;
  size_t iters = kiters << kMultExp;
  Shallow data = makeShallow();
  braces.dismiss();
  while (iters--) {
    ThriftSerializerCompact<> ser;
    string buf;
    ser.serialize(data, &buf);
  }
  braces.rehire();
}

BENCHMARK(TCompactProtocol_deserialize_deep, kiters) {
  BenchmarkSuspender braces;
  size_t iters = kiters << kMultExp;
  Deep data = makeDeep(16);
  ThriftSerializerCompact<> ser;
  string buf;
  ser.serialize(data, &buf);
  braces.dismiss();
  while (iters--) {
    ThriftSerializerCompact<> s;
    Deep deep;
    s.deserialize(buf, &deep);
  }
  braces.rehire();
}

BENCHMARK(TCompactProtocol_serialize_deep, kiters) {
  BenchmarkSuspender braces;
  size_t iters = kiters << kMultExp;
  Deep data = makeDeep(16);
  braces.dismiss();
  while (iters--) {
    ThriftSerializerCompact<> ser;
    string buf;
    ser.serialize(data, &buf);
  }
  braces.rehire();
}

int main(int argc, char** argv) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  runBenchmarks();
  return 0;
}
