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

#pragma once

#include <cstring>
#include <string_view>
#include <type_traits>
#include <utility>
#include <folly/Traits.h>
#include <folly/lang/Pretty.h>

namespace apache::thrift {

namespace detail {
// Forward declaration from header `thrift/lib/cpp2/gen/module_types_h.h`
// which is supposed to only be included by generated files.
template <typename Tag>
struct invoke_reffer;
} // namespace detail

namespace test::detail {

template <typename FieldTag, typename InnerMatcher>
class ThriftFieldMatcher {
 public:
  explicit ThriftFieldMatcher(InnerMatcher matcher)
      : matcher_(std::move(matcher)) {}

  template <typename ThriftStruct>
  operator testing::Matcher<ThriftStruct>() const {
    return testing::Matcher<ThriftStruct>(
        new Impl<const ThriftStruct&>(matcher_));
  }

 private:
  using Accessor = thrift::detail::invoke_reffer<FieldTag>;
  // For field_ref, we want to forward the value pointed to the matcher
  // directly (it's fully transparent).
  // For optional_field_ref, we don't want to do it becase the mental model
  // is as if it was an std::optional<field_ref>. If we were to forward the
  // value, it would be impossible to check if it is empty.
  // Instead, we send the optional_field_ref to the matcher. The user can
  // provide a testing::Optional matcher to match the value contained in it.

  template <typename ThriftStruct>
  class Impl : public testing::MatcherInterface<ThriftStruct> {
   private:
    using FieldRef = decltype(Accessor{}(std::declval<ThriftStruct>()));
    using FieldReferenceType = typename FieldRef::reference_type;
    inline static constexpr bool IsOptionalFieldRef =
        std::is_same_v<optional_field_ref<FieldReferenceType>, FieldRef>;
    // See above on why we do this switch
    using MatchedValue = std::conditional_t<
        IsOptionalFieldRef,
        optional_field_ref<FieldReferenceType>,
        FieldReferenceType>;

   public:
    template <typename MatcherOrPolymorphicMatcher>
    explicit Impl(const MatcherOrPolymorphicMatcher& matcher)
        : concrete_matcher_(testing::MatcherCast<MatchedValue>(matcher)) {}

    void DescribeTo(::std::ostream* os) const override {
      *os << "is an object whose field `" << getFieldName() << "` ";
      concrete_matcher_.DescribeTo(os);
    }

    void DescribeNegationTo(::std::ostream* os) const override {
      *os << "is an object whose field `" << getFieldName() << "` ";
      concrete_matcher_.DescribeNegationTo(os);
    }

    bool MatchAndExplain(
        ThriftStruct obj,
        testing::MatchResultListener* listener) const override {
      *listener << "whose field `" << getFieldName() << "` is ";
      auto val = Accessor{}(obj);
      // Using gtest internals??
      // This function does some printing and formatting.
      // Here we want the same behaviour as other matchers, so
      // this allows us to save code and stay consistent.
      // If in later gtest versions this breaks, we can either
      //  1. use the new functionality looking at how
      //     testing::FieldMatcher is implemented, or
      //  2. copy the old implementation here.
      using testing::internal::MatchPrintAndExplain;
      if constexpr (IsOptionalFieldRef) {
        return MatchPrintAndExplain(val, concrete_matcher_, listener);
      } else {
        return MatchPrintAndExplain(*val, concrete_matcher_, listener);
      }
    }

   private:
    std::string_view getFieldName() const {
      // Thrift generates the tags in the 'apache::thrift::tag' namespace,
      // so we can just skip it to provide a better name for the users.
      // -1 because we don't want to count the terminating \0 character.
      constexpr std::size_t offset = sizeof("apache::thrift::tag::") - 1;
      // it's safe to return the string_view because pretty_name() returns a
      // statically allocated char *
      std::string_view name{folly::pretty_name<FieldTag>()};
      if (name.size() > offset) {
        name.remove_prefix(offset);
      }
      return name;
    }

    const testing::Matcher<MatchedValue> concrete_matcher_;
  }; // class Impl

  const InnerMatcher matcher_;
};

} // namespace test::detail
} // namespace apache::thrift
