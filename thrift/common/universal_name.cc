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

#include <thrift/common/universal_name.h>

#include <algorithm>
#include <cctype>
#include <cstddef>
#include <stdexcept>
#include <string_view>
#include <vector>
#include <fmt/core.h>
#include <fmt/format.h>

namespace apache::thrift {
namespace {
/**
 * Trivial wrapper type around a `char`, to allow safe formatting of
 * non-printable characters (as their hexadecimal literal representation, eg.
 * '\x1c').
 */
struct printable_char final {
  explicit printable_char(char c) : c(c) {}

  char c;
};
} // namespace
} // namespace apache::thrift

/**
 * Formats a `printable_char` with the corresponding (printable or hexadecimal)
 * character literal.
 */
template <>
struct fmt::formatter<apache::thrift::printable_char>
    : fmt::formatter<std::string> {
  format_context::iterator format(
      apache::thrift::printable_char p, format_context& ctx) const {
    std::string s = std::isprint(p.c) ? fmt::format("'{}'", p.c)
                                      : fmt::format(R"('\x{:x}')", p.c);
    return formatter<std::string>::format(s, ctx);
  }
};

namespace apache::thrift {
namespace {

/**
 * Throws an `invalid_argument` exception with the given (formatted) message if
 * `cond` is `false`.
 */
template <typename... T>
void check(bool cond, fmt::format_string<T...> msg, T&&... args) {
  if (!cond) [[unlikely]] {
    throw std::invalid_argument(fmt::format(msg, std::forward<T>(args)...));
  }
}

bool is_domain_char(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'z');
}

bool is_path_char(char c) {
  return is_domain_char(c) || c == '_';
}

bool is_type_char(char c) {
  return is_path_char(c) || (c >= 'A' && c <= 'Z');
}

void check_domain_component(std::size_t i, std::string_view component) {
  check(!component.empty(), "URI domain component at index {} is empty", i);

  for (std::size_t j = 0; j < component.size(); ++j) {
    char c = component[j];
    check(
        is_domain_char(c),
        "URI domain component #{} has invalid character at position {}: {}",
        i,
        j,
        printable_char(c));
  }
}

void check_path_segment(std::size_t i, std::string_view segment) {
  check(!segment.empty(), "URI path segment at index {} is empty", i);

  for (std::size_t j = 0; j < segment.size(); ++j) {
    char c = segment[j];
    check(
        is_path_char(c),
        "URI path segment #{} has invalid character at position {}: {}",
        i,
        j,
        printable_char(c));
  }
}

template <typename TStringContainer>
void check_domain_components(const TStringContainer& domain) {
  check(
      domain.size() >= 2,
      "Not enough domain components: expected at least 2, got {}",
      domain.size());
  for (std::size_t i = 0; i < domain.size(); ++i) {
    check_domain_component(i, domain[i]);
  }
}

template <typename TStringishIterator>
void check_path_segments(TStringishIterator begin, TStringishIterator end) {
  check(begin != end, "Empty URI path");

  std::size_t i = 0;
  for (TStringishIterator it = begin; it != end; ++it, ++i) {
    check_path_segment(i, *it);
  }
}

void check_type_segment(std::string_view segment) {
  check(!segment.empty(), "Empty URI type segment");
  for (std::size_t j = 0; j < segment.size(); ++j) {
    char c = segment[j];
    check(
        is_type_char(c),
        "URI type segment has invalid character at position {}: {}",
        j,
        printable_char(c));
  }
}

/**
 * Returns how many `delimiter`-separated segments `str` has, counting empty
 * ones. Never 0: an empty string has a single, empty segment.
 */
std::size_t count_segments(std::string_view str, char delimiter) {
  return 1 +
      static_cast<std::size_t>(std::count(str.begin(), str.end(), delimiter));
}

/**
 * Calls `visit(index, segment)` for every `delimiter`-separated segment of
 * `str`, in order and including empty ones.
 */
template <typename TVisitor>
void for_each_segment(std::string_view str, char delimiter, TVisitor visit) {
  std::size_t index = 0;
  for (std::size_t start = 0;; ++index) {
    const std::size_t end = str.find(delimiter, start);
    if (end == std::string_view::npos) {
      visit(index, str.substr(start));
      return;
    }
    visit(index, str.substr(start, end - start));
    start = end + 1;
  }
}

void check_domain_components(std::string_view domain) {
  const std::size_t size = count_segments(domain, '.');
  check(
      size >= 2,
      "Not enough domain components: expected at least 2, got {}",
      size);
  for_each_segment(domain, '.', check_domain_component);
}

} // namespace

namespace detail {

void check_univeral_name_domain(const std::vector<std::string>& domain) {
  try {
    check_domain_components(domain);
  } catch (const std::invalid_argument& ex) {
    throw std::invalid_argument(
        fmt::format("Invalid Thrift URI domain ({}).", ex.what()));
  }
}

void check_universal_name_path(const std::vector<std::string>& path) {
  try {
    check_path_segments(path.begin(), path.end());
  } catch (const std::invalid_argument& ex) {
    throw std::invalid_argument(
        fmt::format("Invalid Thrift URI path ({}).", ex.what()));
  }
}

} // namespace detail

void validate_universal_name(std::string_view uri) {
  // A valid Thrift universal name (aka Thrift URI) must have at least 3
  // parts separated by "/":
  // 1. A domain (eg. "facebook.com")
  // 2. A non-empty path
  // 3. A type name
  //
  // e.g.: "facebook.com/thrift/Value"
  //
  // The URI is walked in place rather than split into a container: this runs
  // from AnyRegistry::registerType during static initialization, once per
  // registered type, so it must not allocate.
  try {
    const std::size_t parts = count_segments(uri, '/');
    check(parts >= 3, "Not enough parts: expected at least 3, got: {}", parts);

    // With at least 3 parts there are at least 2 separators, so the domain,
    // the path and the type segment are all non-empty ranges of `uri`.
    const std::size_t domain_end = uri.find('/');
    const std::size_t type_begin = uri.rfind('/') + 1;

    check_domain_components(uri.substr(0, domain_end));
    for_each_segment(
        uri.substr(domain_end + 1, type_begin - domain_end - 2),
        '/',
        check_path_segment);
    check_type_segment(uri.substr(type_begin));
  } catch (const std::invalid_argument& ex) {
    throw std::invalid_argument(
        fmt::format("Not a valid Thrift URI: \"{}\" ({})", uri, ex.what()));
  }
}

} // namespace apache::thrift
