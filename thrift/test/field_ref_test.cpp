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

#include <thrift/lib/cpp2/gen/bad_field_access.h>
#include <thrift/lib/cpp2/gen/field_ref.h>

#include <gtest/gtest.h>

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

// A struct constructible from std::string with a non-noexcept ctor
// to test conditional noexcept in field_ref::emplace.
struct StringConstructible {
  explicit StringConstructible(const std::string& v = {}) {
    value = v;
  }

  std::string value;
};

class TestStruct {
 public:
  field_ref<std::string> name() {
    return {name_, __isset.name};
  }

  field_ref<const std::string> name() const {
    return {name_, __isset.name};
  }

  optional_field_ref<std::string> opt_name() {
    return {name_, __isset.name};
  }

  field_ref<IntAssignable> int_assign() {
    return {int_assign_, __isset.int_assign};
  }

  optional_field_ref<IntAssignable> opt_int_assign() {
    return {int_assign_, __isset.int_assign};
  }

  optional_field_ref<StringConstructible> scons() {
    return {scons_, __isset.scons};
  }

  optional_field_ref<std::shared_ptr<int>> ptr_ref() {
    return {ptr_, __isset.ptr};
  }

 private:
  std::string name_ = "default";
  IntAssignable int_assign_;
  StringConstructible scons_;
  std::shared_ptr<int> ptr_;

  struct __isset {
    bool name;
    bool int_assign;
    bool scons;
    bool ptr;
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

template <template <typename> class FieldRef>
void check_is_assignable() {
  using IntAssignableRef = FieldRef<IntAssignable>;
  static_assert(std::is_assignable<IntAssignableRef, int>::value, "");
  static_assert(!std::is_assignable<IntAssignableRef, std::string>::value, "");
  static_assert(std::is_nothrow_assignable<IntAssignableRef, int>::value, "");

  using StringAssignableRef = FieldRef<StringAssignable>;
  static_assert(
      !std::is_nothrow_assignable<StringAssignableRef, int>::value, "");
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
  field_ref<std::string> name = s.name();
  field_ref<const std::string> const_name = name;
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
  field_ref<const std::string> name = s.name();
  EXPECT_EQ(*name, "bar");
  EXPECT_EQ(name.value(), "bar");
  EXPECT_EQ(name->size(), 3);
  static_assert(is_const_ref<decltype(*name)>(), "");
  static_assert(is_const_ref<decltype(name.value())>(), "");
  static_assert(is_const_ref<decltype(*name.operator->())>(), "");
}

TEST(field_ref_test, mutable_accessors) {
  TestStruct s;
  field_ref<std::string> name = s.name();
  *name = "foo";
  EXPECT_EQ(*name, "foo");
  name.value() = "bar";
  EXPECT_EQ(*name, "bar");
  name->assign("baz");
  EXPECT_EQ(*name, "baz");

  // Field is not marked as set but that's OK for unqualified field.
  EXPECT_FALSE(name.is_set());
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
  optional_field_ref<std::string> name = s.opt_name();
  optional_field_ref<const std::string> const_name = name;
  EXPECT_TRUE(const_name.has_value());
  EXPECT_EQ(*const_name, "foo");
}

TEST(optional_field_ref_test, const_accessors) {
  TestStruct s;
  s.opt_name() = "bar";
  optional_field_ref<const std::string> name = s.opt_name();
  EXPECT_EQ(*name, "bar");
  EXPECT_EQ(name.value(), "bar");
  EXPECT_EQ(name->size(), 3);
  static_assert(is_const_ref<decltype(*name)>(), "");
  static_assert(is_const_ref<decltype(name.value())>(), "");
  static_assert(is_const_ref<decltype(*name.operator->())>(), "");
}

TEST(optional_field_ref_test, mutable_accessors) {
  TestStruct s;
  s.opt_name() = "initial";
  optional_field_ref<std::string> name = s.opt_name();
  *name = "foo";
  EXPECT_EQ(*name, "foo");
  name.value() = "bar";
  EXPECT_EQ(*name, "bar");
  name->assign("baz");
  EXPECT_EQ(*name, "baz");
  EXPECT_TRUE(name.has_value());
}

TEST(optional_field_ref_test, bad_field_access) {
  TestStruct s;
  optional_field_ref<std::string> name = s.opt_name();
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
