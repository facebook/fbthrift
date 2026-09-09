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

#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <type_traits>

namespace apache::thrift::detail {
/**
 * Escapes special characters in the input string view for displaying.
 * This includes new lines and other whitespace characters.
 */
std::string escape(std::string_view str);

/**
 * Replaces every occurrence of `from` in `str` with `to`. Replacements are not
 * rescanned, so replacing "aa" with "a" in "aaaa" yields "aa". Does nothing if
 * `from` is empty.
 */
void replace_all(std::string& str, std::string_view from, std::string_view to);

/**
 * Same as replace_all, but returns the result instead of modifying in place.
 */
std::string replace_all_copy(
    std::string_view str, std::string_view from, std::string_view to);

/**
 * Lowercases every ASCII letter ('A'-'Z') in `str`, in place. Every other byte
 * is left untouched, including bytes belonging to a multi-byte encoding, so the
 * result does not depend on the current locale.
 */
void to_lower_ascii(std::string& str);

/**
 * Same as to_lower_ascii, but uppercases every ASCII letter ('a'-'z') instead.
 */
void to_upper_ascii(std::string& str);

/**
 * Returns whether `a` and `b` are equal, ignoring the case of ASCII letters.
 * Every other byte is compared as-is, including bytes belonging to a multi-byte
 * encoding, so the result does not depend on the current locale.
 */
bool iequals(std::string_view a, std::string_view b);

/**
 * Returns whether `str` starts with `prefix`, ignoring the case of ASCII
 * letters. Same locale independence as iequals.
 */
bool istarts_with(std::string_view str, std::string_view prefix);

/**
 * Returns a predicate matching any single character in `chars`, for use as a
 * split_if delimiter. `chars` is captured as a view, so it must outlive the
 * returned predicate.
 */
inline auto is_any_of(std::string_view chars) {
  return [chars](char c) { return chars.find(c) != std::string_view::npos; };
}

/**
 * An owning temporary dies at the end of the full expression, leaving the
 * predicate holding a dangling view, so reject it at compile time. Deleting a
 * plain `std::string&&` overload would not do: a string literal converts to
 * both parameter types just as well, which makes every literal call site
 * ambiguous. Constraining the deleted overload to an exact match keeps it
 * limited to the temporaries that actually dangle.
 */
template <
    typename T,
    typename =
        std::enable_if_t<std::is_same_v<std::remove_cv_t<T>, std::string>>>
void is_any_of(T&&) = delete;

/**
 * Splits `str` at every character for which `is_delimiter` returns true and
 * assigns the tokens to `out`, replacing its previous contents. Empty tokens
 * are kept: splitting "" yields one empty token, and "a..b" on '.' yields
 * three.
 */
template <typename Container, typename Predicate>
void split_if(Container& out, std::string_view str, Predicate is_delimiter) {
  out.clear();
  std::size_t begin = 0;
  for (std::size_t i = 0; i < str.size(); ++i) {
    if (is_delimiter(str[i])) {
      out.emplace_back(str.substr(begin, i - begin));
      begin = i + 1;
    }
  }
  out.emplace_back(str.substr(begin));
}

} // namespace apache::thrift::detail
