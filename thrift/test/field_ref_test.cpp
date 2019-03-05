/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <memory>
#include <string>
#include <type_traits>

#include <thrift/lib/cpp2/BadFieldAccess.h>
#include <thrift/lib/cpp2/FieldRef.h>

#include <folly/portability/GTest.h>

using apache::thrift::bad_field_access;
using apache::thrift::field_ref;
using apache::thrift::optional_field_ref;

// A struct which is assignable but not constructible from int or other types
// to test forwarding in field_ref::operator=.
struct IntAssignable {
  IntAssignable& operator=(int v) noexcept {
    value = v;
    return *this;
  }

  int value = 0;
};

// A struct assignable from std::string with a non-noexcept assignment operator
// to test conditional noexcept in field_ref::operator=.
struct StringAssignable {
  StringAssignable& operator=(const std::string& v) {
    value = v;
    return *this;
  }

  std::string value;
};

class TestStruct {
 public:
  field_ref<std::string&> name() {
    return {name_, __isset.name};
  }

  field_ref<const std::string&> name() const {
    return {name_, __isset.name};
  }

  optional_field_ref<std::string&> opt_name() {
    return {name_, __isset.name};
  }

  optional_field_ref<const std::string&> opt_name() const {
    return {name_, __isset.name};
  }

  field_ref<IntAssignable&> int_assign() {
    return {int_assign_, __isset.int_assign};
  }

  optional_field_ref<IntAssignable&> opt_int_assign() {
    return {int_assign_, __isset.int_assign};
  }

  optional_field_ref<std::shared_ptr<int>&> ptr_ref() {
    return {ptr_, __isset.ptr};
  }

  field_ref<int&> int_val() {
    return {int_val_, __isset.int_val};
  }

  optional_field_ref<int&> opt_int_val() {
    return {int_val_, __isset.int_val};
  }

  field_ref<std::unique_ptr<int>&> uptr() & {
    return {uptr_, __isset.uptr};
  }

  field_ref<std::unique_ptr<int>&&> uptr() && {
    return {std::move(uptr_), __isset.uptr};
  }

  optional_field_ref<std::unique_ptr<int>&> opt_uptr() & {
    return {uptr_, __isset.uptr};
  }

  optional_field_ref<std::unique_ptr<int>&&> opt_uptr() && {
    return {std::move(uptr_), __isset.uptr};
  }

 private:
  std::string name_ = "default";
  IntAssignable int_assign_;
  std::shared_ptr<int> ptr_;
  int int_val_;
  std::unique_ptr<int> uptr_;

  struct __isset {
    bool name;
    bool int_assign;
    bool ptr;
    bool int_val;
    bool uptr;
  } __isset = {};
};

TEST(field_ref_test, access_default_value) {
  auto s = TestStruct();
  EXPECT_EQ(*s.name(), "default");
}

TEST(field_ref_test, assign) {
  auto s = TestStruct();
  EXPECT_FALSE(s.name().is_set());
  EXPECT_EQ(*s.name(), "default");
  s.name() = "foo";
  EXPECT_TRUE(s.name().is_set());
  EXPECT_EQ(*s.name(), "foo");
}

TEST(field_ref_test, copy_from) {
  auto s = TestStruct();
  auto s2 = TestStruct();
  s.name() = "foo";
  s.name().copy_from(s2.name());
  EXPECT_FALSE(s.name().is_set());
  s2.name() = "foo";
  s.name().copy_from(s2.name());
  EXPECT_TRUE(s.name().is_set());
  EXPECT_EQ(*s.name(), "foo");
}

TEST(field_ref_test, copy_from_const) {
  auto s = TestStruct();
  auto s2 = TestStruct();
  const auto& s_const = s2;
  s2.name() = "foo";
  s.name().copy_from(s_const.name());
  EXPECT_TRUE(s.name().is_set());
  EXPECT_EQ(*s.name(), "foo");
}

TEST(field_ref_test, copy_from_other_type) {
  auto s = TestStruct();
  auto s2 = TestStruct();
  s2.int_val() = 42;
  s.int_assign().copy_from(s2.int_val());
  EXPECT_TRUE(s.int_assign().is_set());
  EXPECT_EQ(s.int_assign()->value, 42);
}

template <template <typename> class FieldRef>
void check_is_assignable() {
  using IntAssignableRef = FieldRef<IntAssignable&>;
  static_assert(std::is_assignable<IntAssignableRef, int>::value, "");
  static_assert(!std::is_assignable<IntAssignableRef, std::string>::value, "");
  static_assert(std::is_nothrow_assignable<IntAssignableRef, int>::value, "");

  using StringAssignableRef = FieldRef<StringAssignable&>;
  static_assert(
      !std::is_nothrow_assignable<StringAssignableRef&, int>::value, "");
}

