/*
 * Copyright 2004-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>

#include <thrift/lib/cpp2/frozen/Frozen.h>
#include <thrift/lib/cpp2/frozen/FrozenUtil.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>

namespace apache {
namespace thrift {

namespace detail {

constexpr int8_t FROZEN_PROTOCOL_ID = 0x83;
constexpr int32_t FROZEN_PROTOCOL_VERSION = 0x02;
constexpr int32_t VERSION_MASK = 0xffff0000;

} // apache::thrift::detail

class Frozen2ProtocolReader;

class Frozen2ProtocolWriter {
 public:
  using ProtocolReader = Frozen2ProtocolReader;

  explicit Frozen2ProtocolWriter() : out_(nullptr, 0) {}

  static constexpr ProtocolType protocolType() {
    return ProtocolType::T_FROZEN2_PROTOCOL;
  }

  inline void setOutput(
      folly::IOBufQueue* queue,
      size_t maxGrowth = std::numeric_limits<size_t>::max()) {
    // Allocate 16KB at a time; leave some room for the IOBuf overhead
    constexpr size_t kDesiredGrowth = (1 << 14) - 64;
    out_.reset(queue, std::min(maxGrowth, kDesiredGrowth));
  }

  inline uint32_t writeMessageBegin(
      const std::string& name,
      MessageType messageType,
      int32_t seqid);

  inline uint32_t writeMessageEnd();

  template <typename T>
  inline uint32_t writeObject(T&);

  inline uint32_t serializedMessageSize(const std::string& name) const;

  template <typename T>
  inline uint32_t serializedObjectSize(T&) const;

 protected:
  inline uint32_t serializedSizeByte() const;
  inline uint32_t serializedSizeI16() const;
  inline uint32_t serializedSizeI32() const;
  inline uint32_t serializedSizeString(folly::StringPiece) const;
  inline uint32_t writeByte(int8_t);
  inline uint32_t writeI16(int16_t);
  inline uint32_t writeI32(int32_t);
  inline uint32_t writeString(folly::StringPiece);

 private:
  folly::io::QueueAppender out_;
};

class Frozen2ProtocolReader {
 public:
  using ProtocolWriter = Frozen2ProtocolWriter;

  static constexpr ProtocolType protocolType() {
    return ProtocolType::T_FROZEN2_PROTOCOL;
  }

  explicit Frozen2ProtocolReader() : in_(nullptr) {}

  void setInput(const folly::io::Cursor& cursor) {
    in_ = cursor;
  }
  void setInput(const folly::IOBuf* buf) {
    setInput(folly::io::Cursor(buf));
  }

  inline uint32_t
  readMessageBegin(std::string& name, MessageType& messageType, int32_t& seqid);

  inline uint32_t readMessageEnd();

  template <typename T>
  inline uint32_t readObject(T&);

  uint32_t skip(TType type) {
    if (type == TType::T_STRUCT) {
      std::string str;
      return readString(str);
    }
    return 0;
  }

 protected:
  inline uint32_t readI32(int32_t&);
  inline uint32_t readString(std::string&);

  [[noreturn]] static void throwBadProtocolId(int32_t sz);
  [[noreturn]] static void throwBadProtocolVersion(int32_t sz);

 private:
  inline uint32_t readStringBody(std::string&, int32_t);
  folly::io::Cursor in_;
};
}
} // apache::thrift

#include "Frozen2Protocol.tcc"
