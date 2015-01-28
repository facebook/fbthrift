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

#include <time.h>

#include <folly/Benchmark.h>
#include <thrift/lib/cpp/util/gen-cpp/ThriftSerializerTest_types.h>
#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>

#include <gtest/gtest.h>

#include "common/init/Init.h"

using namespace facebook;
using namespace std;
using namespace apache::thrift::util;

const std::string randomString = "~!@#$%^&*()_+][/.,\\][/.,'abcde\t\n";

void makeTestValue(TestValue* out) {
  // make bogus data
  out->legalClicks = 1111;
  out->rawImpressions = 222222;
  out->lastLegalClickTime = 3333333333;
  out->lastLegalImpressionTime = 4444444444;
  out->socialClicks = 55;
  out->socialImpressions = 666;
  out->throttle = 7.77777;
  out->pageOtherImpressions = 8888;
  out->pageCanvasImpressions = 9999;
  out->pageProfileImpressions = 1111;
  out->pageSearchImpressions = 2222;
  out->pageEventImpressions = 3333;
  out->pageGroupImpressions = 4444;
  out->pagePhotoImpressions = 5555;
  out->pageHomeImpressions = 6666;
  out->position1Impressions = 7777;
  out->position2Impressions = 8888;
  out->position3Impressions = 9999;
  out->feedbackLegalLikes = 11;
  out->feedbackLegalDislikes = 222;
  out->text = randomString;
}

void expectEqualValues(const TestValue& a, const TestValue& b) {
  EXPECT_EQ(a.legalClicks, b.legalClicks);
  EXPECT_EQ(a.rawImpressions, b.rawImpressions);
  EXPECT_EQ(a.lastLegalClickTime, b.lastLegalClickTime);
  EXPECT_EQ(a.lastLegalImpressionTime, b.lastLegalImpressionTime);
  EXPECT_EQ(a.socialClicks, b.socialClicks);
  EXPECT_EQ(a.socialImpressions, b.socialImpressions);
  EXPECT_EQ(a.throttle, b.throttle);
  EXPECT_EQ(a.pageOtherImpressions, b.pageOtherImpressions);
  EXPECT_EQ(a.pageCanvasImpressions, b.pageCanvasImpressions);
  EXPECT_EQ(a.pageProfileImpressions, b.pageProfileImpressions);
  EXPECT_EQ(a.pageSearchImpressions, b.pageSearchImpressions);
  EXPECT_EQ(a.pageEventImpressions, b.pageEventImpressions);
  EXPECT_EQ(a.pageGroupImpressions, b.pageGroupImpressions);
  EXPECT_EQ(a.pagePhotoImpressions, b.pagePhotoImpressions);
  EXPECT_EQ(a.pageHomeImpressions, b.pageHomeImpressions);
  EXPECT_EQ(a.position1Impressions, b.position1Impressions);
  EXPECT_EQ(a.position2Impressions, b.position2Impressions);
  EXPECT_EQ(a.position3Impressions, b.position3Impressions);
  EXPECT_EQ(a.feedbackLegalLikes, b.feedbackLegalLikes);
  EXPECT_EQ(a.feedbackLegalDislikes, b.feedbackLegalDislikes);
  EXPECT_EQ(a.text, b.text);
}

TEST(Main, ThriftSerializerBinaryTest) {
  ThriftSerializerBinary<> izer;
  TestValue result;
  string serialized;
  TestValue original;

  makeTestValue(&original);

  // serialize and deserialize
  izer.serialize(original, &serialized);
  EXPECT_EQ(serialized.length(), izer.deserialize(serialized, &result));

  expectEqualValues(original, result);

  // invoke the routines again in a strange order
  //
  // note: The implementation, we happen to know, tries to be clever
  // about reusing resources, so we'd like to test it.
  original.legalClicks = 1;
  EXPECT_EQ(serialized.length(), izer.deserialize(serialized, &result));
  izer.serialize(original, &serialized);
  izer.serialize(original, &serialized);
  EXPECT_EQ(serialized.length(), izer.deserialize(serialized, &result));

  expectEqualValues(original, result);
}

