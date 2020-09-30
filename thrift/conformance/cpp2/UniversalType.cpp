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

#include <string>

#include <fmt/core.h>
#include <folly/Range.h>
#include <folly/String.h>
#include <folly/lang/Exception.h>
#include <folly/small_vector.h>

namespace apache::thrift::conformance {
namespace {

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

} // namespace

// TODO(afuller): Consider 'normalizing' a folly::Uri instead of
// requiring the name be expressed in a restricted cononical form.
void validateUniversalType(std::string_view name) {
  // We require a minimum 1 domain and 2 path segements, though up to 4 path
  // segements is likely to be common.
  folly::small_vector<folly::StringPiece, 5> segs;
  folly::splitTo<folly::StringPiece>('/', name, std::back_inserter(segs));
  check(segs.size() >= 3, "not enough path segments");
  checkDomain(segs[0]);
  size_t i = 1;
  for (; i < segs.size() - 1; ++i) {
    checkPathSegment(segs[i]);
  }
  checkTypeSegment(segs[i]);
}

} // namespace apache::thrift::conformance
