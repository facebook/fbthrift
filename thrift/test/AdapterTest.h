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

#include <chrono>
#include <stdexcept>
#include <string>

#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/Thrift.h>

namespace apache::thrift::test {

namespace basic {
enum class AdaptedEnum {
  Zero = 0,
  One,
  Two,
};
}

template <typename T>
struct Wrapper {
  T value;

  Wrapper& operator=(const T&) = delete;

  bool operator==(const Wrapper& other) const { return value == other.value; }
  bool operator<(const Wrapper& other) const { return value < other.value; }
};

struct TemplatedTestAdapter {
  template <typename T>
  static Wrapper<T> fromThrift(T value) {
    return {value};
  }

  template <typename T>
  static T toThrift(Wrapper<T> wrapper) {
    return wrapper.value;
  }
};

struct AdaptTestMsAdapter {
  static std::chrono::milliseconds fromThrift(int64_t ms) {
    return std::chrono::milliseconds{ms};
  }

  static int64_t toThrift(std::chrono::milliseconds duration) {
    return duration.count();
  }
};

struct Num {
  int64_t val = 13;

 private:
  friend bool operator==(const Num& lhs, const Num& rhs) {
    return lhs.val == rhs.val;
  }
  friend bool operator<(const Num& lhs, const Num& rhs) {
    return lhs.val < rhs.val;
  }
};

struct String {
  std::string val;
};

struct IndirectionString {
  FBTHRIFT_CPP_DEFINE_MEMBER_INDIRECTION_FN(val);
  std::string val;
};

struct OverloadedAdapter {
  static Num fromThrift(int64_t val) { return Num{val}; }
  static String fromThrift(std::string&& val) { return String{std::move(val)}; }

  static int64_t toThrift(const Num& num) { return num.val; }
  static const std::string& toThrift(const String& str) { return str.val; }
};

struct CustomProtocolAdapter {
  static Num fromThrift(const folly::IOBuf& data) {
    if (data.length() < sizeof(int64_t)) {
      throw std::invalid_argument("CustomProtocolAdapter parse error");
    }
    return {*reinterpret_cast<const int64_t*>(data.data())};
  }

  static folly::IOBuf toThrift(const Num& num) {
    return folly::IOBuf::wrapBufferAsValue(&num.val, sizeof(int64_t));
  }
};

// TODO(afuller): Move this to a shared location.
template <typename AdaptedT>
struct IndirectionAdapter {
  template <typename ThriftT>
  FOLLY_ERASE static constexpr AdaptedT fromThrift(ThriftT&& value) {
    AdaptedT adapted;
    toThrift(adapted) = std::forward<ThriftT>(value);
    return adapted;
  }
  FOLLY_ERASE static constexpr decltype(auto)
  toThrift(AdaptedT& adapted) noexcept(
      noexcept(::apache::thrift::apply_indirection(adapted))) {
    return ::apache::thrift::apply_indirection(adapted);
  }
};

struct AdaptedWithContext {
  int64_t value = 0;
  int16_t fieldId = 0;
  std::string* meta = nullptr;
};

inline bool operator==(
    const AdaptedWithContext& lhs, const AdaptedWithContext& rhs) {
  return lhs.value == rhs.value;
}

inline bool operator<(
    const AdaptedWithContext& lhs, const AdaptedWithContext& rhs) {
  return lhs.value < rhs.value;
}

struct AdapterWithContext {
  template <typename Context>
  static void construct(AdaptedWithContext& field, Context&& ctx) {
    field.meta = &*ctx.object.meta_ref();
  }

  template <typename Context>
  static AdaptedWithContext fromThriftField(int64_t value, Context&& ctx) {
    return {value, Context::kFieldId, &*ctx.object.meta_ref()};
  }

  static int64_t toThrift(const AdaptedWithContext& adapted) {
    return adapted.value;
  }
};

} // namespace apache::thrift::test
