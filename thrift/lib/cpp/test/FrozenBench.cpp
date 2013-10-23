/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#define BOOST_TEST_MODULE FrozenTest

#include "folly/Conv.h"
#include "folly/Benchmark.h"
#include "thrift/lib/cpp/util/ThriftSerializer.h"
#include "thrift/lib/cpp/test/gen-cpp/FrozenTypes_types.h"

using namespace FrozenTypes;
using std::string;
using std::vector;
using folly::fbstring;
using folly::StringPiece;
using namespace apache::thrift;

Team testValue() {
  Team team;
  auto hasher = std::hash<int64_t>();
  team.__isset.peopleById = true;
  team.__isset.peopleByName = true;
  for (int i = 1; i <= 500; ++i) {
    auto id = hasher(i);
    Person p;
    p.id = id;
    p.__isset.nums = true;
    p.nums.insert(i);
    p.nums.insert(-i);
    folly::toAppend("Person ", i, &p.name);
    team.peopleById[p.id] = p;
    team.peopleByName[p.name] = std::move(p);
  }
  team.projects.insert("alpha");
  team.projects.insert("beta");
  team.__isset.projects = true;
  return team;
}

auto team = testValue();
auto frozen = freeze(team);

BENCHMARK(Freeze, iters) {
  vector<byte> buffer;
  while (iters--) {
    size_t bytes = frozenSize(team);
    if (buffer.size() < bytes) {
      buffer.resize(bytes);
    }
    byte* p = buffer.data();
    auto frozen = freeze(team, p);
  }
}

BENCHMARK_RELATIVE(FreezePreallocated, iters) {
  vector<byte> buffer(102400);
  while (iters--) {
    byte* p = buffer.data();
    auto frozen = freeze(team, p);
    assert(p <= &buffer.back());
  }
}

template <template <class> class SerializerTemplate>
void benchmarkSerializer(int iters) {
  SerializerTemplate<Team> serde;
  string serialized;

  while (iters--) {
    serde.serialize(team, &serialized);
  }
}

BENCHMARK_RELATIVE(SerializerCompact, iters) {
  benchmarkSerializer<apache::thrift::util::ThriftSerializerCompact>(iters);
}

BENCHMARK_RELATIVE(SerializerBinary, iters) {
  benchmarkSerializer<apache::thrift::util::ThriftSerializerBinary>(iters);
}

BENCHMARK_DRAW_LINE()

BENCHMARK(Thaw, iters) {
  Team thawed;
  while (iters--) {
    thaw(*frozen, thawed);
  }
}

template <template <class> class SerializerTemplate>
void benchmarkDeserializer(int iters) {
  SerializerTemplate<Team> serde;
  string serialized;
  Team obj;
  serde.serialize(team, &serialized);

  while (iters--) {
    serde.deserialize(serialized, &obj);
  }
}

BENCHMARK_RELATIVE(DeserializerCompact, iters) {
  benchmarkDeserializer<apache::thrift::util::ThriftSerializerCompact>(iters);
}

BENCHMARK_RELATIVE(DeserializerBinary, iters) {
  benchmarkDeserializer<apache::thrift::util::ThriftSerializerBinary>(iters);
}

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);
  folly::runBenchmarks();

  return 0;
}
