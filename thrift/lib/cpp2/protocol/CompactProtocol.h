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

#ifndef CPP2_PROTOCOL_COMPACTPROTOCOL_H_
#define CPP2_PROTOCOL_COMPACTPROTOCOL_H_ 1

#include <stack>

#include <folly/FBVector.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/lang/Bits.h>
#include <folly/portability/GFlags.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>

DECLARE_int32(thrift_cpp2_protocol_reader_string_limit);
DECLARE_int32(thrift_cpp2_protocol_reader_container_limit);

namespace apache {
namespace thrift {

using folly::IOBuf;
using folly::IOBufQueue;

typedef folly::io::RWPrivateCursor RWCursor;
using folly::io::Cursor;
using folly::io::QueueAppender;

namespace detail {
namespace compact {

static const int8_t COMPACT_PROTOCOL_VERSION = 0x02;
static const int32_t VERSION_2 = 0x82020000;
static const int8_t PROTOCOL_ID = int8_t(0x82);
static const int8_t TYPE_MASK = int8_t(0xE0);
static const int32_t TYPE_SHIFT_AMOUNT = 5;

// Simple stack with an inline buffer for built-in types
// Emulates the interface of std::stack
template <typename T, size_t n>
class SimpleStack {
 public:
  SimpleStack() : top_(0) {}

  void push(T v) {
    if (LIKELY(top_ < n)) {
      a_[top_++] = v;
    } else {
      heapStack_.push(v);
      top_++;
    }
  }

  T top() {
    DCHECK(top_ > 0);
    if (LIKELY(top_ <= n)) {
      return a_[top_ - 1];
    } else {
      return heapStack_.top();
    }
  }

  void pop() {
    DCHECK(top_ > 0);
    if (UNLIKELY(top_ > n)) {
      heapStack_.pop();
    }
    --top_;
  }

 private:
  T a_[n];
  size_t top_;
  std::stack<int16_t, folly::fbvector<int16_t>> heapStack_;
};

} // namespace compact
} // namespace detail

class CompactProtocolReader;

/**
 * C++ Implementation of the Compact Protocol as described in THRIFT-110
 */
class CompactProtocolWriter {
 public:
  using ProtocolReader = CompactProtocolReader;

  explicit CompactProtocolWriter(
      ExternalBufferSharing sharing = COPY_EXTERNAL_BUFFER)
      : out_(nullptr, 0),
        sharing_(sharing),
        booleanField_({nullptr, TType::T_BOOL, 0}) {}

  static constexpr ProtocolType protocolType() {
    return ProtocolType::T_COMPACT_PROTOCOL;
  }

  static constexpr bool kSortKeys() {
    return false;
  }

  /**
   * The IOBufQueue itself is managed by the caller.
   * It must exist for the life of the CompactProtocol as well,
   * or until the output is reset with setOutput/Input(NULL), or
   * set to some other backing storage buffer.
   */
  inline void setOutput(
      IOBufQueue* storage,
      size_t maxGrowth = std::numeric_limits<size_t>::max()) {
    // Allocate 16KB at a time; leave some room for the IOBuf overhead
    constexpr size_t kDesiredGrowth = (1 << 14) - 64;
    out_.reset(storage, std::min(kDesiredGrowth, maxGrowth));
  }

  inline void setOutput(QueueAppender&& output) {
    out_ = std::move(output);
  }

