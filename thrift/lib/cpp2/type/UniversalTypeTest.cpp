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

#include <thrift/lib/cpp2/type/UniversalType.h>

#include <regex>
#include <string>

#include <openssl/evp.h>

#include <fmt/core.h>
#include <folly/Demangle.h>
#include <folly/FBString.h>
#include <folly/String.h>
#include <folly/portability/GTest.h>
#include <thrift/conformance/if/gen-cpp2/type_constants.h>

namespace apache::thrift::type {
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

constexpr auto kGoodIdSizes = {8, 16, 24, 32};

const auto kBadIdSizes = {1, 7, 33, 255};

auto Hash() {
  return EVP_sha256();
}

TEST(UniversalTypeTest, Constants) {
  IsSame<type_hash_size_t, decltype(type_constants::minTypeHashBytes())>();
  EXPECT_EQ(kMinTypeHashBytes, type_constants::minTypeHashBytes());

  auto* ctx = EVP_MD_CTX_new();
  ASSERT_NE(ctx, nullptr);
  EXPECT_NE(EVP_DigestInit_ex(ctx, Hash(), nullptr), 0);
  EXPECT_EQ(getTypeHashSize(TypeHashAlgorithm::Sha2_256), EVP_MD_CTX_size(ctx));
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

TEST(UniversalTypeTest, ValidateTypeHash) {
  EXPECT_THROW(
      validateTypeHash(TypeHashAlgorithm::Sha2_256, ""), std::invalid_argument);
  for (const auto& bad : kBadIdSizes) {
    SCOPED_TRACE(bad);
    EXPECT_THROW(
        validateTypeHash(
            TypeHashAlgorithm::Sha2_256, folly::fbstring(bad, 'a')),
        std::invalid_argument);
  }

  for (const auto& good : kGoodIdSizes) {
    SCOPED_TRACE(good);
    try {
      validateTypeHash(TypeHashAlgorithm::Sha2_256, folly::fbstring(good, 'a'));
    } catch (const std::exception& ex) {
      GTEST_FAIL() << folly::demangle(typeid(ex)) << ": " << ex.what();
    }
  }
}

TEST(UniversalTypeTest, ValidateTypeHashBytes) {
  validateTypeHashBytes(kDisableTypeHash);
  for (const auto& bad : kBadIdSizes) {
    if (bad >= kMinTypeHashBytes) {
      continue; // Upper bounds is not checked, as it can vary.
    }
    SCOPED_TRACE(bad);
    EXPECT_THROW(validateTypeHashBytes(bad), std::invalid_argument);
  }

  for (const auto& good : kGoodIdSizes) {
    SCOPED_TRACE(good);
    validateTypeHashBytes(good);
  }
}

TEST(UniversalTypeTest, TypeHashSha2_256) {
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
  EXPECT_EQ(
      getTypeHash(TypeHashAlgorithm::Sha2_256, "foo.com/my/type"), expected);

  // Make sure the values haven't changed unintentionally.
  EXPECT_EQ(
      expected,
      "\tat$\234\357\255\265\352\rE;\3133\255Tv\001\373\376\304\262\327\225\222N\353g\324[\346F")
      << folly::cEscape<std::string>(expected);
  // Make sure the values differ.
  EXPECT_NE(
      getTypeHash(TypeHashAlgorithm::Sha2_256, "facebook.com/thrift/Object"),
      expected);
}

TEST(UniversalTypeTest, MatchesTypeHash) {
  EXPECT_FALSE(matchesTypeHash("0123456789ABCDEF0123456789ABCDEF", ""));
  EXPECT_FALSE(matchesTypeHash("0123456789ABCDEF0123456789ABCDEF", "1"));
  EXPECT_TRUE(matchesTypeHash("0123456789ABCDEF0123456789ABCDEF", "0"));
  EXPECT_TRUE(
      matchesTypeHash("0123456789ABCDEF0123456789ABCDEF", "0123456789ABCDEF"));
  EXPECT_TRUE(matchesTypeHash(
      "0123456789ABCDEF0123456789ABCDEF", "0123456789ABCDEF0123456789ABCDEF"));
  EXPECT_FALSE(matchesTypeHash(
      "0123456789ABCDEF0123456789ABCDEF", "0123456789ABCDEF0123456789ABCDEF0"));
}

TEST(UniversalTypeTest, GetTypeHashPrefix) {
  folly::fbstring fullId(32, 'a');
  EXPECT_EQ(getTypeHashPrefix(fullId, 0), "");

  for (size_t i = 1; i < fullId.size(); ++i) {
    EXPECT_EQ(getTypeHashPrefix(fullId, i), fullId.substr(0, i)) << i;
  }
  EXPECT_EQ(getTypeHashPrefix(fullId, fullId.size()), fullId);
  EXPECT_EQ(getTypeHashPrefix(fullId, fullId.size() + 1), fullId);
  EXPECT_EQ(
      getTypeHashPrefix(fullId, std::numeric_limits<type_hash_size_t>::max()),
      fullId);
}

TEST(UniversalTypeTest, MaybeGetTypeHashPrefix) {
  std::string name(24, 'a');
  EXPECT_EQ(
      maybeGetTypeHashPrefix(
          TypeHashAlgorithm::Sha2_256, name, kDisableTypeHash)
          .size(),
      0);
  EXPECT_EQ(
      maybeGetTypeHashPrefix(TypeHashAlgorithm::Sha2_256, name, 8).size(), 8);
  EXPECT_EQ(
      maybeGetTypeHashPrefix(TypeHashAlgorithm::Sha2_256, name, 16).size(), 16);
  EXPECT_EQ(
      maybeGetTypeHashPrefix(TypeHashAlgorithm::Sha2_256, name, 23).size(), 23);
  EXPECT_EQ(
      maybeGetTypeHashPrefix(TypeHashAlgorithm::Sha2_256, name, 24).size(), 0);
  EXPECT_EQ(
      maybeGetTypeHashPrefix(TypeHashAlgorithm::Sha2_256, name, 32).size(), 0);

  name += name;
  EXPECT_EQ(
      maybeGetTypeHashPrefix(TypeHashAlgorithm::Sha2_256, name, 32).size(), 32);
  EXPECT_EQ(
      maybeGetTypeHashPrefix(TypeHashAlgorithm::Sha2_256, name, 33).size(), 32);
}

TEST(UniversalTypeTest, FindByTypeHash) {
  std::map<std::string, int> typeHashs = {
      {"1233", 0},
      {"1234", 1},
      {"12345", 2},
      {"1235", 3},
  };

  EXPECT_TRUE(containsTypeHash(typeHashs, "1"));
  EXPECT_THROW(findByTypeHash(typeHashs, "1"), std::runtime_error);
  EXPECT_TRUE(containsTypeHash(typeHashs, "12"));
  EXPECT_THROW(findByTypeHash(typeHashs, "12"), std::runtime_error);
  EXPECT_TRUE(containsTypeHash(typeHashs, "123"));
  EXPECT_THROW(findByTypeHash(typeHashs, "123"), std::runtime_error);
  EXPECT_TRUE(containsTypeHash(typeHashs, "1234"));
  EXPECT_THROW(findByTypeHash(typeHashs, "1234"), std::runtime_error);

  EXPECT_FALSE(containsTypeHash(typeHashs, ""));
  EXPECT_EQ(findByTypeHash(typeHashs, ""), typeHashs.end());

  EXPECT_FALSE(containsTypeHash(typeHashs, "0"));
  EXPECT_EQ(findByTypeHash(typeHashs, "0"), typeHashs.end());

  EXPECT_TRUE(containsTypeHash(typeHashs, "1233"));
  EXPECT_EQ(findByTypeHash(typeHashs, "1233")->second, 0);

  EXPECT_FALSE(containsTypeHash(typeHashs, "12333"));
  EXPECT_EQ(findByTypeHash(typeHashs, "12333"), typeHashs.end());

  EXPECT_TRUE(containsTypeHash(typeHashs, "12345"));
  EXPECT_EQ(findByTypeHash(typeHashs, "12345")->second, 2);

  EXPECT_FALSE(containsTypeHash(typeHashs, "12346"));
  EXPECT_EQ(findByTypeHash(typeHashs, "12346"), typeHashs.end());

  EXPECT_TRUE(containsTypeHash(typeHashs, "1235"));
  EXPECT_EQ(findByTypeHash(typeHashs, "1235")->second, 3);

  EXPECT_FALSE(containsTypeHash(typeHashs, "2"));
  EXPECT_EQ(findByTypeHash(typeHashs, "2"), typeHashs.end());
}

} // namespace
} // namespace apache::thrift::type
