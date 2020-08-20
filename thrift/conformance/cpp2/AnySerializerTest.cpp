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

#include <thrift/conformance/cpp2/AnySerializer.h>

#include <any>

#include <gtest/gtest.h>
#include <thrift/conformance/cpp2/Testing.h>

// Helper so gtest prints out the line number when running the given check.
#define SCOPED_CHECK(check) \
  {                         \
    SCOPED_TRACE(#check);   \
    check;                  \
  }

namespace apache::thrift::conformance {

namespace {

TEST(AnySerializerTest, TypedSerializer) {
  FollyToStringSerializer<int> intCodec;
  EXPECT_EQ(intCodec.encode(1), "1");
  EXPECT_EQ(intCodec.decode("1"), 1);

  AnySerializer& anyCodec(intCodec);
  EXPECT_EQ(anyCodec.encode(1), "1");
  EXPECT_EQ(anyCodec.encode(std::any(1)), "1");
  EXPECT_THROW(anyCodec.encode(2.5), std::bad_any_cast);
  EXPECT_THROW(anyCodec.encode(std::any(2.5)), std::bad_any_cast);

  EXPECT_EQ(anyCodec.decode<int>("1"), 1);
  EXPECT_EQ(std::any_cast<int>(anyCodec.decode(typeid(int), "1")), 1);

  EXPECT_THROW(anyCodec.decode<double>("1.0"), std::bad_any_cast);
  EXPECT_THROW(anyCodec.decode(typeid(double), ""), std::bad_any_cast);
}

TEST(SerializerTest, MultiSerializer) {
  MultiSerializer multiSerializer;
  const AnySerializer& serializer = multiSerializer;

  SCOPED_CHECK(multiSerializer.checkAndResetAll());

  // Can handle ints.
  EXPECT_EQ(serializer.encode(1), "1");
  SCOPED_CHECK(multiSerializer.checkIntEnc());
  EXPECT_EQ(serializer.decode<int>("1"), 1);
  SCOPED_CHECK(multiSerializer.checkIntDec());

  // Can handle std::any(int).
  std::any a = serializer.decode(typeid(int), "1");
  SCOPED_CHECK(multiSerializer.checkAnyIntDec());
  EXPECT_EQ(std::any_cast<int>(a), 1);
  EXPECT_EQ(serializer.encode(a), "1");
  SCOPED_CHECK(multiSerializer.checkIntEnc());

  // Can handle doubles.
  EXPECT_EQ(serializer.encode(2.5), "2.5");
  SCOPED_CHECK(multiSerializer.checkDblEnc());
  EXPECT_EQ(serializer.decode<double>("0.5"), 0.5f);
  SCOPED_CHECK(multiSerializer.checkDblDec());

  // Can handle std::any(double).
  a = serializer.decode(typeid(double), "1");
  SCOPED_CHECK(multiSerializer.checkAnyDblDec());
  EXPECT_EQ(std::any_cast<double>(a), 1.0);
  EXPECT_EQ(serializer.encode(a), "1");
  SCOPED_CHECK(multiSerializer.checkDblEnc());

  // Cannot handle float.
  EXPECT_THROW(serializer.encode(1.0f), std::bad_any_cast);
  SCOPED_CHECK(multiSerializer.checkAndResetAll());
  EXPECT_THROW(serializer.decode<float>("1"), std::bad_any_cast);
  SCOPED_CHECK(multiSerializer.checkAndResetAll());

  // Cannot handle std::any(float).
  a = 1.0f;
  EXPECT_THROW(serializer.decode(typeid(float), "1"), std::bad_any_cast);
  SCOPED_CHECK(multiSerializer.checkAndResetAny(1));
  SCOPED_CHECK(multiSerializer.checkAndResetAll());
  EXPECT_THROW(serializer.encode(a), std::bad_any_cast);
  SCOPED_CHECK(multiSerializer.checkAndResetAll());
}

} // namespace
} // namespace apache::thrift::conformance
