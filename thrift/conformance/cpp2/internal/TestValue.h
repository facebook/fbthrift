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

#include <stdexcept>

#include <fmt/core.h>
#include <folly/io/IOBuf.h>
#include <folly/lang/Exception.h>
#include <thrift/conformance/cpp2/ThriftTypes.h>
#include <thrift/conformance/if/gen-cpp2/test_value_types.h>

namespace apache::thrift::conformance::detail {

template <typename W>
uint32_t invoke(WriteToken token, W& writer) {
  switch (token) {
    case WriteToken::StructEnd:
      return writer.writeStructEnd();
    case WriteToken::FieldEnd:
      return writer.writeFieldEnd();
    case WriteToken::FieldStop:
      return writer.writeFieldStop();
    case WriteToken::MapEnd:
      return writer.writeMapEnd();
    case WriteToken::ListEnd:
      return writer.writeListEnd();
    case WriteToken::SetEnd:
      return writer.writeSetEnd();
    default:
      folly::throw_exception<std::runtime_error>(
          fmt::format("Unknown write token: {}", token));
  }
}

template <typename W>
uint32_t invoke(const WriteOp& write, W& writer) {
  switch (write.getType()) {
    case WriteOp::writeBool:
      return writer.writeBool(*write.writeBool_ref());
    case WriteOp::writeByte:
      return writer.writeByte(*write.writeByte_ref());
    case WriteOp::writeI16:
      return writer.writeI16(*write.writeI16_ref());
    case WriteOp::writeI32:
      return writer.writeI32(*write.writeI32_ref());
    case WriteOp::writeI64:
      return writer.writeI64(*write.writeI64_ref());
    case WriteOp::writeFloat:
      return writer.writeFloat(*write.writeFloat_ref());
    case WriteOp::writeDouble:
      return writer.writeDouble(*write.writeDouble_ref());
    case WriteOp::writeString:
      return writer.writeString(*write.writeString_ref());
    case WriteOp::writeBinary:
      return writer.writeBinary(*write.writeBinary_ref());

    case WriteOp::writeToken:
      return invoke(*write.writeToken_ref(), writer);

    case WriteOp::writeStructBegin: {
      const auto& begin = *write.writeStructBegin_ref();
      return writer.writeStructBegin(begin.name_ref()->c_str());
    }
    case WriteOp::writeFieldBegin: {
      const auto& begin = *write.writeFieldBegin_ref();
      return writer.writeFieldBegin(
          begin.name_ref()->c_str(),
          toTType(*begin.type_ref()),
          *begin.id_ref());
    }
    case WriteOp::writeMapBegin: {
      const auto& begin = *write.writeMapBegin_ref();
      return writer.writeMapBegin(
          toTType(*begin.keyType_ref()),
          toTType(*begin.valueType_ref()),
          *begin.size_ref());
    }
    case WriteOp::writeListBegin: {
      const auto& begin = *write.writeListBegin_ref();
      return writer.writeListBegin(
          toTType(*begin.elemType_ref()), *begin.size_ref());
    }
    case WriteOp::writeSetBegin: {
      const auto& begin = *write.writeSetBegin_ref();
      return writer.writeSetBegin(
          toTType(*begin.elemType_ref()), *begin.size_ref());
    }

    default:
      folly::throw_exception<std::runtime_error>(
          fmt::format("Unknown write call: {}", write.getType()));
  }
}

template <typename C, typename W>
uint32_t invoke(const C& writes, W& writer) {
  uint32_t result = 0;
  for (const WriteOp& write : writes) {
    result += invoke(write, writer);
  }
  return result;
}

class EncodeValueRecorder {
 public:
  explicit EncodeValueRecorder(EncodeValue* output) : output_(*output) {}
  constexpr static bool kSortKeys() { return false; }
  constexpr static bool kHasIndexSupport() { return false; }

  uint32_t writeStructBegin(const char* name) {
    next().writeStructBegin_ref().ensure().name_ref() = name;
    return 0;
  }

  uint32_t writeStructEnd() {
    nextToken() = WriteToken::StructEnd;
    return 0;
  }

  uint32_t writeFieldBegin(const char* name, TType fieldType, int16_t fieldId) {
    auto& fieldBegin = next().writeFieldBegin_ref().ensure();
    fieldBegin.name_ref() = name;
    fieldBegin.type_ref() = toThriftBaseType(fieldType);
    fieldBegin.id_ref() = fieldId;
    return 0;
  }

  uint32_t writeFieldEnd() {
    nextToken() = WriteToken::FieldEnd;
    return 0;
  }

  uint32_t writeFieldStop() {
    nextToken() = WriteToken::FieldStop;
    return 0;
  }

  uint32_t writeMapBegin(TType keyType, TType valType, uint32_t size) {
    auto& mapBegin = next().writeMapBegin_ref().ensure();
    mapBegin.keyType_ref() = toThriftBaseType(keyType);
    mapBegin.valueType_ref() = toThriftBaseType(valType);
    mapBegin.size_ref() = size;
    return 0;
  }

  uint32_t writeMapEnd() {
    nextToken() = WriteToken::MapEnd;
    return 0;
  }

  uint32_t writeListBegin(TType elemType, uint32_t size) {
    auto& listBegin = next().writeListBegin_ref().ensure();
    listBegin.elemType_ref() = toThriftBaseType(elemType);
    listBegin.size_ref() = size;
    return 0;
  }

  uint32_t writeListEnd() {
    nextToken() = WriteToken::ListEnd;
    return 0;
  }

  uint32_t writeSetBegin(TType elemType, uint32_t size) {
    auto& setBegin = next().writeSetBegin_ref().ensure();
    setBegin.elemType_ref() = toThriftBaseType(elemType);
    setBegin.size_ref() = size;
    return 0;
  }

  uint32_t writeSetEnd() {
    nextToken() = WriteToken::SetEnd;
    return 0;
  }

  uint32_t writeBool(bool value) {
    next().writeBool_ref().ensure() = value;
    return 0;
  }

  uint32_t writeByte(int8_t byte) {
    next().writeByte_ref().ensure() = byte;
    return 0;
  }

  uint32_t writeI16(int16_t i16) {
    next().writeI16_ref().ensure() = i16;
    return 0;
  }

  uint32_t writeI32(int32_t i32) {
    next().writeI32_ref().ensure() = i32;
    return 0;
  }

  uint32_t writeI64(int64_t i64) {
    next().writeI64_ref().ensure() = i64;
    return 0;
  }

  uint32_t writeDouble(double dub) {
    next().writeDouble_ref().ensure() = dub;
    return 0;
  }

  uint32_t writeFloat(float flt) {
    next().writeFloat_ref().ensure() = flt;
    return 0;
  }

  uint32_t writeString(folly::StringPiece str) {
    next().writeString_ref().ensure() = std::string(str);
    return 0;
  }

  uint32_t writeBinary(const folly::IOBuf& str) {
    next().writeBinary_ref().ensure() = str;
    return 0;
  }

  uint32_t writeBinary(std::unique_ptr<folly::IOBuf> str) {
    next().writeBinary_ref().ensure() = std::move(*str);
    return 0;
  }

  uint32_t writeBinary(folly::ByteRange str) {
    return writeBinary(folly::IOBuf::copyBuffer(str));
  }

  uint32_t writeBinary(folly::StringPiece str) {
    return writeBinary(folly::ByteRange(str));
  }

 private:
  EncodeValue& output_;

  WriteOp& next() { return output_.writes_ref()->emplace_back(); }

  WriteToken& nextToken() { return next().writeToken_ref().ensure(); }
};

} // namespace apache::thrift::conformance::detail