TEST(Main, ThriftSerializerCompactTest) {
  ThriftSerializerCompactDeprecated<> izerDeprecated;
  ThriftSerializerCompact<> izer;
  TestValue result;
  string serialized;
  string serializedDeprecated;
  TestValue original;

  makeTestValue(&original);

  // serialize and deserialize
  izer.serialize(original, &serialized);
  izerDeprecated.serialize(original, &serializedDeprecated);
  EXPECT_EQ(serialized.length(), izer.deserialize(serialized, &result));
  EXPECT_NE(serialized, serializedDeprecated);

  expectEqualValues(original, result);
  EXPECT_EQ(original.throttle, result.throttle);
  EXPECT_EQ(7.77777, result.throttle);
  izerDeprecated.deserialize(serialized, &result);
  EXPECT_NE(result.throttle, 7.77777);
}

TEST(Main, ThriftOneSerializerTwoTypesTest) {
  ThriftSerializerCompact<> izer;
  TestValue value;
  TestOtherValue other;
  string result;

  makeTestValue(&value);

  izer.serialize(value, &result);
  izer.deserialize(result, &value);

  // This should compile, but fail at runtime, since the types are
  // incompatible.

  EXPECT_THROW(izer.deserialize(result, &other),
               apache::thrift::protocol::TProtocolException);

  other.color = "blue";

  izer.serialize(other, &result);
  izer.deserialize(result, &other);
}

TEST(Main, ThriftSerializerBinaryIgnoreTypesTest) {
  // Explicit types on the serializer are ignored.  Not only are these
  // types different, they aren't even thrift types!
  ThriftSerializerBinary<int> izer1;
  ThriftSerializerBinary<double> izer2;

  TestValue original;
  string serialized;
  TestValue result;

  makeTestValue(&original);
  izer1.serialize(original, &serialized);
  izer2.deserialize(serialized, &result);

  expectEqualValues(original, result);
}

TEST(Main, ThriftSerializerBinaryTest_Fail) {
  ThriftSerializerBinary<> izer;
  TestValue result;
  string serialized;
  TestValue original;

  makeTestValue(&original);

  // serialize and modify
  izer.serialize(original, &serialized);
  serialized += "Garbage";

  EXPECT_GT(serialized.length(), izer.deserialize(serialized, &result));
}

TEST(Main, ThriftSerializerJson) {
  ThriftSerializerJson<> izer;
  TestValue result;
  string serialized;
  TestValue original;

  makeTestValue(&original);

  // serialize and deserialize
  izer.serialize(original, &serialized);
  EXPECT_EQ(serialized.length(), izer.deserialize(serialized, &result));

  expectEqualValues(original, result);

  // invoke the routines again in a strange order
  //
  // note: The implementation, we happen to know, tries to be clever
  // about reusing resources, so we'd like to test it.
  original.legalClicks = 1;
  EXPECT_EQ(serialized.length(), izer.deserialize(serialized, &result));
  izer.serialize(original, &serialized);
  izer.serialize(original, &serialized);
  EXPECT_EQ(serialized.length(), izer.deserialize(serialized, &result));

  expectEqualValues(original, result);
}

TEST(Main, ThriftSerializerJson_Fail) {
  ThriftSerializerJson<> izer;
  TestValue result;
  string serialized;
  TestValue original;

  makeTestValue(&original);

  // serialize and modify
  izer.serialize(original, &serialized);
  serialized = "Garbage" + serialized;

  EXPECT_THROW(izer.deserialize(serialized, &result),
               apache::thrift::protocol::TProtocolException);
}