TEST(field_ref_test, is_assignable) {
  check_is_assignable<field_ref>();
}

TEST(field_ref_test, assign_forwards) {
  auto s = TestStruct();
  s.int_assign() = 42;
  EXPECT_TRUE(s.int_assign().is_set());
  EXPECT_EQ(s.int_assign()->value, 42);
}

TEST(field_ref_test, construct_const_from_mutable) {
  auto s = TestStruct();
  s.name() = "foo";
  field_ref<std::string&> name = s.name();
  field_ref<const std::string&> const_name = name;
  EXPECT_TRUE(const_name.is_set());
  EXPECT_EQ(*const_name, "foo");
}

template <typename T>
constexpr bool is_const_ref() {
  return std::is_reference<T>::value &&
      std::is_const<std::remove_reference_t<T>>::value;
}

TEST(field_ref_test, const_accessors) {
  TestStruct s;
  s.name() = "bar";
  field_ref<const std::string&> name = s.name();
  EXPECT_EQ(*name, "bar");
  EXPECT_EQ(name.value(), "bar");
  EXPECT_EQ(name->size(), 3);
  static_assert(is_const_ref<decltype(*name)>(), "");
  static_assert(is_const_ref<decltype(name.value())>(), "");
  static_assert(is_const_ref<decltype(*name.operator->())>(), "");
}

TEST(field_ref_test, mutable_accessors) {
  TestStruct s;
  field_ref<std::string&> name = s.name();
  *name = "foo";
  EXPECT_EQ(*name, "foo");
  name.value() = "bar";
  EXPECT_EQ(*name, "bar");
  name->assign("baz");
  EXPECT_EQ(*name, "baz");

  // Field is not marked as set but that's OK for unqualified field.
  EXPECT_FALSE(name.is_set());
}

using expander = int[];

template <template <typename, typename> class F, typename T, typename... Args>
void do_gen_pairs() {
  (void)expander{(F<T, Args>(), 0)...};
}

// Generates all possible pairs of types (T1, T2) from T (cross product) and
// invokes F<T1, T2>().
template <template <typename, typename> class F, typename... T>
void gen_pairs() {
  (void)expander{(do_gen_pairs<F, T, T...>(), 0)...};
}

template <template <typename, typename> class F>
void test_conversions() {
  gen_pairs<F, int&, const int&, int&&, const int&&>();
}

template <typename From, typename To>
struct FieldRefConversionChecker {
  static_assert(
      std::is_convertible<From, To>() ==
          std::is_convertible<field_ref<From>, field_ref<To>>(),
      "inconsistent implicit conversion");
};

TEST(field_ref_test, conversions) {
  test_conversions<FieldRefConversionChecker>();
}

TEST(field_ref_test, copy_list_initialization) {
  TestStruct s;
  s.name() = {};
}

TEST(field_ref_test, move) {
  TestStruct s;
  s.uptr() = std::make_unique<int>(42);
  auto rawp = s.uptr()->get();
  std::unique_ptr<int> p = *std::move(s).uptr();
  EXPECT_TRUE(!*s.uptr());
  EXPECT_EQ(p.get(), rawp);
}

TEST(optional_field_ref_test, access_default_value) {
  auto s = TestStruct();
  EXPECT_THROW(*s.opt_name(), bad_field_access);
  EXPECT_EQ(s.opt_name().value_unchecked(), "default");
}

TEST(optional_field_ref_test, assign) {
  auto s = TestStruct();
  EXPECT_FALSE(s.opt_name().has_value());
  s.opt_name() = "foo";
  EXPECT_TRUE(s.opt_name().has_value());
  EXPECT_EQ(*s.opt_name(), "foo");
}

TEST(optional_field_ref_test, copy_from) {
  auto s = TestStruct();
  auto s2 = TestStruct();
  s.opt_name() = "foo";
  s.opt_name().copy_from(s2.opt_name());
  EXPECT_FALSE(s.opt_name().has_value());
  s2.opt_name() = "foo";
  s.opt_name().copy_from(s2.opt_name());
  EXPECT_TRUE(s.opt_name().has_value());
  EXPECT_EQ(*s.opt_name(), "foo");
}

TEST(optional_field_ref_test, copy_from_const) {
  auto s = TestStruct();
  auto s2 = TestStruct();
  const auto& s_const = s2;
  s2.opt_name() = "foo";
  s.opt_name().copy_from(s_const.opt_name());
  EXPECT_TRUE(s.opt_name().has_value());
  EXPECT_EQ(*s.opt_name(), "foo");
}

TEST(optional_field_ref_test, copy_from_other_type) {
  auto s = TestStruct();
  auto s2 = TestStruct();
  s2.opt_int_val() = 42;
  s.opt_int_assign().copy_from(s2.opt_int_val());
  EXPECT_TRUE(s.opt_int_assign().has_value());
  EXPECT_EQ(s.opt_int_assign()->value, 42);
}

