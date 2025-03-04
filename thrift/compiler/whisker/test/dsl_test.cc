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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <thrift/compiler/whisker/dsl.h>

#include <string>
#include <type_traits>

namespace whisker::dsl {

template <typename... Cases>
using make_poly = make_polymorphic_native_handle<Cases...>;
template <typename... Cases>
using poly = polymorphic_native_handle<Cases...>;

TEST(DslTest, make_polymorphic_native_handle) {
  EXPECT_TRUE((std::is_same_v<make_poly<int>, poly<int>>));
  EXPECT_TRUE(
      (std::is_same_v<make_poly<int, std::string>, poly<int, std::string>>));

  struct t_A {};
  struct t_B : t_A {};
  struct t_B2 : t_B {};
  struct t_C : t_A {};
  struct t_C2 : t_C {};

  using C2 = make_poly<t_C2>;
  using C = make_poly<t_C, C2>;
  using B2 = make_poly<t_B2>;
  using B = make_poly<t_B, B2>;
  using A = make_poly<t_A, B, C>;

  EXPECT_TRUE((std::is_same_v<C2, poly<t_C2>>));
  EXPECT_TRUE((std::is_same_v<C, poly<t_C, t_C2>>));
  EXPECT_TRUE((std::is_same_v<B2, poly<t_B2>>));
  EXPECT_TRUE((std::is_same_v<B, poly<t_B, t_B2>>));
  EXPECT_TRUE((std::is_same_v<A, poly<t_A, t_B, t_B2, t_C, t_C2>>));
}

} // namespace whisker::dsl