TEST(Main, ThriftSerializerSimpleJson_Unicode) {
  ThriftSerializerSimpleJson<> izer;
  TestOtherValue result;

  string s1(R"test({"color":"\u0026"})test");
  string s2(R"test({"color":"\u00f1"})test");
  string s3(R"test({"color":"\u1234"})test");
  string s4(R"test({"color":"\ud834\udd1e"})test");
  string s5(R"test({"color":"\"\/\b\n\f\t\r"})test");
  string s6(R"test({"color":"\u1234\""})test");
  string s7(R"test({"color":"\u1234\\"})test");
  string s8(R"test({"color":"\u1234\\\""})test");

  izer.deserialize(s1, &result);
  EXPECT_EQ(result.color.length(), 1);
  izer.deserialize(s2, &result);
  EXPECT_EQ(result.color.length(), 1);
  EXPECT_THROW(izer.deserialize(s3, &result),
               apache::thrift::protocol::TProtocolException);

  ThriftSerializerSimpleJson<> izer1;
  izer1.allowDecodeUTF8(true);

  izer1.deserialize(s2, &result);
  EXPECT_EQ(result.color.length(), 2);
  izer1.deserialize(s3, &result);
  EXPECT_EQ(result.color.length(), 3);
  izer1.deserialize(s4, &result);
  EXPECT_EQ(result.color.length(), 4);
  izer1.deserialize(s5, &result);
  EXPECT_EQ(result.color.length(), 7);
  izer1.deserialize(s6, &result);
  EXPECT_EQ(result.color.length(), 4);
  izer1.deserialize(s7, &result);
  EXPECT_EQ(result.color.length(), 4);
  izer1.deserialize(s8, &result);
  EXPECT_EQ(result.color.length(), 5);
}

TEST(Main, ThriftSerializerBinaryVersionTest) {
  ThriftSerializerBinary<> izer;
  TestValue result;
  string serialized;
  TestValue original;

  makeTestValue(&original);

  izer.setSerializeVersion(true);
  // serialize and deserialize
  izer.serialize(original, &serialized);
  EXPECT_GT(serialized.length(), izer.deserialize(serialized, &result));

  expectEqualValues(original, result);

  // invoke the routines again in a strange order
  //
  // note: The implementation, we happen to know, tries to be clever
  // about reusing resources, so we'd like to test it.
  original.legalClicks = 1;
  EXPECT_GT(serialized.length(), izer.deserialize(serialized, &result));
  izer.serialize(original, &serialized);
  izer.serialize(original, &serialized);
  EXPECT_GT(serialized.length(), izer.deserialize(serialized, &result));

  expectEqualValues(original, result);
}

TEST(Main, ThriftSerializerCompactVersionTest) {
  ThriftSerializerCompactDeprecated<> izerDeprecated;
  ThriftSerializerCompact<> izer;
  TestValue result;
  string serialized;
  string serializedDeprecated;
  TestValue original;

  makeTestValue(&original);

  izer.setSerializeVersion(true);
  // serialize and deserialize
  izer.serialize(original, &serialized);
  izerDeprecated.serialize(original, &serializedDeprecated);
  EXPECT_GT(serialized.length(), izer.deserialize(serialized, &result));
  EXPECT_NE(serialized, serializedDeprecated);

  expectEqualValues(original, result);
  EXPECT_EQ(original.throttle, result.throttle);
  EXPECT_EQ(7.77777, result.throttle);
  izerDeprecated.deserialize(serialized, &result);
  EXPECT_NE(result.throttle, 7.77777);
}

TEST(Main, ThriftOneSerializerTwoTypesVersionTest) {
  ThriftSerializerCompact<> izer;
  TestValue value;
  TestOtherValue other;
  string result;

  makeTestValue(&value);

  izer.serialize(value, &result);
  izer.deserialize(result, &value);

  // This should compile, but fail at runtime, since the types are
  // incompatible.

  EXPECT_THROW(izer.deserialize(result, &other),
               apache::thrift::protocol::TProtocolException);

  other.color = "blue";

  izer.serialize(other, &result);
  izer.deserialize(result, &other);
}

