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

#include <algorithm>
#include <limits>
#include <string>

#include <openssl/evp.h>

#include <fmt/core.h>
#include <folly/Range.h>
#include <folly/String.h>
#include <folly/lang/Exception.h>
#include <folly/small_vector.h>

namespace apache::thrift::conformance {
namespace {

const std::string_view kThriftScheme = "fbthrift://";

struct MdCtxDeleter {
  void operator()(EVP_MD_CTX* ctx) const {
    EVP_MD_CTX_free(ctx);
  }
};
using ctx_ptr = std::unique_ptr<EVP_MD_CTX, MdCtxDeleter>;

ctx_ptr newMdContext() {
  auto* ctx = EVP_MD_CTX_new();
  if (ctx == nullptr) {
    folly::throw_exception<std::runtime_error>("could not create ctx");
  }
  return ctx_ptr(ctx);
}

void checkResult(int evp_result) {
  if (evp_result == 0) {
    folly::throw_exception<std::runtime_error>("EVP failure");
  }
}
void check(bool cond, const char* err) {
  if (!cond) {
    folly::throw_exception<std::invalid_argument>(err);
  }
}

bool isDomainChar(char c) {
  return std::isdigit(c) || std::islower(c) || c == '-';
}

bool isPathChar(char c) {
  return isDomainChar(c) || c == '_';
}

bool isTypeChar(char c) {
  return isPathChar(c) || std::isupper(c);
}

void checkDomainSegment(folly::StringPiece seg) {
  check(!seg.empty(), "empty domain segment");
  for (const auto& c : seg) {
    check(isDomainChar(c), "invalid domain char");
  }
}

void checkPathSegment(folly::StringPiece seg) {
  check(!seg.empty(), "empty path segment");
  for (const auto& c : seg) {
    check(isPathChar(c), "invalid path char");
  }
}

void checkTypeSegment(folly::StringPiece seg) {
  check(!seg.empty(), "empty type segment");
  for (const auto& c : seg) {
    check(isTypeChar(c), "invalid type char");
  }
}

void checkDomain(folly::StringPiece domain) {
  // We require a minimum of 2 domain segments, but up to 4 is likely to be
  // common.
  folly::small_vector<folly::StringPiece, 4> segs;
  folly::splitTo<folly::StringPiece>('.', domain, std::back_inserter(segs));
  check(segs.size() >= 2, "not enough domain segments");
  for (const auto& seg : segs) {
    checkDomainSegment(seg);
  }
}

folly::fbstring TypeHashSha2_256(std::string_view uri) {
  // Save an initalized context.
  static EVP_MD_CTX* kBase = []() {
    auto ctx = newMdContext();
    checkResult(EVP_DigestInit_ex(ctx.get(), EVP_sha256(), nullptr));
    checkResult(EVP_DigestUpdate(
        ctx.get(), kThriftScheme.data(), kThriftScheme.size()));
    return ctx.release(); // Leaky singleton.
  }();

  // Copy the base context.
  auto ctx = newMdContext();
  checkResult(EVP_MD_CTX_copy_ex(ctx.get(), kBase));
  // Digest the uri.
  checkResult(EVP_DigestUpdate(ctx.get(), uri.data(), uri.size()));

  // Get the result.
  folly::fbstring result(EVP_MD_CTX_size(ctx.get()), 0);
  uint32_t size;
  checkResult(EVP_DigestFinal_ex(
      ctx.get(), reinterpret_cast<uint8_t*>(result.data()), &size));
  assert(size == result.size()); // Should already be the correct size.
  result.resize(size);
  return result;
}

type_hash_size_t TypeHashSizeSha2_256() {
  return EVP_MD_size(EVP_sha256());
}

} // namespace

// TODO(afuller): Consider 'normalizing' a folly::Uri instead of
// requiring the uri be expressed in a restricted cononical form.
void validateUniversalType(std::string_view uri) {
  // We require a minimum 1 domain and 2 path segements, though up to 4 path
  // segements is likely to be common.
  folly::small_vector<folly::StringPiece, 5> segs;
  folly::splitTo<folly::StringPiece>('/', uri, std::back_inserter(segs));
  check(segs.size() >= 3, "not enough path segments");
  checkDomain(segs[0]);
  size_t i = 1;
  for (; i < segs.size() - 1; ++i) {
    checkPathSegment(segs[i]);
  }
  checkTypeSegment(segs[i]);
}

folly::fbstring getTypeHash(TypeHashAlgorithm alg, std::string_view uri) {
  switch (alg) {
    case TypeHashAlgorithm::Sha2_256:
      return TypeHashSha2_256(uri);
    default:
      folly::throw_exception<std::runtime_error>(
          "Unsupported type hash algorithm: " + std::to_string((int)alg));
  }
}

type_hash_size_t getTypeHashSize(TypeHashAlgorithm alg) {
  switch (alg) {
    case TypeHashAlgorithm::Sha2_256:
      return TypeHashSizeSha2_256();
    default:
      folly::throw_exception<std::runtime_error>(
          "Unsupported type hash algorithm: " + std::to_string((int)alg));
  }
}

void validateTypeHash(TypeHashAlgorithm alg, folly::StringPiece typeHash) {
  auto maxBytes = getTypeHashSize(alg);
  if (typeHash.size() > std::numeric_limits<type_hash_size_t>::max()) {
    folly::throw_exception<std::invalid_argument>(fmt::format(
        "Type hash size must be <= {}, was {}.", maxBytes, typeHash.size()));
  }
  auto typeHashBytes = type_hash_size_t(typeHash.size());
  if (typeHashBytes < kMinTypeHashBytes || typeHashBytes > maxBytes) {
    folly::throw_exception<std::invalid_argument>(fmt::format(
        "Type hash size must be in the range [{}, {}], was {}.",
        kMinTypeHashBytes,
        maxBytes,
        typeHashBytes));
  }
}

void validateTypeHashBytes(type_hash_size_t typeHashBytes) {
  if (typeHashBytes == kDisableTypeHash) {
    return;
  }
  if (typeHashBytes < kMinTypeHashBytes) {
    folly::throw_exception<std::invalid_argument>(fmt::format(
        "Type hash size must be >= {}, was {}.",
        kMinTypeHashBytes,
        typeHashBytes));
  }
}

bool matchesTypeHash(folly::StringPiece typeHash, folly::StringPiece prefix) {
  if (typeHash.size() < prefix.size() || prefix.empty()) {
    return false;
  }
  return typeHash.subpiece(0, prefix.size()) == prefix;
}

folly::StringPiece getTypeHashPrefix(
    folly::StringPiece typeHash,
    type_hash_size_t typeHashBytes) {
  return typeHash.subpiece(0, typeHashBytes);
}

folly::fbstring maybeGetTypeHashPrefix(
    TypeHashAlgorithm alg,
    std::string_view uri,
    type_hash_size_t typeHashBytes) {
  if (typeHashBytes == kDisableTypeHash || // Type hash disabled.
      uri.size() <= size_t(typeHashBytes)) { // Type uri is smaller.
    return {};
  }
  folly::fbstring result = getTypeHash(alg, uri);
  if (result.size() > size_t(typeHashBytes)) {
    result.resize(typeHashBytes);
  }
  return result;
}

} // namespace apache::thrift::conformance
