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

namespace php facebook_thrift_annotation
namespace java com.facebook.thrift.annotation_deprecated
namespace py.asyncio facebook_thrift_asyncio.annotation.thrift
namespace go thrift.annotation.thrift
namespace py thrift.annotation.thrift

////
// Thrift release state annotations.
////

/** Indicates a definition/feature may change in incompatible ways. */
@scope.Program
@scope.Definition
struct Beta {}

/**
 * Indicates a definition/feature should only be used with permission, may only
 * work in specific contexts, and may change in incompatible ways without notice.
 */
@scope.Program
@scope.Definition
struct Experimental {}

/** Indicates a definition/feature should no longer be used. */
// TODO(afuller): Add a validator to produce warnings when annotated definitions
// are used.
@Beta // TODO(afuller): Hook up to code gen.
@scope.Program
@scope.Definition
struct Deprecated {
  1: string message;
}

/**
 * Indicates  a definition/feature will be removed in the next release.
 *
 * Pleased migrate off of all @Legacy as soon as possible.
 */
// TODO(afuller): Add a linter to produce errors when annotated definitions
// are used.
@Deprecated // Legacy implies deprecated.
@scope.Transitive
struct Legacy {
  1: string message;
}

/**
 * Indicates additional backward compatibility restrictions, beyond the
 * standard Thrift required 'wire' compatibility.
 */
// TODO(afuller): Hook up to backward compat linter.
@scope.Structured
@Experimental // TODO: Fix naming style.
struct RequiresBackwardCompatibility {
  1: bool field_name = false;
}

/** Best-effort disables experimental features. */
@scope.Program
@scope.Definition
struct NoExperimental {}

/** Best-effort disables @Beta features. */
@NoExperimental // Implies no experimental
@scope.Transitive
struct NoBeta {}

/**
 * Best-effort disables @Legacy features.
 */
// TODO(ytj): Everyone should be able to test without legacy features. Fix
// compatibility with legacy reflection and move to @Beta.
@scope.Program
@scope.Definition
@Experimental
struct NoLegacy {}

/**
  * Best-effort disables @Deprecated features.
 *
 * Should only be enabled in `test` versions, as deprecated implies removing
 * the feature will break current usage (otherwise it would be @Legacy or
 * deleted)
 */
@NoLegacy // Implies NoLegacy
@Beta // Everyone should be able to test without deprecated features.
@scope.Transitive
struct NoDeprecated {}

////
// Thrift feature annotations.
////

// TODO(dokwon): Document.
// TODO(dokwon): Fix code gen and release to Beta.
@scope.Program
@scope.Structured
@scope.Field
@Experimental
struct TerseWrite {}

// TODO(dokwon): Document.
@scope.Field
@Beta
struct Box {}

// TODO(ytj): Document.
@scope.Field
@Beta
struct Mixin {}

/**
 * Option to serialize thrift struct in ascending field id order.
 *
 * This can potentially make serialized data size smaller in compact protocol,
 * since compact protocol can write deltas between subsequent field ids.
 */
@scope.Struct
@Experimental // TODO(ytj): Release to Beta.
struct SerializeInFieldIdOrder {}

////
// Thrift version annotations.
////

/** Enables all released v1 features. */
// TODO: Release features ;-).
@scope.Transitive
struct v1 {}

/**
 * Enables all beta v1 features.
 *
 * Beta features are guaranteed to *not* break unrelated Thrift features
 * so they should be relatively safe to test alongside other beta or
 * released Thrift features.
 */
@v1 // All v1 features.
@Beta // All uses of v1beta inherit `@Beta`.
@scope.Transitive
struct v1beta {}

/**
 * Enables all experimental v1 features.
 *
 * Use with *caution* and only with explicit permission. This may enable
 * features may change significantly without notice or not work correctly
 * in all contexts.
 */
@v1beta // All v1beta features.
@SerializeInFieldIdOrder
@NoLegacy // Disables features that will be removed.
@Experimental // All uses of v1alpha inherit `@Experimental`.
@scope.Transitive
struct v1alpha {}

/**
 * Enables experimental features, even those that are known to break common
 * use cases.
 */
@TerseWrite
@NoDeprecated // Remove deprecated features by default for tests.
@v1alpha // All v1alpha features.
@scope.Transitive
struct v1test {}

// TODO(afuller): Remove.
@Legacy
@scope.Program
@scope.Struct
@scope.Union
@scope.Exception
@scope.Service
struct NoLegacyAPIs {}
