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

#include <thrift/common/detail/string.h>

#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>
#include <gtest/gtest.h>

namespace apache::thrift::detail {
namespace {

std::string replaced(
    std::string str, std::string_view from, std::string_view to) {
  replace_all(str, from, to);
  return str;
}

std::string lowered(std::string str) {
  to_lower_ascii(str);
  return str;
}

std::string uppered(std::string str) {
  to_upper_ascii(str);
  return str;
}

std::vector<std::string> split_on(
    std::string_view str, std::string_view chars) {
  std::vector<std::string> parts;
  split_if(parts, str, is_any_of(chars));
  return parts;
}

TEST(StringTest, replace_all_every_occurrence) {
  EXPECT_EQ(replaced("a.b.c", ".", "/"), "a/b/c");
}

TEST(StringTest, replace_all_replacement_longer_than_match) {
  // tree_printer swaps single-byte separators for multi-byte box characters.
  EXPECT_EQ(replaced("+-+-", "+", "├"), "├-├-");
}

TEST(StringTest, replace_all_replacement_shorter_than_match) {
  EXPECT_EQ(replaced("a::b::c", "::", "_"), "a_b_c");
}

TEST(StringTest, replace_all_does_not_rescan_replacement) {
  // The text just written is never re-examined, so overlapping matches are
  // consumed left to right and a replacement containing the needle is left
  // alone. boost::algorithm::replace_all behaved the same way.
  EXPECT_EQ(replaced("aaaa", "aa", "a"), "aa");
  EXPECT_EQ(replaced("ab", "a", "aa"), "aab");
}

TEST(StringTest, replace_all_collapses_runs_without_rescanning) {
  // A 2-character needle and a 1-character replacement is the narrow-string
  // analogue of format_abs_path collapsing "\\\\" to "\\", which relies on
  // this: matches are consumed left to right and the text just written is
  // never re-examined, so a run of n characters leaves n / 2 replacements
  // followed by the odd character. boost::algorithm::replace_all did the same.
  EXPECT_EQ(replaced("aaa", "aa", "a"), "aa");
  EXPECT_EQ(replaced("aaaa", "aa", "a"), "aa");
  EXPECT_EQ(replaced("aaaaa", "aa", "a"), "aaa");
}

TEST(StringTest, replace_all_without_a_match_is_unchanged) {
  EXPECT_EQ(replaced("abc", "z", "y"), "abc");
}

TEST(StringTest, replace_all_empty_needle_is_a_noop) {
  EXPECT_EQ(replaced("abc", "", "y"), "abc");
}

TEST(StringTest, replace_all_empty_input) {
  EXPECT_EQ(replaced("", "a", "b"), "");
}

TEST(StringTest, replace_all_copy_leaves_the_input_alone) {
  const std::string input = "x::y";
  EXPECT_EQ(replace_all_copy(input, "::", "_"), "x_y");
  EXPECT_EQ(input, "x::y");
}

TEST(StringTest, to_lower_ascii_and_to_upper_ascii_round_trip) {
  EXPECT_EQ(
      lowered("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), "abcdefghijklmnopqrstuvwxyz");
  EXPECT_EQ(
      uppered("abcdefghijklmnopqrstuvwxyz"), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
  EXPECT_EQ(
      lowered("abcdefghijklmnopqrstuvwxyz"), "abcdefghijklmnopqrstuvwxyz");
  EXPECT_EQ(
      uppered("ABCDEFGHIJKLMNOPQRSTUVWXYZ"), "ABCDEFGHIJKLMNOPQRSTUVWXYZ");
}

TEST(StringTest, to_lower_ascii_and_to_upper_ascii_leave_non_letters_alone) {
  // '@' and '[' bracket 'A'-'Z', '`' and '{' bracket 'a'-'z', so they catch an
  // off-by-one in the range test.
  const std::string non_letters = "0123456789 @[`{_-.:/\t\n";
  EXPECT_EQ(lowered(non_letters), non_letters);
  EXPECT_EQ(uppered(non_letters), non_letters);
}

TEST(StringTest, to_lower_ascii_and_to_upper_ascii_are_empty_safe) {
  EXPECT_EQ(lowered(""), "");
  EXPECT_EQ(uppered(""), "");
}

TEST(StringTest, to_lower_ascii_and_to_upper_ascii_ignore_non_ascii_bytes) {
  // The whole point of the hand-rolled ASCII loop over std::tolower: bytes
  // with the high bit set are part of a multi-byte encoding, and mapping them
  // through the current locale would corrupt them. "É" is UTF-8 0xC3 0x89 and
  // must survive both directions untouched.
  const std::string non_ascii = "\xc3\x89t\xc3\xa9";
  EXPECT_EQ(lowered(non_ascii), "\xc3\x89t\xc3\xa9");
  EXPECT_EQ(uppered(non_ascii), "\xc3\x89T\xc3\xa9");
}

TEST(StringTest, iequals_ignores_ascii_case) {
  EXPECT_TRUE(iequals("Transfer-Encoding", "transfer-encoding"));
  EXPECT_TRUE(iequals("CHUNKED", "chunked"));
  EXPECT_TRUE(iequals("fbthrift", "FBThrift"));
  EXPECT_FALSE(iequals("chunked", "close"));
}

TEST(StringTest, iequals_requires_equal_lengths) {
  EXPECT_FALSE(iequals("chunk", "chunked"));
  EXPECT_FALSE(iequals("chunked", "chunk"));
}

TEST(StringTest, iequals_compares_non_letters_exactly) {
  EXPECT_TRUE(iequals("a_1-b", "A_1-B"));
  // '_' (0x5f) and '?' (0x3f) differ only in the 0x20 bit that separates
  // 'A'-'Z' from 'a'-'z', so a blanket bit mask would wrongly match them.
  EXPECT_FALSE(iequals("_", "?"));
  EXPECT_FALSE(iequals("[", "{"));
}

TEST(StringTest, iequals_on_empty_strings) {
  EXPECT_TRUE(iequals("", ""));
  EXPECT_FALSE(iequals("", "a"));
  EXPECT_FALSE(iequals("a", ""));
}

TEST(StringTest, iequals_compares_non_ascii_bytes_bytewise) {
  EXPECT_TRUE(iequals("\xc3\x89", "\xc3\x89"));
  // 0xC3 0x89 is "É" and 0xC3 0xA9 is "é": case folding them is a locale
  // decision this helper deliberately does not make.
  EXPECT_FALSE(iequals("\xc3\x89", "\xc3\xa9"));
}

TEST(StringTest, istarts_with_ignores_ascii_case) {
  EXPECT_TRUE(istarts_with("FBThriftIsReserved", "fbthrift"));
  EXPECT_TRUE(istarts_with("fbthrift", "fbthrift"));
  EXPECT_FALSE(istarts_with("NotFbthrift", "fbthrift"));
}

TEST(StringTest, istarts_with_rejects_a_prefix_longer_than_the_string) {
  EXPECT_FALSE(istarts_with("fbthrif", "fbthrift"));
}

TEST(StringTest, istarts_with_on_empty_strings) {
  EXPECT_TRUE(istarts_with("", ""));
  EXPECT_TRUE(istarts_with("abc", ""));
  EXPECT_FALSE(istarts_with("", "a"));
}

TEST(StringTest, split_if_splits_on_a_single_delimiter) {
  EXPECT_EQ(split_on("a.b.c", "."), (std::vector<std::string>{"a", "b", "c"}));
}

TEST(StringTest, split_if_splits_on_any_delimiter_in_the_set) {
  EXPECT_EQ(
      split_on("a/b.c", "\\/."), (std::vector<std::string>{"a", "b", "c"}));
}

TEST(StringTest, split_if_keeps_empty_tokens) {
  // Callers index into the result positionally, so runs of delimiters must
  // not collapse. boost::algorithm::split kept them too.
  EXPECT_EQ(split_on("a..b", "."), (std::vector<std::string>{"a", "", "b"}));
  EXPECT_EQ(split_on(".x.", "."), (std::vector<std::string>{"", "x", ""}));
}

TEST(StringTest, split_if_empty_input_yields_one_empty_token) {
  EXPECT_EQ(split_on("", "."), (std::vector<std::string>{""}));
}

TEST(StringTest, split_if_without_a_delimiter_yields_the_whole_input) {
  EXPECT_EQ(split_on("abc", "."), (std::vector<std::string>{"abc"}));
}

TEST(StringTest, split_if_accepts_an_arbitrary_predicate) {
  std::vector<std::string> parts;
  split_if(parts, "a1b2c", [](char c) { return c >= '0' && c <= '9'; });
  EXPECT_EQ(parts, (std::vector<std::string>{"a", "b", "c"}));
}

TEST(StringTest, split_if_replaces_previous_contents) {
  std::vector<std::string> parts{"stale"};
  split_if(parts, "a.b", is_any_of("."));
  EXPECT_EQ(parts, (std::vector<std::string>{"a", "b"}));
}

template <typename T, typename = void>
constexpr bool is_any_of_accepts = false;
template <typename T>
constexpr bool
    is_any_of_accepts<T, std::void_t<decltype(is_any_of(std::declval<T>()))>> =
        true;

// is_any_of only views its argument, so an owning temporary would leave the
// returned predicate dangling. Rejecting it is a compile error, not a runtime
// one, so assert it here rather than in a TEST body.
static_assert(is_any_of_accepts<std::string_view>);
static_assert(is_any_of_accepts<const char (&)[3]>);
static_assert(is_any_of_accepts<const std::string&>);
static_assert(!is_any_of_accepts<std::string>);
static_assert(!is_any_of_accepts<const std::string>);

} // namespace
} // namespace apache::thrift::detail
