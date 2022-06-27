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

#include <folly/portability/GTest.h>
#include <thrift/lib/thrift/gen-cpp2/protocol_constants.h>
#include <thrift/lib/thrift/gen-cpp2/protocol_types.h>

using apache::thrift::protocol::Mask;
using apache::thrift::protocol::protocol_constants;

namespace apache::thrift::test {
TEST(FieldMaskTest, Example) {
  // example masks
  // inclusive{7: exclusive{},
  //           9: inclusive{5: exclusive{},
  //                        6: exclusive{}}}
  Mask m;
  auto& inclusive = m.inclusive_ref().emplace();
  inclusive[7] = protocol_constants::allMask();
  auto& nestedInclusive = inclusive[9].inclusive_ref().emplace();
  nestedInclusive[5] = protocol_constants::allMask();
  nestedInclusive[6] = protocol_constants::allMask();
  inclusive[8] = protocol_constants::noneMask(); // not required
}

TEST(FieldMaskTest, Constant) {
  EXPECT_EQ(protocol_constants::allMask().exclusive_ref()->size(), 0);
  EXPECT_EQ(protocol_constants::noneMask().inclusive_ref()->size(), 0);
}
} // namespace apache::thrift::test
