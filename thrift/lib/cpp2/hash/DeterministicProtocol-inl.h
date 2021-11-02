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

#include <limits>
#include <utility>

#include <thrift/lib/cpp/protocol/TProtocol.h>

namespace apache {
namespace thrift {
namespace hash {

template <typename Accumulator>
DeterministicProtocol<Accumulator>::DeterministicProtocol(
    Accumulator accumulator)
    : accumulator_{std::move(accumulator)} {}

template <typename Accumulator>
constexpr bool DeterministicProtocol<Accumulator>::kSortKeys() {
  return false;
}

template <typename Accumulator>
DeterministicProtocol<Accumulator>::operator Accumulator&&() && {
  return std::move(accumulator_);
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeStructBegin(
    const char* /* unused */) {
  accumulator_.unorderedElementsBegin();
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeStructEnd() {
  accumulator_.unorderedElementsEnd();
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeFieldBegin(
    const char* /* unused */, protocol::TType fieldType, std::int16_t fieldId) {
  accumulator_.orderedElementsBegin();
  writeByte(static_cast<std::int8_t>(fieldType));
  writeI16(fieldId);
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeFieldEnd() {
  accumulator_.orderedElementsEnd();
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeFieldStop() {
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeMapBegin(
    protocol::TType keyType, protocol::TType valType, std::uint32_t size) {
  accumulator_.orderedElementsBegin();
  writeByte(static_cast<std::int8_t>(keyType));
  writeByte(static_cast<std::int8_t>(valType));
  writeI32(static_cast<std::int32_t>(size));
  accumulator_.unorderedElementsBegin();
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeMapValueBegin() {
  accumulator_.orderedElementsBegin();
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeMapValueEnd() {
  accumulator_.orderedElementsEnd();
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeMapEnd() {
  accumulator_.unorderedElementsEnd();
  accumulator_.orderedElementsEnd();
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeListBegin(
    protocol::TType elemType, std::uint32_t size) {
  accumulator_.orderedElementsBegin();
  writeByte(static_cast<std::int8_t>(elemType));
  writeI32(static_cast<std::int32_t>(size));
  accumulator_.orderedElementsBegin();
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeListEnd() {
  accumulator_.orderedElementsEnd();
  accumulator_.orderedElementsEnd();
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeSetBegin(
    protocol::TType elemType, std::uint32_t size) {
  accumulator_.orderedElementsBegin();
  writeByte(static_cast<std::int8_t>(elemType));
  writeI32(static_cast<std::int32_t>(size));
  accumulator_.unorderedElementsBegin();
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeSetEnd() {
  accumulator_.unorderedElementsEnd();
  accumulator_.orderedElementsEnd();
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeBool(bool value) {
  accumulator_.combine(value);
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeByte(std::int8_t value) {
  accumulator_.combine(value);
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeI16(std::int16_t value) {
  accumulator_.combine(value);
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeI32(std::int32_t value) {
  accumulator_.combine(value);
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeI64(std::int64_t value) {
  accumulator_.combine(value);
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeDouble(double value) {
  accumulator_.combine(value);
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeFloat(float value) {
  accumulator_.combine(value);
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeString(
    folly::StringPiece value) {
  writeBinary(value);
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeBinary(
    folly::StringPiece value) {
  writeBinary(folly::ByteRange{value});
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeBinary(
    folly::ByteRange value) {
  writeBuffer(value, value.size());
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeBinary(
    const std::unique_ptr<folly::IOBuf>& value) {
  if (value == nullptr) {
    writeBinary("");
  } else {
    writeBinary(*value);
  }
  return 0;
}

template <typename Accumulator>
std::uint32_t DeterministicProtocol<Accumulator>::writeBinary(
    const folly::IOBuf& value) {
  writeBuffer(value, value.computeChainDataLength());
  return 0;
}

template <typename Accumulator>
template <typename Buffer>
void DeterministicProtocol<Accumulator>::writeBuffer(
    const Buffer& buffer, std::size_t size) {
  std::uint32_t limit = std::numeric_limits<std::uint32_t>::max();
  if (size > limit) {
    protocol::TProtocolException::throwExceededSizeLimit(size, limit);
  }
  accumulator_.orderedElementsBegin();
  writeI32(static_cast<std::int32_t>(size));
  accumulator_.combine(buffer);
  accumulator_.orderedElementsEnd();
}

} // namespace hash
} // namespace thrift
} // namespace apache
