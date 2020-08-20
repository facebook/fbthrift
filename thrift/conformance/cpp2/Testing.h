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

// Utilities for unit testing conformance implementions.

#pragma once

#include <folly/Conv.h>
#include <folly/io/Cursor.h>
#include <thrift/conformance/cpp2/AnySerializer.h>
#include <thrift/conformance/cpp2/Protocol.h>

namespace apache::thrift::conformance {

// Always serializes integers to the number 1.
class Number1Serializer
    : public BaseTypedAnySerializer<int, Number1Serializer> {
  using Base = BaseTypedAnySerializer<int, Number1Serializer>;

 public:
  static const Protocol kProtocol;

  const Protocol& getProtocol() const override {
    return kProtocol;
  }

  using Base::encode;
  void encode(const int&, folly::io::QueueAppender&& appender) const {
    std::string data = "number 1!!";
    appender.push(reinterpret_cast<const uint8_t*>(data.data()), data.size());
  }

  using Base::decode;
  int decode(folly::io::Cursor& cursor) const {
    cursor.readFixedString(10);
    return 1;
  }
};

extern const Protocol kFollyToStringProtocol;

// A serializer based on `folly::to<std::string>`.
template <typename T>
class FollyToStringSerializer
    : public BaseTypedAnySerializer<T, FollyToStringSerializer<T>> {
  using Base = BaseTypedAnySerializer<T, FollyToStringSerializer<T>>;

 public:
  const Protocol& getProtocol() const override {
    return kFollyToStringProtocol;
  }
  using Base::encode;
  void encode(const T& value, folly::io::QueueAppender&& appender) const {
    std::string data = folly::to<std::string>(value);
    appender.push(reinterpret_cast<const uint8_t*>(data.data()), data.size());
  }
  using Base::decode;
  T decode(folly::io::Cursor& cursor) const {
    return folly::to<T>(cursor.readFixedString(cursor.totalLength()));
  }
};

class MultiSerializer : public AnySerializer {
  using Base = AnySerializer;

 public:
  mutable size_t intEncCount = 0;
  mutable size_t dblEncCount = 0;

  mutable size_t intDecCount = 0;
  mutable size_t dblDecCount = 0;
  mutable size_t anyDecCount = 0;

  using Base::decode;
  using Base::encode;

  const Protocol& getProtocol() const override {
    return kFollyToStringProtocol;
  }
  void encode(any_ref value, folly::io::QueueAppender&& appender)
      const override;
  void decode(
      const std::type_info& typeInfo,
      folly::io::Cursor& cursor,
      any_ref value) const override;

  // Helper functions to check the statis
  void checkAndResetInt(size_t enc, size_t dec) const;
  void checkAndResetDbl(size_t enc, size_t dec) const;
  void checkAndResetAny(size_t dec) const;
  void checkAndResetAll() const;
  void checkAnyDec() const;
  void checkIntEnc() const;
  void checkIntDec() const;
  void checkDblEnc() const;
  void checkDblDec() const;
  void checkAnyIntDec() const;
  void checkAnyDblDec() const;
};

} // namespace apache::thrift::conformance