  inline uint32_t writeMessageBegin(
      folly::StringPiece name,
      MessageType messageType,
      int32_t seqid);
  inline uint32_t writeMessageEnd();
  inline uint32_t writeStructBegin(const char* name);
  inline uint32_t writeStructEnd();
  inline uint32_t
  writeFieldBegin(const char* name, TType fieldType, int16_t fieldId);
  inline uint32_t writeFieldBeginInternal(
      const char* name,
      const TType fieldType,
      const int16_t fieldId,
      int8_t typeOverride);
  inline uint32_t writeFieldEnd();
  inline uint32_t writeFieldStop();
  inline uint32_t writeMapBegin(TType keyType, TType valType, uint32_t size);
  inline uint32_t writeMapEnd();
  inline uint32_t writeCollectionBegin(int8_t elemType, int32_t size);
  inline uint32_t writeListBegin(TType elemType, uint32_t size);
  inline uint32_t writeListEnd();
  inline uint32_t writeSetBegin(TType elemType, uint32_t size);
  inline uint32_t writeSetEnd();
  inline uint32_t writeBool(bool value);
  inline uint32_t writeByte(int8_t byte);
  inline uint32_t writeI16(int16_t i16);
  inline uint32_t writeI32(int32_t i32);
  inline uint32_t writeI64(int64_t i64);
  inline uint32_t writeDouble(double dub);
  inline uint32_t writeFloat(float flt);
  inline uint32_t writeString(folly::StringPiece str);
  inline uint32_t writeBinary(folly::StringPiece str);
  inline uint32_t writeBinary(folly::ByteRange str);
  inline uint32_t writeBinary(const std::unique_ptr<IOBuf>& str);
  inline uint32_t writeBinary(const IOBuf& str);
  inline uint32_t writeSerializedData(
      const std::unique_ptr<folly::IOBuf>& /*data*/) {
    // TODO
    return 0;
  }

  /**
   * Functions that return the serialized size
   *
   * Notes:
   *  * Serialized size is intended to be an upper bound, rather than an exact
   *    value, since we don't want to unnecessarily pay varint encoding costs.
   *    Don't use rely on these values as more than an estimate.
   *
   *  * ZC versions are the preallocated estimate if any IOBufs are shared (i.e.
   *    there are IOBuf fields, and their sizes aren't too small to be packed),
   *    and won't count in the ZC estimate.
   *
   *    Note that we still may not pre-allocate ideally for the IOBuf case,
   *    since the IOBuf might be in the middle of the serialized stream.
   */
  inline uint32_t serializedMessageSize(folly::StringPiece name) const;
  inline uint32_t
  serializedFieldSize(const char* name, TType fieldType, int16_t fieldId) const;
  inline uint32_t serializedStructSize(const char* name) const;
  inline uint32_t
  serializedSizeMapBegin(TType keyType, TType valType, uint32_t size) const;
  inline uint32_t serializedSizeMapEnd() const;
  inline uint32_t serializedSizeListBegin(TType elemType, uint32_t size) const;
  inline uint32_t serializedSizeListEnd() const;
  inline uint32_t serializedSizeSetBegin(TType elemType, uint32_t size) const;
  inline uint32_t serializedSizeSetEnd() const;
  inline uint32_t serializedSizeStop() const;
  inline uint32_t serializedSizeBool(bool = false) const;
  inline uint32_t serializedSizeByte(int8_t = 0) const;
  inline uint32_t serializedSizeI16(int16_t = 0) const;
  inline uint32_t serializedSizeI32(int32_t = 0) const;
  inline uint32_t serializedSizeI64(int64_t = 0) const;
  inline uint32_t serializedSizeDouble(double = 0.0) const;
  inline uint32_t serializedSizeFloat(float = 0) const;
  inline uint32_t serializedSizeString(folly::StringPiece str) const;
  inline uint32_t serializedSizeBinary(folly::StringPiece str) const;
  inline uint32_t serializedSizeBinary(folly::ByteRange v) const;
  inline uint32_t serializedSizeBinary(std::unique_ptr<IOBuf> const& v) const;
  inline uint32_t serializedSizeBinary(IOBuf const& v) const;
  inline uint32_t serializedSizeZCBinary(folly::StringPiece str) const;
  inline uint32_t serializedSizeZCBinary(folly::ByteRange v) const;
  inline uint32_t serializedSizeZCBinary(
      std::unique_ptr<IOBuf> const& /*v*/) const;
  inline uint32_t serializedSizeZCBinary(IOBuf const& /*v*/) const;
  inline uint32_t serializedSizeSerializedData(
      std::unique_ptr<folly::IOBuf> const& /*data*/) const {
    // TODO (kek)
    return 0;
  }

