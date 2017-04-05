/*
 * Copyright 2016 Facebook, Inc.
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
#include <memory>
#include <vector>

#include <folly/Memory.h>
#include <folly/Range.h>
#include <gtest/gtest.h>

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>

#include <thrift/test/gen-cpp2/GetterSetterTest_types.h>
#include <thrift/test/gen-cpp2/GetterSetterTest_types.tcc>

using namespace thrift::test::getter_setter::cpp2;

TEST(GetterSetter, BasicOptionalFields) {
  GetterSetterTest obj;
  std::vector<int32_t> vec = { 1, 2, 3 };
  auto buf = std::make_unique<folly::IOBuf>();

  EXPECT_FALSE(obj.__isset.optionalInt);
  EXPECT_EQ(nullptr, obj.get_optionalInt());
  EXPECT_FALSE(obj.__isset.optionalList);
  EXPECT_EQ(nullptr, obj.get_optionalList());
  EXPECT_FALSE(obj.__isset.optionalBuf);
  EXPECT_EQ(nullptr, obj.get_optionalBuf());

  obj.set_optionalInt(42);
  EXPECT_EQ(42, *obj.get_optionalInt());
  EXPECT_TRUE(obj.__isset.optionalInt);
  obj.set_optionalList(vec);
  EXPECT_EQ(vec, *obj.get_optionalList());
  EXPECT_TRUE(obj.__isset.optionalList);
  obj.set_optionalBuf(std::move(buf));
  EXPECT_TRUE((*obj.get_optionalBuf())->empty());
  EXPECT_TRUE(obj.__isset.optionalBuf);
}

TEST(GetterSetter, BasicDefaultFields) {
  GetterSetterTest obj;
  std::vector<int32_t> vec = { 1, 2, 3 };
  folly::StringPiece str("abc123");
  auto buf = std::make_unique<folly::IOBuf>(folly::IOBuf::WRAP_BUFFER, str);

  EXPECT_TRUE(obj.get_defaultList().empty());
  EXPECT_EQ(nullptr, obj.get_defaultBuf());

  obj.set_defaultInt(42);
  EXPECT_EQ(42, obj.get_defaultInt());
  obj.set_defaultList(vec);
  EXPECT_EQ(vec, obj.get_defaultList());
  obj.set_defaultBuf(std::move(buf));
  EXPECT_EQ(6, obj.get_defaultBuf()->length());
}
