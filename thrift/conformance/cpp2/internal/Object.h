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

#include <type_traits>

#include <folly/CPortability.h>
#include <thrift/conformance/cpp2/ThriftTypes.h>
#include <thrift/conformance/if/gen-cpp2/object_types.h>

namespace apache::thrift::conformance::detail {

template <typename TT, typename = void>
struct ValueHelper;

template <>
struct ValueHelper<type::bool_t> {
  FOLLY_ERASE static void set(Value& result, bool value) {
    result.set_boolValue(value);
  }
};

template <>
struct ValueHelper<type::byte_t> {
  FOLLY_ERASE static void set(Value& result, int8_t value) {
    result.set_byteValue(value);
  }
};

template <>
struct ValueHelper<type::i16_t> {
  FOLLY_ERASE static void set(Value& result, int16_t value) {
    result.set_i16Value(value);
  }
};

template <>
struct ValueHelper<type::i32_t> {
  FOLLY_ERASE static void set(Value& result, int32_t value) {
    result.set_i32Value(value);
  }
};

template <>
struct ValueHelper<type::i64_t> {
  FOLLY_ERASE static void set(Value& result, int64_t value) {
    result.set_i64Value(value);
  }
};

template <>
struct ValueHelper<type::enum_t> {
  template <typename E>
  FOLLY_ERASE static void set(Value& result, E value) {
    result.set_i32Value(static_cast<int32_t>(value));
  }
};

template <>
struct ValueHelper<type::float_t> {
  FOLLY_ERASE static void set(Value& result, float value) {
    result.set_floatValue(value);
  }
};

template <>
struct ValueHelper<type::double_t> {
  FOLLY_ERASE static void set(Value& result, float value) {
    result.set_doubleValue(value);
  }
};

template <>
struct ValueHelper<type::string_t> {
  FOLLY_ERASE static void set(Value& result, std::string value) {
    result.set_stringValue(std::move(value));
  }
};

template <>
struct ValueHelper<type::binary_t> {
  FOLLY_ERASE static void set(Value& result, std::string value) {
    result.set_binaryValue(std::move(value));
  }
};

} // namespace apache::thrift::conformance::detail
