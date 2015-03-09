/*
 * Copyright 2015 Facebook, Inc.
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
#ifndef CPP2_PROTOCOL_PROTOCOLREADER_WITHREFILL_H_
#define CPP2_PROTOCOL_PROTOCOLREADER_WITHREFILL_H_ 1

#include <folly/FBString.h>
#include <thrift/lib/cpp2/protocol/VirtualProtocol.h>

namespace apache { namespace thrift {

template<typename ProtocolT> class ProtocolReaderWithRefill;

using VirtualBinaryReader = ProtocolReaderWithRefill<BinaryProtocolReader>;
using VirtualCompactReader = ProtocolReaderWithRefill<CompactProtocolReader>;
using Refiller = std::function<std::unique_ptr<folly::IOBuf>(
    const uint8_t*, int, int)>;

/**
 * Used by python fastproto module. Read methods must check if there are
 * enough bytes to read. If not, call the refiller to read more from python.
 */
template<typename ProtocolT>
class ProtocolReaderWithRefill : public VirtualReader<ProtocolT> {
  public:
    explicit ProtocolReaderWithRefill(Refiller refiller)
      : refiller_(refiller)
      , buffer_(nullptr)
      , bufferLength_(0) {}

    uint32_t totalBytesRead() {
      return bufferLength_ - this->protocol_.in_.length();
    }

    void setInput(const folly::IOBuf* buf) override {
      VirtualReader<ProtocolT>::setInput(buf);
      bufferLength_ = buf->length();
    }

  protected:
    void ensureBuffer(uint32_t size) {
      if (this->protocol_.in_.length() >= size) {
        return;
      }

      auto avail = this->protocol_.in_.peek();
      buffer_ = refiller_(avail.first, avail.second, size);
      setInput(buffer_.get());
    }

    Refiller refiller_;
    std::unique_ptr<folly::IOBuf> buffer_;
    uint32_t bufferLength_;
};

class CompactProtocolReaderWithRefill : public VirtualCompactReader {
  public:
    explicit CompactProtocolReaderWithRefill(Refiller refiller)
      : VirtualCompactReader(refiller) {}

    inline uint32_t readMessageBegin(std::string& name,
                                     MessageType& messageType,
                                     int32_t& seqid) {
      // Only called in python so leave it unimplemented.
      throw std::runtime_error("not implemented");
    }

    inline uint32_t readFieldBegin(std::string& name,
                                   TType& fieldType,
                                   int16_t& fieldId) {
      ensureFieldBegin();
      return protocol_.readFieldBegin(name, fieldType, fieldId);
    }

    inline uint32_t readMapBegin(TType& keyType,
                                 TType& valType,
                                 uint32_t& size) {
      ensureMapBegin();
      return protocol_.readMapBegin(keyType, valType, size);
    }

    inline uint32_t readListBegin(TType& elemType, uint32_t& size) {
      ensureListBegin();
      return protocol_.readListBegin(elemType, size);
    }

    inline uint32_t readBool(bool& value) {
      if (!protocol_.boolValue_.hasBoolValue) {
        ensureBuffer(1);
      }
      return protocol_.readBool(value);
    }

    inline uint32_t readBool(std::vector<bool>::reference value) {
      bool ret = false;
      uint32_t sz = readBool(ret);
      value = ret;
      return sz;
    }

    inline uint32_t readByte(int8_t& byte) {
      ensureBuffer(1);
      return protocol_.readByte(byte);
    }

    inline uint32_t readI16(int16_t& i16) {
      ensureInteger();
      return protocol_.readI16(i16);
    }

    inline uint32_t readI32(int32_t& i32) {
      ensureInteger();
      return protocol_.readI32(i32);
    }

    inline uint32_t readI64(int64_t& i64) {
      ensureInteger();
      return protocol_.readI64(i64);
    }

    inline uint32_t readDouble(double& dub) {
      ensureBuffer(8);
      return protocol_.readDouble(dub);
    }

    inline uint32_t readFloat(float& flt) {
      ensureBuffer(4);
      return protocol_.readFloat(flt);
    }

