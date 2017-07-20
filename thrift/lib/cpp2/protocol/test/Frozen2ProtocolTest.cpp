/*
 * Copyright 2004-present Facebook, Inc.
 *
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

#include <thrift/lib/cpp2/protocol/Frozen2Protocol.h>

#include <map>
#include <vector>

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/protocol/test/gen-cpp2/frozen2_protocol_test_layouts.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/frozen2_protocol_test_types.h>

static constexpr int VECTOR_ELEMENTS_MAX = 100;
static constexpr int MAP_ELEMENTS_MAX = 10;

using namespace ::apache::thrift;
using namespace ::test::frozen2;

void makeMap(std::unordered_map<int32_t, std::string>& m) {
  int32_t start = std::numeric_limits<int32_t>::max() - MAP_ELEMENTS_MAX + 1;
  for (int32_t i = 0; i < MAP_ELEMENTS_MAX; ++i) {
    m[start + i] = "ARandomString";
  }
}

void makeVector(std::vector<std::unordered_map<int32_t, std::string>>& vec) {
  vec.reserve(VECTOR_ELEMENTS_MAX);
  for (int32_t i = 0; i < VECTOR_ELEMENTS_MAX; ++i) {
    std::unordered_map<int32_t, std::string> m;
    makeMap(m);
    vec.push_back(m);
  }
}

TEST(Frozen2Protocol, readObject) {
  Frozen2ProtocolWriter outprot;
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());

  StructA a, b;
  a.i32Field = 1;
  a.strField = "a";
  makeVector(a.listField);

  outprot.setOutput(&queue, outprot.serializedObjectSize(a));
  outprot.writeObject(a);

  Frozen2ProtocolReader inprot;
  inprot.setInput(queue.front());
  inprot.readObject(b);

  ASSERT_EQ(a, b);
}
