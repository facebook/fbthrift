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

#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>

#include <folly/portability/GTest.h>

using namespace apache::thrift::rocket;

TEST(FlagsTest, setting) {
  Flags flags;

  EXPECT_FALSE(flags.ignore());
  flags.ignore(true);
  EXPECT_TRUE(flags.ignore());
  flags.ignore(false);
  EXPECT_FALSE(flags.ignore());
}

TEST(FlagsTest, invalidFlags) {
  // Flags use the lower 10 bits
  constexpr uint8_t numBits = Flags::frameTypeOffset();
  ASSERT_EQ(numBits, 10);

  // Flags currently do not use the lowest 5 bits
  constexpr uint8_t unusedBits = 5;

  uint16_t over = 1 << numBits;
  uint16_t under = 1 << (unusedBits - 1);
  uint16_t allowed = 1 << (numBits - 1);

  auto mk = [](uint16_t bits) {
    Flags f(bits);
    (void)f;
  };

  EXPECT_DEATH(mk(over), "Check failed");
  EXPECT_THROW(mk(under), std::runtime_error);
  EXPECT_NO_THROW(mk(allowed));

  EXPECT_DEATH(mk(over | allowed), "Check failed");
  EXPECT_THROW(mk(under | allowed), std::runtime_error);
  EXPECT_DEATH(mk(over | under), "Check failed");

  EXPECT_DEATH(mk(over | under | allowed), "Check failed");
}