TEST(Main, ThriftSerializerBinaryIgnoreTypesVersionTest) {
  // Explicit types on the serializer are ignored.  Not only are these
  // types different, they aren't even thrift types!
  ThriftSerializerBinary<int> izer1;
  ThriftSerializerBinary<double> izer2;

  TestValue original;
  string serialized;
  TestValue result;

  makeTestValue(&original);
  izer1.setSerializeVersion(true);
  izer2.setSerializeVersion(true);
  izer1.serialize(original, &serialized);
  izer2.deserialize(serialized, &result);

  expectEqualValues(original, result);
}

TEST(Main, ThriftSerializerBinaryVersionTest_Fail) {
  ThriftSerializerBinary<> izer;
  TestValue result;
  string serialized;
  TestValue original;

  makeTestValue(&original);

  izer.setSerializeVersion(true);
  // serialize and modify
  izer.serialize(original, &serialized);
  serialized += "Garbage";

  EXPECT_GT(serialized.length(), izer.deserialize(serialized, &result));
}

TEST(Main, ThriftSerializerJsonVersion) {
  ThriftSerializerJson<> izer;
  TestValue result;
  string serialized;
  TestValue original;

  makeTestValue(&original);

  izer.setSerializeVersion(true);
  // serialize and deserialize
  izer.serialize(original, &serialized);
  EXPECT_GT(serialized.length(), izer.deserialize(serialized, &result));

  expectEqualValues(original, result);

  // invoke the routines again in a strange order
  //
  // note: The implementation, we happen to know, tries to be clever
  // about reusing resources, so we'd like to test it.
  original.legalClicks = 1;
  EXPECT_GT(serialized.length(), izer.deserialize(serialized, &result));
  izer.serialize(original, &serialized);
  izer.serialize(original, &serialized);
  EXPECT_GT(serialized.length(), izer.deserialize(serialized, &result));

  expectEqualValues(original, result);
}

TEST(Main, ThriftSerializerJsonVersion_Fail) {
  ThriftSerializerJson<> izer;
  TestValue result;
  string serialized;
  TestValue original;

  makeTestValue(&original);

  izer.setSerializeVersion(true);
  // serialize and modify
  izer.serialize(original, &serialized);
  serialized = "Garbage" + serialized;

  EXPECT_THROW(izer.deserialize(serialized, &result),
               apache::thrift::protocol::TProtocolException);
}

BENCHMARK(BM_ThriftSerializerBinary, count) {
  // devrs003.sctm 9/2008: 100K in 413 ms
  // devrs003.snc1 12/2008: 100K in 292 ms

  TestValue original;
  makeTestValue(&original);

  ThriftSerializerBinary<> izer;
  string serialized;
  for (int i = 0; i < count; ++i) {
    izer.serialize(original, &serialized);
  }

  TestValue result;
  for (int i = 0; i < count; ++i) {
    izer.deserialize(serialized, &result);
  }
}

BENCHMARK(ThriftSerializerBinary_NoReuse, count) {
  // devrs003.snc1 12/2008: 100K in 444 ms

  TestValue result;
  string serialized;
  TestValue original;

  makeTestValue(&original);

  for (int i = 0; i < count; ++i) {
    ThriftSerializerBinary<> izer;
    izer.serialize(original, &serialized);
  }
  for (int i = 0; i < count; ++i) {
    ThriftSerializerBinary<> izer;
    izer.deserialize(serialized, &result);
  }
}

TEST(Main, ThriftSerializerBinaryExt) {
  // devrs003.sctm 9/2008: 100K in 413 ms
  // devrs003.snc1 12/2008: 100K in 292 ms

  TestValue original;
  makeTestValue(&original);

  ThriftSerializerBinary<> izer;
  const uint8_t* serializedBuffer;
  size_t serializedLen;
  izer.serialize(original, &serializedBuffer, &serializedLen);
  TestValue result;
  string serialized((const char*)serializedBuffer, serializedLen);
  izer.deserialize(serialized, &result);
  expectEqualValues(original, result);
}
