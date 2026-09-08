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

include "thrift/annotation/cpp.thrift"
include "thrift/annotation/thrift.thrift"

@thrift.AllowLegacyMissingUris
package;

namespace cpp2 apache.thrift.test
namespace py3 thrift.test.lazy_deserialization

struct Foo {
  1: list<double> field1; // fast to skip in CompactProtocol
  2: list<i32> field2; // slow to skip in CompactProtocol
  3: list<double> field3; // fast to skip in CompactProtocol
  4: list<i32> field4; // slow to skip in CompactProtocol
}

// Identical to Foo, except field3 and field4 are lazy
struct LazyFoo {
  1: list<double> field1;
  2: list<i32> field2;
  @cpp.Lazy
  3: list<double> field3;
  @cpp.Lazy{ref = true}
  4: list<i32> field4;
}

struct OptionalFoo {
  1: optional list<double> field1;
  2: optional list<i32> field2;
  3: optional list<double> field3;
  4: optional list<i32> field4;
}

struct OptionalLazyFoo {
  1: optional list<double> field1;
  2: optional list<i32> field2;
  @cpp.Lazy
  3: optional list<double> field3;
  @cpp.Lazy
  4: optional list<i32> field4;
}

struct OptionalBoxedLazyFoo {
  @thrift.Box
  1: optional list<double> field1;
  @thrift.Box
  2: optional list<i32> field2;
  @thrift.Box
  @cpp.Lazy
  3: optional list<double> field3;
  @cpp.Lazy
  @thrift.Box
  4: optional list<i32> field4;
}

struct FooNoChecksum {
  1: list<double> field1;
  2: list<i32> field2;
  3: list<double> field3;
  4: list<i32> field4;
}

@cpp.DisableLazyChecksum
struct LazyFooNoChecksum {
  1: list<double> field1;
  2: list<i32> field2;
  @cpp.Lazy
  3: list<double> field3;
  @cpp.Lazy
  4: list<i32> field4;
}

struct LazyCppRef {
  @cpp.Lazy
  @cpp.Ref{type = cpp.RefType.Unique}
  1: optional list<i32> field1;
  @cpp.Lazy
  @cpp.Ref{type = cpp.RefType.SharedMutable}
  2: optional list<i32> field2;
  @cpp.Lazy
  @cpp.Ref{type = cpp.RefType.Shared}
  3: optional list<i32> field3;
  @cpp.Lazy
  @cpp.Ref{type = cpp.RefType.Unique}
  @cpp.AllowLegacyNonOptionalRef
  4: list<i32> field4;
}

// Same as Foo, except adding index field explicitly
// Since we can't use negative as index field, we will change id
// in serialized data manually
struct IndexedFoo {
  102: i64 random_number;
  100: double serialized_data_size;

  1: list<double> field1;
  2: list<i32> field2;
  3: list<double> field3;
  4: list<i32> field4;

  101: map<i16, i64> field_id_to_size;
}

struct OptionalIndexedFoo {
  102: i64 random_number;
  100: double serialized_data_size;

  1: optional list<double> field1;
  2: optional list<i32> field2;
  3: optional list<double> field3;
  4: optional list<i32> field4;

  101: map<i16, i64> field_id_to_size;
}

struct Empty {}

const i32 kSizeId = 100;
const i32 kIndexId = 101;
const i32 kRandomNumberId = 102;

