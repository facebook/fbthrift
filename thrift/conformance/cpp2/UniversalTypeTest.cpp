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

#include <thrift/conformance/cpp2/UniversalType.h>

#include <regex>
#include <string>

#include <openssl/evp.h>

#include <fmt/core.h>
#include <folly/Demangle.h>
#include <folly/FBString.h>
#include <folly/String.h>
#include <folly/portability/GTest.h>
#include <thrift/conformance/if/gen-cpp2/thrift_type_info_constants.h>

namespace apache::thrift::conformance {
namespace {

template <typename LHS, typename RHS>
struct IsSame;
template <typename T>
struct IsSame<T, T> {};

constexpr auto kGoodNames = {
    "foo.com/my/type",
    "foo.com/my/Type",
    "foo.com/my/other-type",
    "foo.com/my/other_type",
    "foo.com/m_y/type",
    "foo.com/m-y/type",
    "foo-bar.com/my/type",
    "foo.com/my/type/type",
    "1.2/3/4",
};

auto kBadNames = {
    "my",
    "my/type",
    "foo/my/type",
    "foo.com/my",
    "Foo.com/my/type",
    "foo.Com/my/type",
    "foo.com/My/type",
    "foo.com:42/my/type",
    "foo%20.com/my/type",
    "foo.com/m%20y/type",
    "foo.com/my/ty%20pe",
    "@foo.com/my/type",
    ":@foo.com/my/type",
    ":foo.com/my/type",
    "user@foo.com/my/type",
    "user:pass@foo.com/my/type",
    "fbthrift://foo.com/my/type",
    ".com/my/type",
    "foo./my/type",
    "./my/type",
    "/my/type/type",
    "foo.com//type",
    "foo.com/my/",
    "foo.com/my//type",
    "foo.com/my/type?",
    "foo.com/my/type?a=",
    "foo.com/my/type?a=b",
    "foo.com/my/type#",
    "foo.com/my/type#1",
    "foo.com/m#y/type",
    "foo.com/my/type@",
    "foo.com/my/type@1",
    "foo.com/m@y/type",
    "foo.com/my/ty@pe/type",
    "foo_bar.com/my/type",
};

constexpr auto kGoodIdSizes = {16, 24, 32};

const auto kBadIdSizes = {1, 15, 33};

auto Hash() {
  return EVP_sha256();
}

TEST(UniversalTypeTest, Constants) {
  IsSame<
      type_id_size_t,
      decltype(thrift_type_info_constants::minTypeIdBytes())>();
  IsSame<
      type_id_size_t,
      decltype(thrift_type_info_constants::maxTypeIdBytes())>();
  EXPECT_EQ(kMinTypeIdBytes, thrift_type_info_constants::minTypeIdBytes());
  EXPECT_EQ(kMaxTypeIdBytes, thrift_type_info_constants::maxTypeIdBytes());

  auto* ctx = EVP_MD_CTX_new();
  ASSERT_NE(ctx, nullptr);
  EXPECT_NE(EVP_DigestInit_ex(ctx, Hash(), nullptr), 0);
  EXPECT_EQ(kMaxTypeIdBytes, EVP_MD_CTX_size(ctx));
}

TEST(UniversalTypeTest, ValidateUniversalType) {
  std::regex pattern(fmt::format(
      "{0}(\\.{0})+\\/{1}(\\/{1})*(\\/{2})",
      "[a-z0-9-]+",
      "[a-z0-9_-]+",
      "[a-zA-Z0-9_-]+"));
  for (const auto& good : kGoodNames) {
    SCOPED_TRACE(good);
    EXPECT_TRUE(std::regex_match(good, pattern));
    validateUniversalType(good);
  }

  for (const auto& bad : kBadNames) {
    SCOPED_TRACE(bad);
    EXPECT_FALSE(std::regex_match(bad, pattern));
    EXPECT_THROW(validateUniversalType(bad), std::invalid_argument);
  }
}

TEST(UniversalTypeTest, ValidateTypeId) {
  EXPECT_THROW(validateTypeId(""), std::invalid_argument);
  for (const auto& bad : kBadIdSizes) {
    SCOPED_TRACE(bad);
    EXPECT_THROW(
        validateTypeId(folly::fbstring(bad, 'a')), std::invalid_argument);
  }

  for (const auto& good : kGoodIdSizes) {
    SCOPED_TRACE(good);
    try {
      validateTypeId(folly::fbstring(good, 'a'));
    } catch (const std::exception& ex) {
      GTEST_FAIL() << folly::demangle(typeid(ex)) << ": " << ex.what();
    }
  }
}

TEST(UniversalTypeTest, ValidateTestIdBytes) {
  validateTypeIdBytes(kDisableTypeId);
  for (const auto& bad : kBadIdSizes) {
    SCOPED_TRACE(bad);
    EXPECT_THROW(validateTypeIdBytes(bad), std::invalid_argument);
  }

  for (const auto& good : kGoodIdSizes) {
    SCOPED_TRACE(good);
    validateTypeIdBytes(good);
  }
}

TEST(UniversalTypeTest, TypeId) {
  // The digest should include the fbthrift:// prefix.
  std::string expected(EVP_MAX_MD_SIZE, 0);
  constexpr std::string_view message = "fbthrift://foo.com/my/type";
  uint32_t size;
  EXPECT_TRUE(EVP_Digest(
      message.data(),
      message.size(),
      reinterpret_cast<uint8_t*>(expected.data()),
      &size,
      Hash(),
      nullptr));
  expected.resize(size);
  EXPECT_EQ(getTypeId("foo.com/my/type"), expected);

  // Make sure the values haven't changed unintentionally.
  EXPECT_EQ(
      expected,
      "\tat$\234\357\255\265\352\rE;\3133\255Tv\001\373\376\304\262\327\225\222N\353g\324[\346F")
      << folly::cEscape<std::string>(expected);
  // Make sure the values differ.
  EXPECT_NE(getTypeId("facebook.com/thrift/Object"), expected);
}

TEST(UniversalTypeTest, MatchesTypeId) {
  EXPECT_FALSE(matchesTypeId("0123456789ABCDEF0123456789ABCDEF", ""));
  EXPECT_FALSE(matchesTypeId("0123456789ABCDEF0123456789ABCDEF", "1"));
  EXPECT_TRUE(matchesTypeId("0123456789ABCDEF0123456789ABCDEF", "0"));
  EXPECT_TRUE(
      matchesTypeId("0123456789ABCDEF0123456789ABCDEF", "0123456789ABCDEF"));
  EXPECT_TRUE(matchesTypeId(
      "0123456789ABCDEF0123456789ABCDEF", "0123456789ABCDEF0123456789ABCDEF"));
  EXPECT_FALSE(matchesTypeId(
      "0123456789ABCDEF0123456789ABCDEF", "0123456789ABCDEF0123456789ABCDEF0"));
}

TEST(UniversalTypeTest, ClampTypeIdBytes) {
  std::map<type_id_size_t, type_id_size_t> testCases = {
      {kDisableTypeId, kDisableTypeId},
      {8, 16},
      {15, 16},
      {16, 16},
      {17, 17},
      {24, 24},
      {31, 31},
      {32, 32},
      {33, 32},
  };
  for (auto [idBytes, expected] : testCases) {
    EXPECT_EQ(clampTypeIdBytes(idBytes), expected) << idBytes;
  }
}

TEST(UniversalTypeTest, GetParitalId) {
  folly::fbstring fullId(kMaxTypeIdBytes, 'a');
  EXPECT_EQ(getPartialTypeId(fullId, 0), "");

  for (size_t i = 1; i < fullId.size(); ++i) {
    EXPECT_EQ(getPartialTypeId(fullId, i), fullId.substr(0, i)) << i;
  }
  EXPECT_EQ(getPartialTypeId(fullId, fullId.size()), fullId);
  EXPECT_EQ(getPartialTypeId(fullId, fullId.size() + 1), fullId);
  EXPECT_EQ(
      getPartialTypeId(fullId, std::numeric_limits<type_id_size_t>::max()),
      fullId);
}

TEST(UniversalTypeTest, MaybeGetTypeId) {
  std::string name(24, 'a');
  EXPECT_EQ(maybeGetTypeId(name, kDisableTypeId).size(), 0);
  EXPECT_EQ(maybeGetTypeId(name, 8).size(), 8);
  EXPECT_EQ(maybeGetTypeId(name, 16).size(), 16);
  EXPECT_EQ(maybeGetTypeId(name, 23).size(), 23);
  EXPECT_EQ(maybeGetTypeId(name, 24).size(), 0);
  EXPECT_EQ(maybeGetTypeId(name, 32).size(), 0);

  name += name;
  EXPECT_EQ(maybeGetTypeId(name, 32).size(), 32);
  EXPECT_EQ(maybeGetTypeId(name, 33).size(), 32);
}

TEST(UniversalTypeTest, FindByTypeId) {
  std::map<std::string, int> typeIds = {
      {"1233", 0},
      {"1234", 1},
      {"12345", 2},
      {"1235", 3},
  };

  EXPECT_TRUE(containsTypeId(typeIds, "1"));
  EXPECT_THROW(findByTypeId(typeIds, "1"), std::runtime_error);
  EXPECT_TRUE(containsTypeId(typeIds, "12"));
  EXPECT_THROW(findByTypeId(typeIds, "12"), std::runtime_error);
  EXPECT_TRUE(containsTypeId(typeIds, "123"));
  EXPECT_THROW(findByTypeId(typeIds, "123"), std::runtime_error);
  EXPECT_TRUE(containsTypeId(typeIds, "1234"));
  EXPECT_THROW(findByTypeId(typeIds, "1234"), std::runtime_error);

  EXPECT_FALSE(containsTypeId(typeIds, ""));
  EXPECT_EQ(findByTypeId(typeIds, ""), typeIds.end());

  EXPECT_FALSE(containsTypeId(typeIds, "0"));
  EXPECT_EQ(findByTypeId(typeIds, "0"), typeIds.end());

  EXPECT_TRUE(containsTypeId(typeIds, "1233"));
  EXPECT_EQ(findByTypeId(typeIds, "1233")->second, 0);

  EXPECT_FALSE(containsTypeId(typeIds, "12333"));
  EXPECT_EQ(findByTypeId(typeIds, "12333"), typeIds.end());

  EXPECT_TRUE(containsTypeId(typeIds, "12345"));
  EXPECT_EQ(findByTypeId(typeIds, "12345")->second, 2);

  EXPECT_FALSE(containsTypeId(typeIds, "12346"));
  EXPECT_EQ(findByTypeId(typeIds, "12346"), typeIds.end());

  EXPECT_TRUE(containsTypeId(typeIds, "1235"));
  EXPECT_EQ(findByTypeId(typeIds, "1235")->second, 3);

  EXPECT_FALSE(containsTypeId(typeIds, "2"));
  EXPECT_EQ(findByTypeId(typeIds, "2"), typeIds.end());
}

} // namespace
} // namespace apache::thrift::conformance