 protected:
  /**
   * Cursor to write the data out to. Must support some of the interface of
   * folly::io::QueueAppender, notably, reset(), push(), and
   * write()/writeBE()/writeLE().
   */
  QueueAppender out_;
  ExternalBufferSharing sharing_;

  struct {
    const char* name;
    TType fieldType;
    int16_t fieldId;
  } booleanField_;

  detail::compact::SimpleStack<int16_t, 10> lastField_;
  int16_t lastFieldId_{-1};
};

class CompactProtocolReader {
 public:
  static const int8_t VERSION_MASK = 0x1f; // 0001 1111

  using ProtocolWriter = CompactProtocolWriter;

  explicit CompactProtocolReader(
      ExternalBufferSharing sharing = COPY_EXTERNAL_BUFFER)
      : string_limit_(FLAGS_thrift_cpp2_protocol_reader_string_limit),
        container_limit_(FLAGS_thrift_cpp2_protocol_reader_container_limit),
        sharing_(sharing),
        in_(nullptr),
        boolValue_({false, false}) {}

  CompactProtocolReader(
      int32_t string_limit,
      int32_t container_limit,
      ExternalBufferSharing sharing = COPY_EXTERNAL_BUFFER)
      : string_limit_(string_limit),
        container_limit_(container_limit),
        sharing_(sharing),
        in_(nullptr),
        boolValue_({false, false}) {}

  static constexpr ProtocolType protocolType() {
    return ProtocolType::T_COMPACT_PROTOCOL;
  }

  static constexpr bool kUsesFieldNames() {
    return false;
  }

  static constexpr bool kOmitsContainerSizes() {
    return false;
  }

  void setStringSizeLimit(int32_t string_limit) {
    string_limit_ = string_limit;
  }

  void setContainerSizeLimit(int32_t container_limit) {
    container_limit_ = container_limit;
  }

  /**
   * The IOBuf itself is managed by the caller.
   * It must exist for the life of the CompactProtocol as well,
   * or until the output is reset with setOutput/Input(NULL), or
   * set to some other buffer.
   */
  void setInput(const Cursor& cursor) {
    in_ = cursor;
  }
  void setInput(const IOBuf* buf) {
    in_.reset(buf);
  }

  /**
   * Reading functions
   */
  inline void
  readMessageBegin(std::string& name, MessageType& messageType, int32_t& seqid);
  inline void readMessageEnd();
  inline void readStructBegin(std::string& name);
  inline void readStructEnd();
  inline void
  readFieldBegin(std::string& name, TType& fieldType, int16_t& fieldId);
  inline void readFieldEnd();
  inline void readMapBegin(TType& keyType, TType& valType, uint32_t& size);
  inline void readMapEnd();
  inline void readListBegin(TType& elemType, uint32_t& size);
  inline void readListEnd();
  inline void readSetBegin(TType& elemType, uint32_t& size);
  inline void readSetEnd();
  inline void readBool(bool& value);
  inline void readBool(std::vector<bool>::reference value);
  inline void readByte(int8_t& byte);
  inline void readI16(int16_t& i16);
  inline void readI32(int32_t& i32);
  inline void readI64(int64_t& i64);
  inline void readDouble(double& dub);
  inline void readFloat(float& flt);
  template <typename StrType>
  inline void readString(StrType& str);
  template <typename StrType>
  inline void readBinary(StrType& str);
  inline void readBinary(std::unique_ptr<IOBuf>& str);
  inline void readBinary(IOBuf& str);
  void skip(TType type) {
    apache::thrift::skip(*this, type);
  }
  bool peekMap() {
    return false;
  }
  bool peekSet() {
    return false;
  }
  bool peekList() {
    return false;
  }

  const Cursor& getCursor() const {
    return in_;
  }

  size_t getCursorPosition() const {
    return in_.getCurrentPosition();
  }

  inline uint32_t readFromPositionAndAppend(
      Cursor& /*cursor*/,
      std::unique_ptr<folly::IOBuf>& /*ser*/) {
    // TODO
    return 0;
  }

  struct StructReadState {
    int16_t fieldId;
    apache::thrift::protocol::TType fieldType;
    // bool boolValue;

    constexpr static bool kAcceptsContext = false;

