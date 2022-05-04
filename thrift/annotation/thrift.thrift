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

namespace cpp2 facebook.thrift.annotation.thrift
namespace py3 facebook.thrift.annotation
namespace php facebook_thrift_annotation_thrift
namespace java2 com.facebook.thrift.annotation.thrift
namespace java.swift com.facebook.thrift.annotation.thrift
namespace java com.facebook.thrift.annotation.thrift_deprecated
namespace py.asyncio facebook_thrift_asyncio.annotation.thrift
namespace go thrift.annotation.thrift
namespace py thrift.annotation.thrift

// Indicates changes that break compatibility
@scope.Struct
@scope.Union
@scope.Exception
struct RequiresBackwardCompatibility {
  1: bool field_name = false;
} (
  thrift.uri = "facebook.com/thrift/annotation/thrift/RequiresBackwardCompatibility",
)

// Indicates a definition may change in backwards incompatible ways.
@scope.Definition
struct Experimental {}

// Indicates a definition should no longer be used.
@Experimental // TODO(afuller): Hook up to code gen.
@scope.Definition
struct Deprecated {}

@scope.Program
@scope.Field
@scope.Struct
@Experimental
struct TerseWrite {} (
  thrift.uri = "facebook.com/thrift/annotation/thrift/TerseWrite",
)

@scope.Field
struct Box {} (thrift.uri = "facebook.com/thrift/annotation/thrift/Box")

// Option to serialize thrift struct in ascending field id order.
// This can potentially make serialized data size smaller in compact protocol,
// since compact protocol can write deltas between subsequent field ids.
@scope.Struct
@Experimental
struct SerializeInFieldIdOrder {} (
  thrift.uri = "facebook.com/thrift/annotation/thrift/SerializeInFieldIdOrder",
)

@scope.Program
@scope.Struct
@scope.Union
@scope.Exception
@scope.Service
@Experimental
struct NoLegacyAPIs {} (
  thrift.uri = "facebook.com/thrift/annotation/thrift/NoLegacyAPIs",
)

@TerseWrite
@NoLegacyAPIs
@scope.FbthriftInternalScopeTransitive
struct v1 {}
