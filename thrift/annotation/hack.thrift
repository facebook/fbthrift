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

include "thrift/annotation/scope.thrift"

package "facebook.com/thrift/annotation/hack"

namespace py thrift.annotation.hack
namespace java com.facebook.thrift.annotation.java_deprecated
namespace py.asyncio facebook_thrift_asyncio.annotation.hack
namespace go thrift.annotation.hack
namespace hs2 facebook.thrift.annotation.hack

// An experimental annotation that applies a Hack wrapper to fields.
// For example:
//
//   struct User {
//     @hack.FieldWrapper{name="MyWrapper"}
//     1: i64 id;
//   }
//
@scope.Field
struct FieldWrapper {
  // The name of a Hack wrapper class used to wrap the field
  1: string name;
} (thrift.uri = "facebook.com/thrift/annotation/hack/FieldWrapper")

// An annotation that applies a Hack adapter to types. For example:
// @hack.Adapter{name="\TimestampAdapter"}
// typedef i64 Timestamp;
//
//   struct User {
//     1: Timestamp account_creation_time;
//   }
//
// Here the field `account_creation_time` will have type TimestampAdapter::THackType instead of i64.
@scope.Typedef
@scope.Field
struct Adapter {
  // The name of a Hack adapter class that implements IThriftAdapter
  1: string name;
} (thrift.uri = "facebook.com/thrift/annotation/hack/Adapter")

@scope.Field
@scope.Function
struct SkipCodegen {
  1: string reason;
} (thrift.uri = "facebook.com/thrift/annotation/hack/SkipCodegen")

// This annotation is mainly used to rename symbols which can result in symbol
// conflict errors in Hack codegen.
// For ex: reserved keywords in Hack language, symbols with similar names from
// other files in Hack
struct Name {
  1: string name;
  2: string reason;
} (thrift.uri = "facebook.com/thrift/annotation/hack/Name")

// This annotation is for adding Hack attributes to union enums.
@scope.Union
struct UnionEnumAttributes {
  1: list<string> attributes;
} (thrift.uri = "facebook.com/thrift/annotation/hack/UnionEnumAttributes")

// This annotation is for using a custom trait for structs.
@scope.Struct
struct StructTrait {
  1: string name;
} (thrift.uri = "facebook.com/thrift/annotation/hack/StructTrait")

// This annotation is for adding Hack attributes.
struct Attributes {
  1: list<string> attributes;
}
