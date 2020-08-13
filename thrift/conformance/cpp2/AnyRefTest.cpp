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

#include <thrift/conformance/cpp2/AnyRef.h>

#include <gtest/gtest.h>

using namespace ::testing;

namespace apache::thrift::conformance {
namespace {

TEST(AnyRefTest, DocExmaple) {
  std::any foo;
  any_ref fooRef = foo;
  assert(fooRef.has_value()); // Is set to an empty std::any.
  // The empty std::any advertises the any type.
  assert(fooRef.type() == typeid(std::any));

  // The std::any value can be set through the any_ref .
  any_cast<std::any&>(fooRef) = 1;
  // Now it advertises the int type.
  assert(fooRef.type() == typeid(int));
  // Which can be accessed directly through the any_ref.
  any_cast<int&>(fooRef) = 2;
  // The original value shows the change.
  assert(any_cast<int>(foo) == 2);

  // The std::any is still accessible.
  any_cast<std::any&>(fooRef) = 2.0;
  assert(fooRef.type() == typeid(double));
}

TEST(AnyRefTest, Empty) {
  any_ref empty;
  EXPECT_FALSE(empty);
  EXPECT_FALSE(empty.has_value());
  EXPECT_EQ(empty.type(), typeid(void));
  EXPECT_FALSE(empty.is_const());
  EXPECT_FALSE(empty.is_rvalue_reference());

  EXPECT_THROW(any_cast<const int&>(empty), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<const int&>(empty), std::bad_any_cast);
  EXPECT_THROW(any_cast<void*>(empty), std::bad_any_cast);

  // Uncomment to get epxected compiler error.
  // EXPECT_EQ(ref, std::nullopt);
}

TEST(AnyRefTest, Const) {
  constexpr int i = 1;
  any_ref ref = i;
  EXPECT_EQ(ref.type(), typeid(int));
  EXPECT_TRUE(ref.is_const());
  EXPECT_FALSE(ref.is_rvalue_reference());

  EXPECT_EQ(any_cast<const int>(ref), i);
  EXPECT_EQ(any_cast<int>(ref), i);
  EXPECT_THROW(any_cast<double>(ref), std::bad_any_cast);

  EXPECT_EQ(&any_cast<const int&>(ref), &i);
  EXPECT_EQ(&any_cast_exact<const int&>(ref), &i);

  EXPECT_THROW(any_cast<const int&&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<const int&&>(ref), std::bad_any_cast);

  EXPECT_THROW(any_cast<int&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<int&>(ref), std::bad_any_cast);

  EXPECT_THROW(any_cast<const double&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<const double&>(ref), std::bad_any_cast);

  ref = std::move(i);
  EXPECT_EQ(ref.type(), typeid(int));
  EXPECT_TRUE(ref.is_const());
  EXPECT_TRUE(ref.is_rvalue_reference());

  EXPECT_EQ(any_cast<const int>(ref), i);
  EXPECT_EQ(any_cast<int>(ref), i);
  EXPECT_THROW(any_cast<double>(ref), std::bad_any_cast);

  EXPECT_EQ(&any_cast<const int&>(ref), &i);
  EXPECT_THROW(&any_cast_exact<const int&>(ref), std::bad_any_cast);

  EXPECT_EQ(any_cast<const int&&>(ref), i);
  EXPECT_EQ(any_cast_exact<const int&&>(ref), i);

  EXPECT_THROW(any_cast<int&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<int&>(ref), std::bad_any_cast);

  EXPECT_THROW(any_cast<const double&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<const double&>(ref), std::bad_any_cast);
}

TEST(AnyRefTest, NonConst) {
  any_ref ref;
  EXPECT_FALSE(ref);
  int i = 2;
  ref = i;
  EXPECT_TRUE(ref);
  EXPECT_TRUE(ref.has_value());
  EXPECT_FALSE(ref.is_const());
  EXPECT_FALSE(ref.is_rvalue_reference());

  EXPECT_EQ(&any_cast<const int&>(ref), &i);
  EXPECT_THROW(any_cast_exact<const int&>(ref), std::bad_any_cast);

  EXPECT_EQ(&any_cast<int&>(ref), &i);
  EXPECT_EQ(&any_cast_exact<int&>(ref), &i);

  any_cast<int&>(ref) = 3;
  EXPECT_EQ(i, 3);
  any_cast_exact<int&>(ref) = 4;
  EXPECT_EQ(i, 4);

  ref = std::move(i);
  EXPECT_FALSE(ref.is_const());
  EXPECT_TRUE(ref.is_rvalue_reference());

  EXPECT_EQ(any_cast<const int>(ref), i);
  EXPECT_EQ(any_cast<int>(ref), i);
  EXPECT_THROW(any_cast<double>(ref), std::bad_any_cast);

  EXPECT_EQ(&any_cast<int&>(ref), &i);
  EXPECT_THROW(any_cast_exact<int&>(ref), std::bad_any_cast);

  EXPECT_EQ(any_cast<int&&>(ref), i);
  EXPECT_EQ(any_cast_exact<int&&>(ref), i);

  EXPECT_EQ(&any_cast<const int&>(ref), &i);
  EXPECT_THROW(any_cast_exact<const int&>(ref), std::bad_any_cast);

  EXPECT_EQ(any_cast<const int&&>(ref), i);
  EXPECT_THROW(any_cast_exact<const int&&>(ref), std::bad_any_cast);

  EXPECT_THROW(any_cast<double&&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<double&&>(ref), std::bad_any_cast);

  EXPECT_THROW(any_cast<double&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<double&>(ref), std::bad_any_cast);

  const float f = 1.5f;
  ref = f;
  EXPECT_TRUE(ref.is_const());
  EXPECT_THROW(any_cast<double>(ref), std::bad_any_cast);
  EXPECT_EQ(any_cast<float>(ref), 1.5f);
  EXPECT_EQ(any_cast_exact<const float&>(ref), 1.5f);
}

TEST(AnyRefTest, NullOpt) {
  // Should not beable to create an any reference to nullopt.
  // Uncomment to produce expected compiler error.
  // any_ref empty(std::nullopt);
}

TEST(AnyRefTest, Volatile) {
  // Uncomment to get expected compile-time error.
  // volatile int i;
  // any_ref ref = i;
}

TEST(AnyRefTest, MoveOnly) {
  auto i = std::make_unique<int>(2);
  any_ref ref = i;
  EXPECT_FALSE(ref.is_const());
  EXPECT_FALSE(ref.is_rvalue_reference());

  EXPECT_EQ(&any_cast<std::unique_ptr<int>&>(ref), &i);
  EXPECT_EQ(&any_cast_exact<std::unique_ptr<int>&>(ref), &i);

  EXPECT_THROW(any_cast<std::unique_ptr<int>&&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<std::unique_ptr<int>&&>(ref), std::bad_any_cast);

  // Uncomment to get expected compile-time error.
  // any_cast<std::unique_ptr<int>>(ref);

  ref = std::move(i);
  EXPECT_NE(i, nullptr);
  EXPECT_FALSE(ref.is_const());
  EXPECT_TRUE(ref.is_rvalue_reference());

  EXPECT_EQ(&any_cast<std::unique_ptr<int>&>(ref), &i);
  EXPECT_THROW(any_cast_exact<std::unique_ptr<int>&>(ref), std::bad_any_cast);

  EXPECT_EQ(any_cast<std::unique_ptr<int>&&>(ref).get(), i.get());
  EXPECT_EQ(any_cast_exact<std::unique_ptr<int>&&>(ref).get(), i.get());
}

TEST(AnyRefTest, ImplicitCapture) {
  auto func = [](any_ref ref) -> int& { return any_cast<int&>(ref); };

  EXPECT_THROW(func({}), std::bad_any_cast);
  EXPECT_THROW(func({1.0}), std::bad_any_cast);
  EXPECT_EQ(func(1), 1);
  int i = 1;
  EXPECT_EQ(&func(i), &i);
  func(i) = 2;
  EXPECT_EQ(i, 2);
  func(std::move(i)) = 3;
  EXPECT_EQ(i, 3);
  any_ref ref = i;
  EXPECT_EQ(&func(ref), &i);
}

TEST(AnyRefTest, Pointer) {
  int i = 2;
  int* pi = &i;
  any_ref ref = pi;

  EXPECT_THROW(any_cast<const int&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<const int&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast<int>(ref), std::bad_any_cast);
  EXPECT_EQ(any_cast<int*>(ref), &i);
  EXPECT_EQ(&any_cast<int*&>(ref), &pi);
  EXPECT_EQ(&any_cast_exact<int*&>(ref), &pi);

  // Uncomment to get epxected compiler error.
  // EXPECT_NE(ref, nullptr);
}

TEST(AnyRefTest, AnyTransparency_Empty) {
  std::any a;
  any_ref ref = a;
  EXPECT_TRUE(ref.has_value());
  EXPECT_EQ(ref.type(), typeid(std::any));
  EXPECT_FALSE(ref.is_const());
  EXPECT_FALSE(ref.is_rvalue_reference());

  EXPECT_EQ(&any_cast<std::any&>(ref), &a);
  EXPECT_THROW(any_cast<int>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast<double>(ref), std::bad_any_cast);

  EXPECT_THROW(any_cast<int&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<int&>(ref), std::bad_any_cast);

  EXPECT_FALSE(any_cast<std::any>(ref).has_value());

  EXPECT_FALSE(any_cast<std::any&>(ref).has_value());
  EXPECT_FALSE(any_cast_exact<std::any&>(ref).has_value());

  EXPECT_FALSE(any_cast<const std::any&>(ref).has_value());
  EXPECT_THROW(any_cast_exact<const std::any&>(ref), std::bad_any_cast);

  // Can be assigned via the any_ref
  any_cast<std::any&>(ref) = 1;
  EXPECT_EQ(ref.type(), typeid(int));
  EXPECT_FALSE(ref.is_const());
  EXPECT_FALSE(ref.is_rvalue_reference());
  EXPECT_EQ(std::any_cast<int&>(a), 1);
}

TEST(AnyRefTest, AnyTransparency_NonConst) {
  int i = 2;
  std::any a = i;
  any_ref ref = a;
  EXPECT_EQ(ref.type(), typeid(int));
  EXPECT_FALSE(ref.is_const());
  EXPECT_FALSE(ref.is_rvalue_reference());

  EXPECT_EQ(any_cast<int>(ref), 2);

  EXPECT_EQ(&any_cast<std::any&>(ref), &a);
  EXPECT_EQ(&any_cast_exact<std::any&>(ref), &a);

  EXPECT_EQ(&any_cast<const std::any&>(ref), &a);
  EXPECT_THROW(any_cast_exact<const std::any&>(ref), std::bad_any_cast);

  EXPECT_THROW(any_cast<std::any&&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<std::any&&>(ref), std::bad_any_cast);

  EXPECT_EQ(&any_cast<int&>(ref), &std::any_cast<int&>(a));
  EXPECT_EQ(&any_cast_exact<int&>(ref), &std::any_cast<int&>(a));

  EXPECT_EQ(&any_cast<const int&>(ref), &std::any_cast<const int&>(a));
  EXPECT_THROW(any_cast_exact<const int&>(ref), std::bad_any_cast);

  EXPECT_THROW(any_cast<int&&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<int&&>(ref), std::bad_any_cast);

  EXPECT_NE(&any_cast<int&>(ref), &i);
  EXPECT_NE(&any_cast_exact<int&>(ref), &i);

  EXPECT_THROW(any_cast<double>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<double&>(ref), std::bad_any_cast);

  any_cast<int&>(ref) = 3;
  EXPECT_EQ(i, 2);
  EXPECT_EQ(any_cast<int&>(a), 3);
}

TEST(AnyRefTest, AnyTransparency_Const) {
  int i = 2;
  std::any a = i;
  const std::any& ca = a;
  any_ref ref = ca;
  EXPECT_EQ(ref.type(), typeid(int));
  EXPECT_TRUE(ref.is_const());
  EXPECT_FALSE(ref.is_rvalue_reference());

  EXPECT_EQ(any_cast<int>(ref), 2);

  EXPECT_THROW(any_cast<std::any&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<std::any&>(ref), std::bad_any_cast);

  EXPECT_EQ(&any_cast<const std::any&>(ref), &a);
  EXPECT_EQ(&any_cast_exact<const std::any&>(ref), &a);

  EXPECT_THROW(any_cast<std::any&&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<std::any&&>(ref), std::bad_any_cast);

  EXPECT_THROW(any_cast<int&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<int&>(ref), std::bad_any_cast);

  EXPECT_EQ(&any_cast<const int&>(ref), &std::any_cast<const int&>(a));
  EXPECT_EQ(&any_cast_exact<const int&>(ref), &std::any_cast<const int&>(a));

  EXPECT_THROW(any_cast<const int&&>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<const int&&>(ref), std::bad_any_cast);

  EXPECT_NE(&any_cast<const int&>(ref), &i);
  EXPECT_NE(&any_cast_exact<const int&>(ref), &i);

  EXPECT_THROW(any_cast<double>(ref), std::bad_any_cast);
  EXPECT_THROW(any_cast_exact<const double&>(ref), std::bad_any_cast);
}

} // namespace
} // namespace apache::thrift::conformance

