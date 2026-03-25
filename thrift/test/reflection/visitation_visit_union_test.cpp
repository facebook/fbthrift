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

#include <any>
#include <gtest/gtest.h>
#include <folly/Utility.h>
#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/thrift/gen-cpp2/metadata_types.h>
#include <thrift/test/gen-cpp2/UnionFieldRef_types.h>

namespace apache::thrift::test {
namespace {

struct VisitUnionAdapter {
  template <class T, class F>
  void operator()(T&& t, F&& f) const {
    apache::thrift::op::visit_union_with_tag(
        std::forward<T>(t),
        [&]<typename Ident>(folly::tag_t<Ident>, auto& value) {
          using U = folly::remove_cvref_t<T>;
          f(apache::thrift::op::get_name_v<U, Ident>,
            apache::thrift::op::get_field_id_v<U, Ident>,
            value);
        },
        []() {});
  }
};

struct ForEachFieldAdapter {
  template <class T, class F>
  void operator()(T&& t, F&& f) const {
    using U = folly::remove_cvref_t<T>;
    apache::thrift::op::for_each_field_id<U>([&]<class Id>(Id) {
      if (folly::to_underlying(t.getType()) ==
          folly::to_underlying(Id::value)) {
        auto fieldRef = apache::thrift::op::get<Id>(std::forward<T>(t));
        if (auto* value = apache::thrift::op::get_value_or_null(fieldRef)) {
          f(apache::thrift::op::get_name_v<U, Id>,
            apache::thrift::op::get_field_id_v<U, Id>,
            *value);
        }
      }
    });
  }
};

template <class Adapter>
struct VisitUnionTest : ::testing::Test {
  static constexpr Adapter adapter;
};

using Adapters = ::testing::Types<VisitUnionAdapter, ForEachFieldAdapter>;
TYPED_TEST_CASE(VisitUnionTest, Adapters);

TYPED_TEST(VisitUnionTest, basic) {
  Basic a;
  TestFixture::adapter(a, [&](auto&&, auto&&, auto&&) { FAIL(); });

  static const std::string str = "foo";
  a.str() = str;
  TestFixture::adapter(a, [](auto&& name, auto&& id, auto&& v) {
    EXPECT_EQ(name, "str");
    EXPECT_EQ(folly::to_underlying(id), 2);
    if constexpr (std::is_same_v<
                      folly::remove_cvref_t<decltype(v)>,
                      std::string>) {
      EXPECT_EQ(v, str);
    } else {
      FAIL();
    }
  });

  static const int64_t int64 = 42LL << 42;
  a.int64() = int64;
  TestFixture::adapter(a, [](auto&& name, auto&& id, auto&& v) {
    EXPECT_EQ(name, "int64");
    EXPECT_EQ(folly::to_underlying(id), 1);
    EXPECT_EQ(typeid(v), typeid(int64_t));
    if constexpr (std::is_same_v<folly::remove_cvref_t<decltype(v)>, int64_t>) {
      EXPECT_EQ(v, int64);
    } else {
      FAIL();
    }
  });

  static const std::vector<int32_t> list_i32 = {3, 1, 2};
  a.list_i32() = list_i32;
  TestFixture::adapter(a, [](auto&& name, auto&& id, auto&& v) {
    EXPECT_EQ(name, "list_i32");
    EXPECT_EQ(folly::to_underlying(id), 4);
    if constexpr (std::is_same_v<
                      folly::remove_cvref_t<decltype(v)>,
                      std::vector<int32_t>>) {
      EXPECT_EQ(v, list_i32);
    } else {
      FAIL();
    }
  });
}

TYPED_TEST(VisitUnionTest, Metadata) {
  Basic a;
  a.int64() = 42;
  TestFixture::adapter(a, [](auto&& name, auto&&, auto&&) {
    // Verify the visit works and we get the correct field name
    EXPECT_EQ(name, "int64");
  });
}

struct TestPassCallableByValue {
  int i = 0;
  template <class... Args>
  void operator()(Args&&...) {
    ++i;
  }
};

TYPED_TEST(VisitUnionTest, PassCallableByReference) {
  TestPassCallableByValue f;
  Basic a;
  a.int64() = 42;
  TestFixture::adapter(a, folly::copy(f));
  EXPECT_EQ(f.i, 0);
  TestFixture::adapter(a, std::ref(f));
  EXPECT_EQ(f.i, 1);
  TestFixture::adapter(a, f);
  EXPECT_EQ(f.i, 2);
}

template <class T>
constexpr bool kIsString =
    std::is_same_v<folly::remove_cvref_t<T>, std::string>;

TEST(VisitUnionTest, CppRef) {
  CppRef r;
  CppRef r2;
  r2.str() = "42";
  r.set_cppref(r2);
  bool typeMatches = false;
  apache::thrift::op::visit_union_with_tag(
      r,
      [&]<typename Ident>(folly::tag_t<Ident>, auto& r2Value) {
        if constexpr (!kIsString<decltype(r2Value)>) {
          apache::thrift::op::visit_union_with_tag(
              r2Value,
              [&]<typename Ident2>(folly::tag_t<Ident2>, auto& v) {
                if constexpr (kIsString<decltype(v)>) {
                  typeMatches = true;
                  EXPECT_EQ(v, "42");
                }
              },
              []() {});
        }
      },
      []() {});
  EXPECT_TRUE(typeMatches);
}

TEST(VisitUnionTest, NonVoid) {
  DuplicateType r;
  r.set_str1("boo");
  auto result = apache::thrift::op::visit_union_with_tag(
      r,
      []<typename Ident>(folly::tag_t<Ident>, auto& r2) {
        if constexpr (std::is_same_v<
                          folly::remove_cvref_t<decltype(r2)>,
                          std::string>) {
          return std::move(r2);
        }
        return std::string("list");
      },
      []() { return std::string(); });
  EXPECT_EQ(result, "boo");
}

} // namespace
} // namespace apache::thrift::test
