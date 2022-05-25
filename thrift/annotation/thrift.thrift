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
}

// Indicates a definition may change in backwards incompatible ways.
@scope.Program
@scope.Definition
struct Beta {}

// Indicates a definition should only be used with permission, and may
// change in incompatible ways or only work in specific contexts.
@scope.Program
@scope.Definition
struct Experimental {}

// Indicates a definition should no longer be used.
//
// TODO(afuller): Add a validator to produce warnings when annotated definitions
// are used.
@Beta // TODO(afuller): Hook up to code gen.
@scope.Program
@scope.Definition
struct Deprecated {
  1: string message;
}

// Indicates will be removed in the next release.
//
// Pleased migrate off of any feature mark as @Legacy.
//
// TODO(afuller): Add a linter to produce failures when annotated definitions
// are used.
@Deprecated // Legacy implies deprecated.
@scope.FbthriftInternalScopeTransitive
struct Legacy {
  1: string message;
}

@scope.Program
@scope.Struct
@scope.Field
@Experimental
struct TerseWrite {}

@Beta
@scope.Field
struct Box {}
@Beta
@scope.Field
struct Mixin {}

// Option to serialize thrift struct in ascending field id order.
// This can potentially make serialized data size smaller in compact protocol,
// since compact protocol can write deltas between subsequent field ids.
@scope.Struct
@Experimental
struct SerializeInFieldIdOrder {}

// Best-effort disables features that marked as experimental.
@scope.Program
@scope.Definition
struct NoExperimental {}

// Best-effort disables features that marked as beta.
@NoExperimental // Implies no experimental
@scope.FbthriftInternalScopeTransitive
struct NoBeta {}

// Best-effort disables features that are marked for removal.
@Beta // Everyone should be able to test without legacy features.
@scope.Program
@scope.Definition
struct NoLegacy {}

// Best-effort disables features that are marked as deprecated.
//
// Should only be enabled in `test` versions.
@Beta // Everyone should be able to test without deprecated features.
@NoLegacy // Implies NoLegacy
@scope.FbthriftInternalScopeTransitive
struct NoDeprecated {}

// Enables all released v1 features.
//
// TODO: Release features ;-).
@scope.FbthriftInternalScopeTransitive
struct v1 {}

// Enables all beta v1 features.
//
// Beta features are guaranteed to *not* break unrelated Thrift features
// so they should be relatively safe to test alongside other beta or
// released Thrift features.
@Beta // All uses of v1beta inherit `@Beta`.
@NoLegacy // Disables features that will be removed.
@v1 // All v1 features.
@scope.FbthriftInternalScopeTransitive
struct v1beta {}

// Enables all experimental v1 features.
//
// Use with *caution* and only with explicit permission. This may enable
// features may change significantly without notice or not work correctly
// in all contexts.
@Experimental // All uses of v1alpha inherit `@Experimental`.
@v1beta // All v1beta features.
@scope.FbthriftInternalScopeTransitive
struct v1alpha {}

// Enables experimental features, even those that are known to break common
// use cases.
@TerseWrite // TODO(dokwon): Fix code gen. Currently known working: c++
@v1alpha // All v1alpha features.
@NoDeprecated // Remove deprecated features by default for tests.
@scope.FbthriftInternalScopeTransitive
struct v1test {}

// TODO(afuller): Remove.
@Legacy
@scope.Program
@scope.Struct
@scope.Union
@scope.Exception
@scope.Service
struct NoLegacyAPIs {}