// 100 lazy fields, all `list<i32>` so every one lands in the index
// (integers, floats, enums and strings are all fixed-cost to skip and are
// therefore never indexed). Used to measure index lookup cost at scale.
struct LazyFoo100 {
  @cpp.Lazy
  1: list<i32> field1;
  @cpp.Lazy
  2: list<i32> field2;
  @cpp.Lazy
  3: list<i32> field3;
  @cpp.Lazy
  4: list<i32> field4;
  @cpp.Lazy
  5: list<i32> field5;
  @cpp.Lazy
  6: list<i32> field6;
  @cpp.Lazy
  7: list<i32> field7;
  @cpp.Lazy
  8: list<i32> field8;
  @cpp.Lazy
  9: list<i32> field9;
  @cpp.Lazy
  10: list<i32> field10;
  @cpp.Lazy
  11: list<i32> field11;
  @cpp.Lazy
  12: list<i32> field12;
  @cpp.Lazy
  13: list<i32> field13;
  @cpp.Lazy
  14: list<i32> field14;
  @cpp.Lazy
  15: list<i32> field15;
  @cpp.Lazy
  16: list<i32> field16;
  @cpp.Lazy
  17: list<i32> field17;
  @cpp.Lazy
  18: list<i32> field18;
  @cpp.Lazy
  19: list<i32> field19;
  @cpp.Lazy
  20: list<i32> field20;
  @cpp.Lazy
  21: list<i32> field21;
  @cpp.Lazy
  22: list<i32> field22;
  @cpp.Lazy
  23: list<i32> field23;
  @cpp.Lazy
  24: list<i32> field24;
  @cpp.Lazy
  25: list<i32> field25;
  @cpp.Lazy
  26: list<i32> field26;
  @cpp.Lazy
  27: list<i32> field27;
  @cpp.Lazy
  28: list<i32> field28;
  @cpp.Lazy
  29: list<i32> field29;
  @cpp.Lazy
  30: list<i32> field30;
  @cpp.Lazy
  31: list<i32> field31;
  @cpp.Lazy
  32: list<i32> field32;
  @cpp.Lazy
  33: list<i32> field33;
  @cpp.Lazy
  34: list<i32> field34;
  @cpp.Lazy
  35: list<i32> field35;
  @cpp.Lazy
  36: list<i32> field36;
  @cpp.Lazy
  37: list<i32> field37;
  @cpp.Lazy
  38: list<i32> field38;
  @cpp.Lazy
  39: list<i32> field39;
  @cpp.Lazy
  40: list<i32> field40;
  @cpp.Lazy
  41: list<i32> field41;
  @cpp.Lazy
  42: list<i32> field42;
  @cpp.Lazy
  43: list<i32> field43;
  @cpp.Lazy
  44: list<i32> field44;
  @cpp.Lazy
  45: list<i32> field45;
  @cpp.Lazy
  46: list<i32> field46;
  @cpp.Lazy
  47: list<i32> field47;
  @cpp.Lazy
  48: list<i32> field48;
  @cpp.Lazy
  49: list<i32> field49;
  @cpp.Lazy
  50: list<i32> field50;
  @cpp.Lazy
  51: list<i32> field51;
  @cpp.Lazy
  52: list<i32> field52;
  @cpp.Lazy
  53: list<i32> field53;
  @cpp.Lazy
  54: list<i32> field54;
  @cpp.Lazy
  55: list<i32> field55;
  @cpp.Lazy
  56: list<i32> field56;
  @cpp.Lazy
  57: list<i32> field57;
  @cpp.Lazy
  58: list<i32> field58;
  @cpp.Lazy
  59: list<i32> field59;
  @cpp.Lazy
  60: list<i32> field60;
  @cpp.Lazy
  61: list<i32> field61;
  @cpp.Lazy
  62: list<i32> field62;
  @cpp.Lazy
  63: list<i32> field63;
  @cpp.Lazy
  64: list<i32> field64;
  @cpp.Lazy
  65: list<i32> field65;
  @cpp.Lazy
  66: list<i32> field66;
  @cpp.Lazy
  67: list<i32> field67;
  @cpp.Lazy
  68: list<i32> field68;
  @cpp.Lazy
  69: list<i32> field69;
  @cpp.Lazy
  70: list<i32> field70;
  @cpp.Lazy
  71: list<i32> field71;
  @cpp.Lazy
  72: list<i32> field72;
  @cpp.Lazy
  73: list<i32> field73;
  @cpp.Lazy
  74: list<i32> field74;
  @cpp.Lazy
  75: list<i32> field75;
  @cpp.Lazy
  76: list<i32> field76;
  @cpp.Lazy
  77: list<i32> field77;
  @cpp.Lazy
  78: list<i32> field78;
  @cpp.Lazy
  79: list<i32> field79;
  @cpp.Lazy
  80: list<i32> field80;
  @cpp.Lazy
  81: list<i32> field81;
  @cpp.Lazy
  82: list<i32> field82;
  @cpp.Lazy
  83: list<i32> field83;
  @cpp.Lazy
  84: list<i32> field84;
  @cpp.Lazy
  85: list<i32> field85;
  @cpp.Lazy
  86: list<i32> field86;
  @cpp.Lazy
  87: list<i32> field87;
  @cpp.Lazy
  88: list<i32> field88;
  @cpp.Lazy
  89: list<i32> field89;
  @cpp.Lazy
  90: list<i32> field90;
  @cpp.Lazy
  91: list<i32> field91;
  @cpp.Lazy
  92: list<i32> field92;
  @cpp.Lazy
  93: list<i32> field93;
  @cpp.Lazy
  94: list<i32> field94;
  @cpp.Lazy
  95: list<i32> field95;
  @cpp.Lazy
  96: list<i32> field96;
  @cpp.Lazy
  97: list<i32> field97;
  @cpp.Lazy
  98: list<i32> field98;
  @cpp.Lazy
  99: list<i32> field99;
  @cpp.Lazy
  100: list<i32> field100;
}

// A single lazy `string` is fixed-cost to skip under Binary, so it never
// enters the index and nothing is ever hashed, even though the checksum stays
// enabled. Isolates the fixed per-struct cost of the index machinery from the
// cost of the payload.
struct LazyBlob {
  1: i64 id;
  @cpp.Lazy
  2: string blob;
}
