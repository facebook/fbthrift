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

#include "common/fbunit/benchmark.h"
#include "common/fbunit/fbunit.h"
#include "common/init/Init.h"
#include <thrift/lib/cpp/util/gen-cpp/ThriftSerializerTest_types.h>
#include <thrift/lib/cpp/protocol/TProtocolException.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>

using namespace facebook;
using namespace std;
using namespace apache::thrift::util;

FBUNIT_DASHBOARD_OWNER("karl")
FBUNIT_DASHBOARD_EMAILS("oncall+ad_units@xmail.facebook.com")

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

FBUNIT_TEST(ThriftSerializerBinaryTest) {
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

FBUNIT_TEST(ThriftSerializerCompactTest) {
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

FBUNIT_TEST(ThriftOneSerializerTwoTypesTest) {
  ThriftSerializerCompact<> izer;
  TestValue value;
  TestOtherValue other;
  string result;

  makeTestValue(&value);

  izer.serialize(value, &result);
  izer.deserialize(result, &value);

  // This should compile, but fail at runtime, since the types are
  // incompatible.

  EXPECT_EXCEPTION(izer.deserialize(result, &other),
                   apache::thrift::protocol::TProtocolException&);

  other.color = "blue";

  izer.serialize(other, &result);
  izer.deserialize(result, &other);
}

FBUNIT_TEST(ThriftSerializerBinaryIgnoreTypesTest) {
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

FBUNIT_TEST(ThriftSerializerBinaryTest_Fail) {
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

FBUNIT_TEST(ThriftSerializerJson) {
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

FBUNIT_TEST(ThriftSerializerJson_Fail) {
  ThriftSerializerJson<> izer;
  TestValue result;
  string serialized;
  TestValue original;

  makeTestValue(&original);

  // serialize and modify
  izer.serialize(original, &serialized);
  serialized = "Garbage" + serialized;

  EXPECT_EXCEPTION(izer.deserialize(serialized, &result),
                   apache::thrift::protocol::TProtocolException&);
}

FBUNIT_TEST(ThriftSerializerBinaryVersionTest) {
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

FBUNIT_TEST(ThriftSerializerCompactVersionTest) {
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

FBUNIT_TEST(ThriftOneSerializerTwoTypesVersionTest) {
  ThriftSerializerCompact<> izer;
  TestValue value;
  TestOtherValue other;
  string result;

  makeTestValue(&value);

  izer.serialize(value, &result);
  izer.deserialize(result, &value);

  // This should compile, but fail at runtime, since the types are
  // incompatible.

  EXPECT_EXCEPTION(izer.deserialize(result, &other),
                   apache::thrift::protocol::TProtocolException&);

  other.color = "blue";

  izer.serialize(other, &result);
  izer.deserialize(result, &other);
}

FBUNIT_TEST(ThriftSerializerBinaryIgnoreTypesVersionTest) {
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

FBUNIT_TEST(ThriftSerializerBinaryVersionTest_Fail) {
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

FBUNIT_TEST(ThriftSerializerJsonVersion) {
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

FBUNIT_TEST(ThriftSerializerJsonVersion_Fail) {
  ThriftSerializerJson<> izer;
  TestValue result;
  string serialized;
  TestValue original;

  makeTestValue(&original);

  izer.setSerializeVersion(true);
  // serialize and modify
  izer.serialize(original, &serialized);
  serialized = "Garbage" + serialized;

  EXPECT_EXCEPTION(izer.deserialize(serialized, &result),
                   apache::thrift::protocol::TProtocolException&);
}

void BM_ThriftSerializerBinary(int count) {
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
BM_REGISTER(BM_ThriftSerializerBinary);


void BM_ThriftSerializerBinary_NoReuse(int count) {
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
BM_REGISTER(BM_ThriftSerializerBinary_NoReuse);

FBUNIT_TEST(ThriftSerializerBinaryExt) {
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

int main(int argc, char** argv) {
  initFacebook(&argc, &argv);
  RUN_ALL_TESTS_AND_EXIT();
  return 0;
}
