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

namespace cpp2 facebook.thrift.annotation
namespace py3 facebook.thrift.annotation
namespace php facebook_thrift_annotation
namespace java.swift com.facebook.thrift.annotation
namespace java com.facebook.thrift.annotation_deprecated
namespace py.asyncio facebook_thrift_asyncio.annotation.scope
namespace go thrift.annotation.scope
namespace py thrift.annotation.scope

// Annotations that indicate which IDL definition a structured annotation can
// be place on.
//
// For example:
//     include "thrift/annotation/scope.thrift"
//
//     @scope.Struct
//     struct MyStructAnnotation {...}
//
//     @MyStructAnnotation // Good.
//     struct Foo{
//       @MyStructAnnotation // Compile-time failure. MyStructAnnotation is not
//                           // allowed on fields.
//       1: i32 my_field;
//     }
struct Struct {} (thrift.uri = "facebook.com/thrift/annotation/Struct")
struct Union {} (thrift.uri = "facebook.com/thrift/annotation/Union")
struct Exception {} (thrift.uri = "facebook.com/thrift/annotation/Exception")
struct Field {} (thrift.uri = "facebook.com/thrift/annotation/Field")
struct Typedef {} (thrift.uri = "facebook.com/thrift/annotation/Typedef")
struct Service {} (thrift.uri = "facebook.com/thrift/annotation/Service")
struct Interaction {} (
  thrift.uri = "facebook.com/thrift/annotation/Interaction",
)
struct Function {} (thrift.uri = "facebook.com/thrift/annotation/Function")
struct EnumValue {} (thrift.uri = "facebook.com/thrift/annotation/EnumValue")
struct Const {} (thrift.uri = "facebook.com/thrift/annotation/Const")

// Due to cython bug, we can not use `Enum` as class name directly
// https://github.com/cython/cython/issues/2474
struct FbthriftInternalEnum {}
typedef FbthriftInternalEnum Enum (
  thrift.uri = "facebook.com/thrift/annotation/Enum",
)
