/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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
cpp_include "thrift/lib/cpp2/Adapt.h"

@cpp.Adapter{name = "::apache::thrift::test::AdaptTestMsAdapter"}
typedef i64 DurationMs

@cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
typedef bool AdaptedBool

@cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
typedef byte AdaptedByte

@cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
typedef i16 AdaptedShort

@cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
typedef i32 AdaptedInteger

@cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
typedef i64 AdaptedLong

@cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
typedef double AdaptedDouble

@cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
typedef string AdaptedString

typedef AdaptedBool DoubleTypedefBool

@cpp.Adapter{name = "::apache::thrift::test::CustomProtocolAdapter"}
typedef binary (cpp.type = "::folly::IOBuf") CustomProtocolType

@cpp.Adapter{
  name = "::apache::thrift::IndirectionAdapter<::apache::thrift::test::IndirectionString>",
}
typedef string IndirectionString

struct AdaptTestStruct {
  1: DurationMs delay;
  2: CustomProtocolType custom;

  @cpp.Adapter{name = "::apache::thrift::test::AdaptTestMsAdapter"}
  3: i64 timeout;

  @cpp.Adapter{name = "::apache::thrift::test::AdapterWithContext"}
  4: i64 data;
  5: string meta;
  6: IndirectionString indirectionString;
  @cpp.Adapter{name = "::apache::thrift::test::AdapterWithContext"}
  7: string string_data;

  @cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
  8: AdaptedBool double_wrapped_bool;

  @cpp.Adapter{name = "::apache::thrift::test::AdapterWithContext"}
  9: AdaptedInteger double_wrapped_integer;
}

enum AdaptedEnum {
  Zero = 0,
  One = 1,
} (
  cpp.name = "ThriftAdaptedEnum",
  cpp.adapter = "::apache::thrift::StaticCastAdapter<::apache::thrift::test::basic::AdaptedEnum, ::apache::thrift::test::basic::ThriftAdaptedEnum>",
)

struct AdaptTemplatedTestStruct {
  1: AdaptedBool adaptedBool;
  2: AdaptedByte adaptedByte;
  3: AdaptedShort adaptedShort;
  4: AdaptedInteger adaptedInteger;
  5: AdaptedLong adaptedLong;
  6: AdaptedDouble adaptedDouble;
  7: AdaptedString adaptedString;
  @cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
  8: list<i64> adaptedList;
  @cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
  9: set<i64> adaptedSet;
  @cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
  10: map<i64, i64> adaptedMap;
  11: AdaptedBool adaptedBoolDefault = true;
  12: AdaptedByte adaptedByteDefault = 1;
  13: AdaptedShort adaptedShortDefault = 2;
  14: AdaptedInteger adaptedIntegerDefault = 3;
  15: AdaptedLong adaptedLongDefault = 4;
  16: AdaptedDouble adaptedDoubleDefault = 5;
  17: AdaptedString adaptedStringDefault = "6";
  18: AdaptedEnum adaptedEnum = AdaptedEnum.One;
  @cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
  19: list<i64> adaptedListDefault = [1];
  @cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
  20: set<i64> adaptedSetDefault = [1];
  @cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
  21: map<i64, i64> adaptedMapDefault = {1: 1};
  22: DoubleTypedefBool doubleTypedefBool;
}

struct AdaptTemplatedNestedTestStruct {
  1: AdaptTemplatedTestStruct adaptedStruct;
}

union AdaptTestUnion {
  1: DurationMs delay;
  2: CustomProtocolType custom;
} (cpp.name = "ThriftAdaptTestUnion")

struct AdaptedStruct {
  1: i64 data;
} (cpp.name = "ThriftAdaptedStruct")

@cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
typedef AdaptedStruct AdaptedTypedef

@cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
struct DirectlyAdaptedStruct {
  1: i64 data;
}

typedef DirectlyAdaptedStruct TypedefOfDirect

struct StructFieldAdaptedStruct {
  @cpp.Adapter{name = "::apache::thrift::test::TemplatedTestAdapter"}
  1: AdaptedStruct adaptedStruct;
  2: AdaptedTypedef adaptedTypedef;
  3: DirectlyAdaptedStruct directlyAdapted;
  4: TypedefOfDirect typedefOfAdapted;
}

struct CircularAdaptee {
  1: CircularStruct field;
}
struct CircularStruct {
  @cpp.Ref{type = cpp.RefType.Unique}
  1: optional AdaptedCircularAdaptee field;
}
@cpp.Adapter{
  name = "::apache::thrift::test::MemberAccessAdapter",
  adaptedType = "::apache::thrift::test::TaggedWrapper<CircularAdaptee, CircularStruct>",
}
typedef CircularAdaptee AdaptedCircularAdaptee

@cpp.Adapter{
  name = "::apache::thrift::test::TemplatedTestAdapter",
  underlyingName = "UnderlyingRenamedStruct",
}
struct RenamedStruct {
  1: i64 data;
}

@cpp.Adapter{
  name = "::apache::thrift::test::TemplatedTestAdapter",
  underlyingName = "UnderlyingSameNamespaceStruct",
  extraNamespace = "",
}
struct SameNamespaceStruct {
  1: i64 data;
}