    inline uint32_t readString(std::string& str) {
      return readStringImpl(str);
    }

    inline uint32_t readString(folly::fbstring& str) {
      return readStringImpl(str);
    }

    inline uint32_t readBinary(std::string& str) {
      return readStringImpl(str);
    }

    inline uint32_t readBinary(folly::fbstring& str) {
      return readStringImpl(str);
    }

    inline uint32_t readBinary(std::unique_ptr<folly::IOBuf>& str) {
      return readBinaryIOBufImpl(str);
    }

    inline uint32_t readBinary(folly::IOBuf& str) {
      return readBinaryIOBufImpl(str);
    }

    inline uint32_t skip(TType type) {
      return apache::thrift::skip(*this, type);
    }

  private:
    /**
     * Make sure a varint can be read from the current buffer after idx bytes.
     * If not, call the refiller to read more bytes.
     *
     * A varint is stored with up to 10 bytes and only the last byte's
     * MSB isn't set. If the current buffer size is >= idx + 10, return. The
     * following call to readVarint may still fail if the first 10 bytes
     * all have MSB set, but it's not the problem to be addressed here.
     *
     * Otherwise, check if a byte with MSB not set can be found. If so, return.
     * Otherwise, call the refiller to ask for 1 more byte because the exact
     * size of the varint is still unknown but at leat 1 more byte is required.
     * A sane transport reads more data even if asked for just 1 byte so this
     * should not cause any performance problem. After the new buffer is ready,
     * start all over again.
     **/
    void ensureInteger(int idx = 0) {
      while (true) {
        if (protocol_.in_.length() - idx >= 10) {
          return;
        }

        if (protocol_.in_.length() <= idx) {
          ensureBuffer(idx + 1);
        } else {
          auto avail = protocol_.in_.peek();
          const uint8_t *b = avail.first + idx;
          while (idx++ < avail.second) {
            if (!(*b++ & 0x80))
              return;
          }

          ensureBuffer(avail.second + 1);
        }
      }
    }

    void ensureFieldBegin() {
      // Fast path: at most 4 bytes are needed to read field begin.
      if (protocol_.in_.length() >= 4)
        return;

      // At least 1 byte is needed to read ftype.
      ensureBuffer(1);
      if (protocol_.in_.length() >= 4)
        return;
      auto avail = protocol_.in_.peek();
      const uint8_t *b = avail.first;
      int8_t byte = folly::Endian::big(*b);
      int8_t type = (byte & 0x0f);

      if (type == TType::T_STOP)
        return;

      int16_t modifier = (int16_t)(((uint8_t)byte & 0xf0) >> 4);
      if (modifier == 0) {
        ensureInteger(1);
      }
    }

    void ensureMapBegin() {
      // Fast path: at most 11 bytes are needed to read map begin.
      if (protocol_.in_.length() >= 11)
        return;

      ensureInteger();
      if (protocol_.in_.length() >= 11)
        return;

      auto avail = protocol_.in_.peek();
      const uint8_t *b = avail.first;
      int bytes = 1;
      while (bytes <= avail.second) {
        if (*b++ == 0)
          break;
        bytes++;
      }
      if (bytes == 1 || bytes != avail.second) {
        return;
      } else { // Still need 1 more byte to read key/value type
        ensureBuffer(avail.second + 1);
      }
    }

    void ensureListBegin() {
      // Fast path: at most 11 bytes are needed to read list begin.
      if (protocol_.in_.length() >= 11)
        return;

      auto avail = protocol_.in_.peek();
      const uint8_t *b = avail.first;
      int8_t size_and_type = folly::Endian::big(*b);
      int32_t lsize = ((uint8_t)size_and_type >> 4) & 0x0f;
      if (lsize == 15) {
        ensureInteger(1);
      }
    }

    template<typename StrType>
    uint32_t readStringImpl(StrType& str) {
      ensureInteger();
      int32_t size = 0;
      uint32_t rsize = protocol_.readStringSize(size);

      ensureBuffer(size);
      return rsize + protocol_.readStringBody(str, size);
    }

