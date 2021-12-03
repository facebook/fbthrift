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

#include <thrift/lib/cpp2/type/AnyValue.h>

#include <folly/portability/GTest.h>

#include <folly/Poly.h>

namespace apache::thrift::type::detail {
namespace {

struct IntValue {
  int value;

  bool empty() const { return value == 0; }
  void clear() { value = 0; }
  bool identical(IntValue other) const { return value == other.value; }
};

TEST(AnyDataTest, IAnyData) {
  folly::Poly<IAnyData> data = IntValue{1};
  EXPECT_FALSE(data.empty());
  EXPECT_TRUE(data.identical(IntValue{1}));

  data.clear();
  EXPECT_TRUE(data.empty());
  EXPECT_EQ(folly::poly_cast<IntValue&>(data).value, 0);
  EXPECT_TRUE(data.identical(IntValue{0}));
  EXPECT_FALSE(data.identical(IntValue{1}));
}

// TODO(afuller): Add more test converage.

} // namespace
} // namespace apache::thrift::type::detail
