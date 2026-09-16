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

#include <thrift/lib/cpp2/protocol/Json5Protocol.h>

#include <cmath>
#include <limits>
#include <string>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/type/Tag.h>

namespace apache::thrift {
namespace {

// Primitive tags only: thrift codegen is not available in the xplat cell.

TEST(Json5ProtocolAvailabilityTest, RoundTripsPrimitive) {
  EXPECT_EQ(Json5ProtocolUtils::fromJson5<type::i32_t>("42"), 42);
  EXPECT_EQ(
      Json5ProtocolUtils::fromJson5<type::i32_t>(
          Json5ProtocolUtils::toJson5<type::i32_t>(42)),
      42);
  EXPECT_EQ(
      Json5ProtocolUtils::fromJson5<type::string_t>(
          Json5ProtocolUtils::toJson5<type::string_t>("hi")),
      "hi");
}

TEST(Json5ProtocolAvailabilityTest, EncodesNanUnquoted) {
  constexpr double kNan = std::numeric_limits<double>::quiet_NaN();
  EXPECT_EQ(Json5ProtocolUtils::toJson5<type::double_t>(kNan), "NaN");
  EXPECT_EQ(Json5ProtocolUtils::toBasicJson<type::double_t>(kNan), "\"NaN\"");
  EXPECT_TRUE(std::isnan(Json5ProtocolUtils::fromJson5<type::double_t>("NaN")));
}

} // namespace
} // namespace apache::thrift