    void readStructBegin(CompactProtocolReader* iprot) {
      iprot->readStructBeginWithState(*this);
    }

    void readStructEnd(CompactProtocolReader* /*iprot*/) {}

    void readFieldBegin(CompactProtocolReader* iprot) {
      iprot->readFieldBeginWithState(*this);
    }

    FOLLY_NOINLINE void readFieldBeginNoInline(CompactProtocolReader* iprot) {
      iprot->readFieldBeginWithState(*this);
    }

    void readFieldEnd(CompactProtocolReader* /*iprot*/) {}

    FOLLY_ALWAYS_INLINE bool advanceToNextField(
        CompactProtocolReader* iprot,
        int32_t currFieldId,
        int32_t nextFieldId,
        TType nextFieldType) {
      return iprot->advanceToNextField(
          currFieldId, nextFieldId, nextFieldType, *this);
    }

    void afterAdvanceFailure(CompactProtocolReader* /*iprot*/) {}

    void beforeSubobject(CompactProtocolReader* /* iprot */) {}
    void afterSubobject(CompactProtocolReader* /* iprot */) {}

    bool atStop() {
      return fieldType == apache::thrift::protocol::T_STOP;
    }

    /*
     * This is used in generated deserialization code only. When deserializing
     * fields in "non-advanceToNextField" case, we delegate the type check to
     * each protocol since some protocol (such as NimbleProtocol) may not encode
     * type information.
     */
    FOLLY_ALWAYS_INLINE bool isCompatibleWithType(
        CompactProtocolReader* /*iprot*/,
        TType expectedFieldType) {
      return fieldType == expectedFieldType;
    }

    inline void skip(CompactProtocolReader* iprot) {
      iprot->skip(fieldType);
    }

    std::string& fieldName() {
      throw std::logic_error("CompactProtocol doesn't support field names");
    }

    template <typename StructTraits>
    void fillFieldTraitsFromName() {
      throw std::logic_error("CompactProtocol doesn't support field names");
    }
  };

 protected:
  inline void readStringSize(int32_t& size);

  template <typename StrType>
  inline void readStringBody(StrType& str, int32_t size);

  inline TType getType(int8_t type);

  inline void readStructBeginWithState(StructReadState& state);
  inline void readFieldBeginWithState(StructReadState& state);
  FOLLY_NOINLINE void readFieldBeginWithStateMediumSlow(
      StructReadState& state,
      int16_t prevFieldId);
  FOLLY_ALWAYS_INLINE void readFieldBeginWithStateImpl(
      StructReadState& state,
      int16_t prevFieldId,
      uint8_t firstByte);

  FOLLY_ALWAYS_INLINE bool
  matchTypeHeader(uint8_t byte, TType type, uint8_t diff);

  FOLLY_ALWAYS_INLINE bool advanceToNextField(
      int32_t currFieldId,
      int32_t nextFieldId,
      TType type,
      StructReadState& state);

  [[noreturn]] static void throwBadProtocolIdentifier();
  [[noreturn]] static void throwBadProtocolVersion();
  [[noreturn]] static void throwBadType(uint8_t type);

  int32_t string_limit_;
  int32_t container_limit_;
  ExternalBufferSharing sharing_;

  /**
   * Cursor to manipulate the buffer to read from.  Throws an exception if
   * there is not enough data tor ead the whole struct.
   */
  Cursor in_;

  detail::compact::SimpleStack<int16_t, 10> lastField_;
  int16_t lastFieldId_{-1};

  struct {
    bool hasBoolValue;
    bool boolValue;
  } boolValue_;

  template <typename T>
  friend class ProtocolReaderWithRefill;
  friend class CompactProtocolReaderWithRefill;
};

namespace detail {

template <class Protocol>
struct ProtocolReaderStructReadState;

template <>
struct ProtocolReaderStructReadState<CompactProtocolReader>
    : CompactProtocolReader::StructReadState {};

} // namespace detail
} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/protocol/CompactProtocol-inl.h>

#endif // #ifndef CPP2_PROTOCOL_COMPACTPROTOCOL_H_