namespace {

TEST(AnyRefTest, ADL) {
// Requires c++20 to work.
#if __cplusplus >= 202002L
  std::any a = 1;
  apache::thrift::conformance::any_ref ra = a;
  EXPECT_EQ(any_cast<int>(a), 1);
  EXPECT_EQ(any_cast<int>(ra), 1);
#endif
}

TEST(AnyRefTest, ADL_using) {
  std::any a = 1;
  apache::thrift::conformance::any_ref ra = a;
  using apache::thrift::conformance::any_cast;
  using std::any_cast;
  EXPECT_EQ(any_cast<int>(a), 1);
  EXPECT_EQ(any_cast<int>(ra), 1);
}

TEST(AnyRefTest, ADL_using_std) {
  std::any a = 1;
  apache::thrift::conformance::any_ref ra = a;
  using std::any_cast;
  EXPECT_EQ(any_cast<int>(a), 1);
  EXPECT_EQ(any_cast<int>(ra), 1);
}

TEST(AnyRefTest, ADL_using_thrift) {
  std::any a = 1;
  apache::thrift::conformance::any_ref ra = a;
  using apache::thrift::conformance::any_cast;
  EXPECT_EQ(any_cast<int>(a), 1);
  EXPECT_EQ(any_cast<int>(ra), 1);
}

} // namespace
