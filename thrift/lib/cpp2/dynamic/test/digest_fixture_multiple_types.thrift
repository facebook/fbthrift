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

// Fixture for digest testing: struct + enum types.
// The opaque alias (UserId) is constructed programmatically in test code
// since opaque aliases are a runtime-only concept, not expressible in IDL.

include "thrift/annotation/thrift.thrift"

package "meta.com/thrift/test/digest/multiple_types"

namespace cpp2 apache.thrift.type_system.test.digest_multiple_types
namespace rust digest_fixture_multiple_types

@thrift.Uri{value = "meta.com/test/multi/Person"}
struct Person {
  1: string name;
  2: optional i32 age;
}

@thrift.Uri{value = "meta.com/test/multi/Status"}
enum Status {
  ACTIVE = 1,
  INACTIVE = 2,
}
