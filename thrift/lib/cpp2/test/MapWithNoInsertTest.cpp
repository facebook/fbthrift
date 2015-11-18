/*
 * Copyright 2015 Facebook, Inc.
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

#include <gtest/gtest.h>
#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/test/MapWithNoInsert.h>
#include <thrift/lib/cpp2/test/gen-cpp2/MapWithNoInsert_types.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::test;

namespace {

class MapWithNoInsertTest : public testing::Test {};

TEST_F(MapWithNoInsertTest, cpp2_ops_specialization) {
  MyObject obj;
  EXPECT_EQ(protocol::T_MAP, Cpp2Ops<decltype(obj.data)>::thriftType());
}

TEST_F(MapWithNoInsertTest, roundtrip_json) {
  MyObject obj;
  obj.data["hi"] = 3;
  obj.data["ho"] = 4;

  string out;
  SimpleJSONSerializer::serialize(obj, &out);

  MyObject copy;
  SimpleJSONSerializer::deserialize(out, copy);

  EXPECT_EQ(obj, copy);
}

}
