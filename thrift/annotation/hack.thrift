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

namespace cpp2 facebook.thrift.annotation.hack
namespace hack facebook.thrift.annotation
namespace py3 facebook.thrift.annotation.hack
namespace py thrift.annotation.hack
namespace java2 com.facebook.thrift.annotation.java
namespace java.swift com.facebook.thrift.annotation.java_swift
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

@scope.Field
@scope.Function
struct SkipCodegen {
  1: string reason;
} (thrift.uri = "facebook.com/thrift/annotation/hack/SkipCodegen")
