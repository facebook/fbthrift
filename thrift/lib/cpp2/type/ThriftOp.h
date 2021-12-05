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

// Operations supported by all ThriftType values.
#pragma once

#include <algorithm>

#include <folly/lang/Bits.h>
#include <thrift/lib/cpp2/hash/StdHasher.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/cpp2/type/Traits.h>
#include <thrift/lib/cpp2/type/detail/ThriftOp.h>

namespace apache::thrift::op {

// A binary operator that returns true iff the given Thrift values are equal to
// each other.
//
// For example:
//   equal<i32_t>(1, 2) -> false
//   equal<double_t>(0.0, -0.0) -> true
//   equal<float_t>(NaN, NaN) -> false
//   equal<list<double_t>>([NaN, 0.0], [NaN, -0.0]) -> false
template <typename Tag>
struct EqualTo : detail::EqualTo<Tag> {};
template <typename Tag>
constexpr EqualTo<Tag> equal;

// A binary operator that returns true iff the given Thrift values are identical
// to each other (i.e. they are same representations).
//
// For example:
//   identical<i32_t>(1, 2) -> false
//   identical<double_t>(0.0, -0.0) -> false
//   identical<float_t>(NaN, NaN) -> true
//   identical<list<double_t>>([NaN, 0.0], [NaN, -0.0]) -> false
template <typename Tag>
struct IdenticalTo : detail::IdenticalTo<Tag> {};
template <typename Tag>
constexpr IdenticalTo<Tag> identical;

// Returns the 'intrinsic' default for the given type.
//
// Ignores all 'custom' defaults set on fields within structured types.
//
// For example:
//   getDefault<type::i32_t>() -> 0
//   getDefault<type::set<type::i32_t>>() -> {}
//   getDefault<type::string_t>() -> ""
template <typename Tag, typename T = type::standard_type<Tag>>
constexpr decltype(auto) getIntrinsicDefault() {
  return detail::DefaultOf<Tag, T>::get();
}

// Returns true iff the given value is 'empty', and is not serialized in a
// 'terse' context.
//
// Some types cannot represent an 'empty' value. For example,
// a non-terse, non-optional field always serializes to a non-empty buffer,
// thus any struct with such a field, can never be 'empty'.
// In such cases, it is possible to deserialize from an empty buffer, but it is
// impossible to serialize to an empty buffer.
//
// For example:
//   isEmpty<i32_t>(0) -> true
//   isEmpty<i64_t>(1) -> false
//   isEmpty<set<i32_t>>({}) -> true
//   isEmpty<set<i32_t>>({0}) -> false
template <typename Tag>
constexpr detail::Empty<Tag> isEmpty;

// Clears the given value, leaving it equal to its intrinsic default.
// For example:
//   clear<i32_t>(myInt) // sets myInt = 0.
//   clear<set<i32_t>>(myIntSet) // calls myIntSet.clear()
template <typename Tag>
constexpr detail::Clear<Tag> clear;

namespace detail {
struct StdHasherGenerator {
  hash::StdHasher operator()() const { return {}; }
};
} // namespace detail

// Hash the given value. Same hash result will be produced for thrift values
// that are identical to, or equal to each other. Default hash algorithm is
// folly::hash_combine.
//
// For example:
//   hash<i32_t>(myInt) // returns hash of myInt.
//   hash<set<i32_t>>(myIntSet) // returns hash of myIntSet
template <
    typename Tag,
    typename HashAccumulator =
        hash::DeterministicAccumulator<detail::StdHasherGenerator>>
constexpr detail::Hash<Tag, HashAccumulator> hash;

} // namespace apache::thrift::op
