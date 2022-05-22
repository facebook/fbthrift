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

package "facebook.com/thrift/annotation"

namespace cpp2 facebook.thrift.annotation
namespace py3 facebook.thrift.annotation
namespace php facebook_thrift_annotation
namespace java2 com.facebook.thrift.annotation
namespace java.swift com.facebook.thrift.annotation
namespace java com.facebook.thrift.annotation_deprecated
namespace py.asyncio facebook_thrift_asyncio.annotation.thrift
namespace go thrift.annotation.thrift
namespace py thrift.annotation.thrift

// Indicates changes that break compatibility
@scope.Struct
@scope.Union
@scope.Exception
struct RequiresBackwardCompatibility {
  1: bool field_name = false;
} (thrift.uri = "facebook.com/thrift/annotation/RequiresBackwardCompatibility")

// Indicates a definition may change in backwards incompatible ways.
@scope.Program
@scope.Definition
struct Beta {} (thrift.uri = "facebook.com/thrift/annotation/Beta")

// Indicates a definition should only be used with permission, and may
// change in incompatible ways or only work in specific contexts.
@scope.Program
@scope.Definition
struct Experimental {} (
  thrift.uri = "facebook.com/thrift/annotation/Experimental",
)

// Indicates a definition should no longer be used.
//
// TODO(afuller): Add a validator to produce warnings when annotated definitions
// are used.
@Beta // TODO(afuller): Hook up to code gen.
@scope.Program
@scope.Definition
struct Deprecated {}

// Indicates will be removed in the next release.
//
// Pleased migrate off of any feature mark as @Legacy.
//
// TODO(afuller): Add a linter to produce failures when annotated definitions
// are used.
@Deprecated // Legacy implies deprecated.
@scope.FbthriftInternalScopeTransitive
struct Legacy {}

@scope.Program
@scope.Struct
@scope.Field
@Experimental
struct TerseWrite {} (thrift.uri = "facebook.com/thrift/annotation/TerseWrite")

@Beta
@scope.Field
struct Box {} (thrift.uri = "facebook.com/thrift/annotation/Box")
@Beta
@scope.Field
struct Mixin {} (thrift.uri = "facebook.com/thrift/annotation/Mixin")

// Option to serialize thrift struct in ascending field id order.
// This can potentially make serialized data size smaller in compact protocol,
// since compact protocol can write deltas between subsequent field ids.
@scope.Struct
@Experimental
struct SerializeInFieldIdOrder {} (
  thrift.uri = "facebook.com/thrift/annotation/SerializeInFieldIdOrder",
)

// Disables features that are marked for removal.
//
// TODO(afuller): Move to `@Beta`, as everyone should able to test without
// legacy features.
// TODO(afuller): Rename to just `NoLegacy` because this can apply any
// 'feature'.
// TODO(afuller): Add `@NoDeprecated` that removes depreciated features
// (e.g. features that we are planning to remove)
@Experimental
@scope.Program
@scope.Struct
@scope.Union
@scope.Exception
@scope.Service
struct NoLegacyAPIs {} (
  thrift.uri = "facebook.com/thrift/annotation/NoLegacyAPIs",
)

// Enables all released v1 features.
//
// TODO: Release features ;-).
@scope.FbthriftInternalScopeTransitive
struct v1 {}

// Enables all experimental v1 features.
//
// Use with *caution* and only with explicit permission. This may enable
// features may change significantly without notice or not work correctly
// in all contexts.
//
@Experimental // All uses of v1alpha inherit `@Experimental`.
@NoLegacyAPIs
@v1 // All v1 features.
@scope.FbthriftInternalScopeTransitive
struct v1alpha {}

// Enables experimental features, even those that are known to break common
// use cases.
@TerseWrite // TODO(dokwon): Fix code gen. Currently known working: c++
@v1alpha // All v1alpha features.
@scope.FbthriftInternalScopeTransitive
struct v1test {}