TEST(optional_field_ref_test, is_assignable) {
  check_is_assignable<optional_field_ref>();
}

TEST(optional_field_ref_test, assign_forwards) {
  auto s = TestStruct();
  s.opt_int_assign() = 42;
  EXPECT_TRUE(s.opt_int_assign().has_value());
  EXPECT_EQ(s.opt_int_assign()->value, 42);
}

TEST(optional_field_ref_test, reset) {
  auto s = TestStruct();
  EXPECT_FALSE(s.ptr_ref().has_value());
  s.ptr_ref().reset();
  EXPECT_FALSE(s.ptr_ref().has_value());
  auto ptr = std::make_shared<int>(42);
  s.ptr_ref() = ptr;
  EXPECT_TRUE(s.ptr_ref().has_value());
  EXPECT_EQ(ptr.use_count(), 2);
  s.ptr_ref().reset();
  EXPECT_FALSE(s.ptr_ref().has_value());
  EXPECT_EQ(ptr.use_count(), 1);
}

TEST(optional_field_ref_test, construct_const_from_mutable) {
  auto s = TestStruct();
  s.opt_name() = "foo";
  optional_field_ref<std::string&> name = s.opt_name();
  optional_field_ref<const std::string&> const_name = name;
  EXPECT_TRUE(const_name.has_value());
  EXPECT_EQ(*const_name, "foo");
}

TEST(optional_field_ref_test, const_accessors) {
  TestStruct s;
  s.opt_name() = "bar";
  optional_field_ref<const std::string&> name = s.opt_name();
  EXPECT_EQ(*name, "bar");
  EXPECT_EQ(name.value(), "bar");
  EXPECT_EQ(name->size(), 3);
  static_assert(is_const_ref<decltype(*name)>(), "");
  static_assert(is_const_ref<decltype(name.value())>(), "");
  static_assert(is_const_ref<decltype(*name.operator->())>(), "");
  s.opt_uptr() = std::make_unique<int>(42);
  std::move(s).opt_uptr()->get();
}

TEST(optional_field_ref_test, mutable_accessors) {
  TestStruct s;
  s.opt_name() = "initial";
  optional_field_ref<std::string&> name = s.opt_name();
  *name = "foo";
  EXPECT_EQ(*name, "foo");
  name.value() = "bar";
  EXPECT_EQ(*name, "bar");
  name->assign("baz");
  EXPECT_EQ(*name, "baz");
  EXPECT_TRUE(name.has_value());
}

TEST(optional_field_ref_test, value_or) {
  TestStruct s;
  EXPECT_EQ("foo", s.opt_name().value_or("foo"));
  s.opt_name() = "bar";
  EXPECT_EQ("bar", s.opt_name().value_or("foo"));
}

TEST(optional_field_ref_test, bad_field_access) {
  TestStruct s;
  optional_field_ref<std::string&> name = s.opt_name();
  EXPECT_THROW(*name = "foo", bad_field_access);
  EXPECT_THROW(name.value() = "bar", bad_field_access);
  EXPECT_THROW(name->assign("baz"), bad_field_access);
}

TEST(optional_field_ref_test, convert_to_bool) {
  TestStruct s;
  if (auto name = s.opt_name()) {
    EXPECT_TRUE(false);
  }
  s.opt_name() = "foo";
  if (auto name = s.opt_name()) {
    // Do nothing.
  } else {
    EXPECT_TRUE(false);
  }
  EXPECT_FALSE((std::is_convertible<decltype(s.opt_name()), bool>::value));
}

template <typename From, typename To>
struct OptionalFieldRefConversionChecker {
  static_assert(
      std::is_convertible<From, To>() ==
          std::is_convertible<
              optional_field_ref<From>,
              optional_field_ref<To>>(),
      "inconsistent implicit conversion");
};

TEST(optional_field_ref_test, conversions) {
  test_conversions<OptionalFieldRefConversionChecker>();
  TestStruct s;
  optional_field_ref<std::string&> lvalue_ref = s.opt_name();
  optional_field_ref<std::string&&> rvalue_ref =
      static_cast<optional_field_ref<std::string&&>>(lvalue_ref);
  optional_field_ref<const std::string&&> crvalue_ref =
      static_cast<optional_field_ref<const std::string&&>>(lvalue_ref);
}

TEST(optional_field_ref_test, copy_list_initialization) {
  TestStruct s;
  s.opt_name() = {};
}

TEST(optional_field_ref_test, move) {
  TestStruct s;
  s.opt_uptr() = std::make_unique<int>(42);
  auto rawp = s.opt_uptr()->get();
  std::unique_ptr<int> p = *std::move(s).opt_uptr();
  EXPECT_TRUE(!*s.opt_uptr());
  EXPECT_EQ(p.get(), rawp);
}
