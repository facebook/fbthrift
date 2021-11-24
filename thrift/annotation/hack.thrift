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

include "thrift/annotation/scope.thrift"

namespace hack facebook_thrift_annotation

// An experimental annotation that applies a Hack wrapper to fields.
// For example:
//
//   struct User {
//     @hack.ExperimentalAdapter{name="MyAdapter", adapted_generic_type="MyWrapper"}
//     1: i64 id;
//   }
//
@scope.Field
struct ExperimentalAdapter {
  // The name of a Hack adapter class used to convert between Thrift and native
  // Hack representation.
  1: string name;
  // The name of a Hack generic class used as adapted type
  2: optional string adapted_generic_type;
} (thrift.uri = "facebook.com/thrift/annotation/hack/ExperimentalAdapter")
