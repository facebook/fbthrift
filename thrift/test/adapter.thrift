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

namespace cpp2 apache.thrift.test.basic

include "thrift/annotation/cpp.thrift"
cpp_include "thrift/test/AdapterTest.h"

typedef i64 DurationMs (
  cpp.adapter = "::apache::thrift::test::AdaptTestMsAdapter",
)

typedef bool AdaptedBool (
  cpp.adapter = "::apache::thrift::test::TemplatedTestAdapter",
)

typedef byte AdaptedByte (
  cpp.adapter = "::apache::thrift::test::TemplatedTestAdapter",
)

typedef i16 AdaptedShort (
  cpp.adapter = "::apache::thrift::test::TemplatedTestAdapter",
)

typedef i32 AdaptedInteger (
  cpp.adapter = "::apache::thrift::test::TemplatedTestAdapter",
)

typedef i64 AdaptedLong (
  cpp.adapter = "::apache::thrift::test::TemplatedTestAdapter",
)

typedef double AdaptedDouble (
  cpp.adapter = "::apache::thrift::test::TemplatedTestAdapter",
)

typedef string AdaptedString (
  cpp.adapter = "::apache::thrift::test::TemplatedTestAdapter",
)

typedef binary (
  cpp.type = "::folly::IOBuf",
  cpp.adapter = "::apache::thrift::test::CustomProtocolAdapter",
) CustomProtocolType

struct AdaptTestStruct {
  1: DurationMs delay;
  2: CustomProtocolType custom;


  @cpp.ExperimentalAdapter{name = "::apache::thrift::test::AdaptTestMsAdapter"}
  3: i64 timeout;
}

struct AdaptTemplatedTestStruct {
  1: AdaptedBool adaptedBool;
  2: AdaptedByte adaptedByte;
  3: AdaptedShort adaptedShort;
  4: AdaptedInteger adaptedInteger;
  5: AdaptedLong adaptedLong;
  6: AdaptedDouble adaptedDouble;
  7: AdaptedString adaptedString;
  8: AdaptedBool adaptedBoolDefault = true;
  9: AdaptedByte adaptedByteDefault = 1;
  10: AdaptedShort adaptedShortDefault = 2;
  11: AdaptedInteger adaptedIntegerDefault = 3;
  12: AdaptedLong adaptedLongDefault = 4;
  13: AdaptedDouble adaptedDoubleDefault = 5;
  14: AdaptedString adaptedStringDefault = "6";
}

struct AdaptTemplatedNestedTestStruct {
  1: AdaptTemplatedTestStruct adaptedStruct;
}

union AdaptTestUnion {
  1: DurationMs delay;
  2: CustomProtocolType custom;
}
