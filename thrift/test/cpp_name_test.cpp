/*
 * Copyright 2018-present Facebook, Inc.
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

// Make sure that cpp_name_types.h can be included with conflicting_name
// defined to something problematic.
#define conflicting_name 0

#include <folly/test/JsonTestUtil.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/reflection/internal/test_helpers.h>
#include <thrift/test/gen-cpp2/cpp_name_fatal_struct.h>
#include <thrift/test/gen-cpp2/cpp_name_types.h>

#include <folly/portability/GTest.h>

using apache::thrift::SimpleJSONSerializer;
using namespace apache::thrift::test;

TEST(cpp_name_test, rename) {
  auto s = MyStruct();
  s.set_unique_name(42);
  s.set_opt_unique_name(4);  // chosen by fair dice roll
  EXPECT_EQ(42, s.unique_name);
  EXPECT_EQ(4, s.opt_unique_name);
  EXPECT_EQ(42, s.get_unique_name());
  EXPECT_EQ(4, *(s.get_opt_unique_name()));
  EXPECT_EQ(4, *(s.opt_unique_name_ref()));
}

TEST(cpp_name_test, json_serialization) {
  auto in = MyStruct();
  in.set_unique_name(42);
  in.set_opt_unique_name(4);  // chosen by fair dice roll
  auto json = SimpleJSONSerializer::serialize<std::string>(in);
  FOLLY_EXPECT_JSON_EQ(json, R"({"conflicting_name": 42, "opt_conflicting_name": 4})");
  auto out = MyStruct();
  SimpleJSONSerializer::deserialize(json, out);
  EXPECT_EQ(out.unique_name, 42);
  EXPECT_EQ(out.opt_unique_name, 4);
  EXPECT_EQ(*(out.opt_unique_name_ref()), 4);
}

FATAL_S(unique_name, "unique_name");
FATAL_S(opt_unique_name, "opt_unique_name");

TEST(cpp_name_test, reflection) {
  using info = apache::thrift::reflect_struct<MyStruct>;
  EXPECT_SAME<unique_name, info::member::unique_name::name>();
}

TEST(cpp_name_test, enum_value) {
  EXPECT_EQ(static_cast<int>(MyEnum::REALM), 1);
  EXPECT_STREQ(apache::thrift::util::enumName(MyEnum::REALM), "DOMAIN");
}