    template<typename StrType>
    uint32_t readBinaryIOBufImpl(StrType& str) {
      ensureInteger();
      int32_t size = 0;
      uint32_t rsize = protocol_.readStringSize(size);

      ensureBuffer(size);
      protocol_.in_.clone(str, size);
      return rsize + (uint32_t)size;
    }
};

class BinaryProtocolReaderWithRefill : public VirtualBinaryReader {
  public:
    explicit BinaryProtocolReaderWithRefill(Refiller refiller)
      : VirtualBinaryReader(refiller) {}

    inline uint32_t readMessageBegin(std::string& name,
                                     MessageType& messageType,
                                     int32_t& seqid) {
      // This is only called in python so leave it unimplemented.
      throw std::runtime_error("not implemented");
    }

    inline uint32_t readFieldBegin(std::string& name,
                                   TType& fieldType,
                                   int16_t& fieldId) {
      uint32_t result = 0;
      int8_t type;
      result += readByte(type);
      fieldType = (TType)type;
      if (fieldType == TType::T_STOP) {
        fieldId = 0;
        return result;
      }
      result += readI16(fieldId);
      return result;
    }

    inline uint32_t readMapBegin(TType& keyType,
                                 TType& valType,
                                 uint32_t& size) {
      ensureBuffer(6);
      return protocol_.readMapBegin(keyType, valType, size);
    }

    inline uint32_t readListBegin(TType& elemType, uint32_t& size) {
      ensureBuffer(5);
      return protocol_.readListBegin(elemType, size);
    }

    inline uint32_t readSetBegin(TType& elemType, uint32_t& size) {
      return readListBegin(elemType, size);
    }

    inline uint32_t readBool(bool& value) {
      ensureBuffer(1);
      return protocol_.readBool(value);
    }

    inline uint32_t readBool(std::vector<bool>::reference value) {
      ensureBuffer(1);
      return protocol_.readBool(value);
    }

    inline uint32_t readByte(int8_t& byte) {
      ensureBuffer(1);
      return protocol_.readByte(byte);
    }

    inline uint32_t readI16(int16_t& i16) {
      ensureBuffer(2);
      return protocol_.readI16(i16);
    }

    inline uint32_t readI32(int32_t& i32) {
      ensureBuffer(4);
      return protocol_.readI32(i32);
    }

    inline uint32_t readI64(int64_t& i64) {
      ensureBuffer(8);
      return protocol_.readI64(i64);
    }

    inline uint32_t readDouble(double& dub) {
      ensureBuffer(8);
      return protocol_.readDouble(dub);
    }

    inline uint32_t readFloat(float& flt) {
      ensureBuffer(4);
      return protocol_.readFloat(flt);
    }

    inline uint32_t readString(std::string& str) {
      return readStringImpl(str);
    }

    inline uint32_t readString(folly::fbstring& str) {
      return readStringImpl(str);
    }

    inline uint32_t readBinary(std::string& str) {
      return readStringImpl(str);
    }

    inline uint32_t readBinary(folly::fbstring& str) {
      return readStringImpl(str);
    }

    inline uint32_t readBinary(std::unique_ptr<folly::IOBuf>& str) {
      return readBinaryIOBufImpl(str);
    }

    inline uint32_t readBinary(folly::IOBuf& str) {
      return readBinaryIOBufImpl(str);
    }

    inline uint32_t skip(TType type) {
      return apache::thrift::skip(*this, type);
    }

  private:
    template<typename StrType>
    uint32_t readStringImpl(StrType& str) {
      uint32_t result;
      int32_t size;
      result = readI32(size);
      protocol_.checkStringSize(size);

      ensureBuffer(size);
      return result + protocol_.readStringBody(str, size);
    }

    template<typename StrType>
    uint32_t readBinaryIOBufImpl(StrType& str) {
      uint32_t result;
      int32_t size;
      result = readI32(size);
      protocol_.checkStringSize(size);

      ensureBuffer(size);
      protocol_.in_.clone(str, size);
      return result + (uint32_t)size;
    }
};

}} //apache::thrift

#endif // #ifndef CPP2_PROTOCOL_PROTOCOLREADER_WITHREFILL_H_
