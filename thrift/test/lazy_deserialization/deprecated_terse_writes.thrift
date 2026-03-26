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

struct TerseFoo {
  @cpp.DeprecatedTerseWrite
  1: list<double> field1;
  @cpp.DeprecatedTerseWrite
  2: list<i32> field2;
  @cpp.DeprecatedTerseWrite
  3: list<double> field3;
  @cpp.DeprecatedTerseWrite
  4: list<i32> field4;
}

struct TerseLazyFoo {
  @cpp.DeprecatedTerseWrite
  1: list<double> field1;
  @cpp.DeprecatedTerseWrite
  2: list<i32> field2;
  @cpp.Lazy
  @cpp.DeprecatedTerseWrite
  3: list<double> field3;
  @cpp.Lazy
  @cpp.DeprecatedTerseWrite
  4: list<i32> field4;
}

struct TerseOptionalFoo {
  1: optional list<double> field1;
  2: optional list<i32> field2;
  3: optional list<double> field3;
  4: optional list<i32> field4;
}

struct TerseOptionalLazyFoo {
  1: optional list<double> field1;
  2: optional list<i32> field2;
  @cpp.Lazy
  3: optional list<double> field3;
  @cpp.Lazy
  4: optional list<i32> field4;
}
