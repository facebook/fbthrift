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

#include <folly/Overload.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/test/gen-cpp2/IndirectionWrapperTest_types.h>

namespace apache::thrift::test {
int Foo::sum() {
  return (__fbthrift_data().bar_ref() ? __fbthrift_data().bar_ref()->sum()
                                      : 0) +
      *__fbthrift_data().field_ref();
}

int Bar::sum() {
  return __fbthrift_data().foo_ref()->sum() + *__fbthrift_data().field_ref();
}

TEST(Test, sum) {
  Foo foo;
  foo.__fbthrift_data().field_ref() = 1;
  EXPECT_EQ(foo.sum(), 1);

  Bar bar;
  bar.__fbthrift_data().foo_ref().emplace(foo);
  bar.__fbthrift_data().field_ref() = 20;
  EXPECT_EQ(bar.sum(), 21);

  Foo foo2;
  foo2.__fbthrift_data().bar_ref().reset(new auto(bar));
  foo2.__fbthrift_data().field_ref() = 300;
  EXPECT_EQ(foo2.sum(), 321);

  EXPECT_EQ(foo, foo);
  EXPECT_EQ(bar, bar);
  EXPECT_LT(foo, foo2);
}

TEST(Test, NonComparison) {
  Bar bar;
  Baz baz;
  auto has_less_to = folly::overload(
      [](auto&& lhs, auto&& rhs) -> decltype(lhs < rhs) { return true; },
      [](auto&&...) { return false; });
  static_assert(!has_less_to(bar, baz));
  static_assert(!has_less_to(baz, bar));

  auto has_equal_to = folly::overload(
      [](auto&& lhs, auto&& rhs) -> decltype(lhs == rhs) { return true; },
      [](auto&&...) { return false; });
  static_assert(!has_equal_to(bar, baz));
  static_assert(!has_equal_to(baz, bar));

  static_assert(has_less_to(bar, bar));
  static_assert(has_equal_to(bar, bar));
}
} // namespace apache::thrift::test
