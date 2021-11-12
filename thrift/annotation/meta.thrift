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

// This file defines meta-annotations targeted at Thrift framework authors
// rather than general Thrift users.

include "thrift/annotation/scope.thrift"

namespace cpp2 facebook.thrift.annotation
namespace py3 facebook.thrift.annotation
namespace php facebook_thrift_annotation
namespace java.swift com.facebook.thrift.annotation
namespace java com.facebook.thrift.annotation_deprecated
namespace py.asyncio facebook_thrift_asyncio.annotation.meta
namespace go thrift.annotation.meta
namespace py thrift.annotation.meta

// A meta-annotation that applies its sibling annotations through the one it
// annotates. For example:
//
//   @scope.Struct
//   @scope.Union
//   @scope.Exception
//   @meta.Transitive
//   struct Structured {}
//
// Annotating a Thrift struct with @Structured automatically applies
// @scope.Struct, @scope.Union and @scope.Exception annotations, i.e.
//
//   @Structured
//   struct MyAnnotation {}
//
// is equivalent to
//
//   @scope.Struct
//   @scope.Union
//   @scope.Exception
//   struct MyAnnotation {}
@scope.Struct
struct Transitive {} (thrift.uri = "facebook.com/thrift/annotation/Transitive")
