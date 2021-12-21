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

#include <cstdint>
#include <limits>
#include <memory>
#include <utility>

#include <thrift/lib/cpp/protocol/TProtocol.h>

#include <folly/Range.h>
#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp/protocol/TType.h>

namespace apache::thrift::op::detail {

/**
 * A deterministic one-way/write-only custom protocol that guarantees the
 * accumulated results to be stable and consistent across different languages
 * and implementations. Protocol wraps an accumulator and allows users to
 * provide custom implementations for it.
 *
 * Names:
 *   To enable better development experience we do not include struct names,
 *   field names and paths into the hash computation, but we do include field
 *   id. This way refactoring to rename or move a struct around will not change
 *   the computed hash values. One thing to note here if the hashes are used for
 *   signatures or keyed mac computations you might want to use a different key
 *   per struct to avoid collisions in case struct layouts match.
 *
 * Ordering:
 *   Fields, Maps and Sets are treated as unordered collections.
 *   Lists, Prefixes and Map Values are treated as ordered collections.
 *   Re-arranging fields in a struct will not change the hash if you preserve
 *   field ids. The specifics of order handling is delegated to Accumulator
 *   implementation.
 *
 * Prefixing:
 *   To have a better collision protection all fields are prefixed with a type
 *   tag that goes into the hash computation. All dynamic sized (non-scalar)
 *   types also get a size prefix.
 */
// TODO(afuller): Do not use a protocol for this, which requires the custom
// protocol header to be included by the caller and for adapted types to be
// converted to thrift before being hashed, even if they can be hashed
// directly (via the a hook on the given Adapter). Instead add ThriftType
// info and accessors to the struct definition, and use that.
template <typename Accumulator>
class HashProtocol {
 public:
  explicit HashProtocol(Accumulator& accumulator) : acc_(accumulator) {}

  static constexpr bool kSortKeys() { return false; }

  uint32_t writeStructBegin(const char*) { return (acc_.beginUnordered(), 0); }
  uint32_t writeStructEnd() { return (acc_.endUnordered(), 0); }
  uint32_t writeFieldBegin(const char*, protocol::TType type, int16_t id);
  uint32_t writeFieldEnd() { return (acc_.endOrdered(), 0); }
  uint32_t writeFieldStop() { return 0; }
  uint32_t writeMapBegin(
      protocol::TType keyType, protocol::TType valType, uint32_t size) {
    return (beginContainer(size, keyType, valType), acc_.beginUnordered(), 0);
  }
  uint32_t writeMapValueBegin() { return (acc_.beginOrdered(), 0); }
  uint32_t writeMapValueEnd() { return (acc_.endOrdered(), 0); }
  uint32_t writeMapEnd() { return (acc_.endUnordered(), endContainer()); }
  uint32_t writeListBegin(protocol::TType elemType, uint32_t size) {
    return (beginContainer(size, elemType), acc_.beginOrdered(), 0);
  }
  uint32_t writeListEnd() { return (acc_.endOrdered(), endContainer()); }
  uint32_t writeSetBegin(protocol::TType elemType, uint32_t size) {
    return (beginContainer(size, elemType), acc_.beginUnordered(), 0);
  }
  uint32_t writeSetEnd() { return (acc_.endUnordered(), endContainer()); }
  uint32_t writeBool(bool val) { return (acc_.combine(val), 0); }
  uint32_t writeByte(int8_t val) { return (acc_.combine(val), 0); }
  uint32_t writeI16(int16_t val) { return (acc_.combine(val), 0); }
  uint32_t writeI32(int32_t val) { return (acc_.combine(val), 0); }
  uint32_t writeI64(int64_t val) { return (acc_.combine(val), 0); }
  uint32_t writeDouble(double val) { return (acc_.combine(val), 0); }
  uint32_t writeFloat(float val) { return (acc_.combine(val), 0); }
  uint32_t writeString(folly::StringPiece val) { return writeBuffer(val); }
  uint32_t writeBinary(folly::StringPiece val) { return writeBuffer(val); }
  uint32_t writeBinary(folly::ByteRange val) { return writeBuffer(val); }
  uint32_t writeBinary(const std::unique_ptr<folly::IOBuf>& val) {
    return val == nullptr ? writeBinary("") : writeBinary(*val);
  }
  uint32_t writeBinary(const folly::IOBuf& val) {
    return writeBuffer(val, val.computeChainDataLength());
  }

 private:
  template <typename Buffer>
  uint32_t writeBuffer(const Buffer& buffer) {
    return writeBuffer(buffer, buffer.size());
  }

  template <typename Buffer>
  uint32_t writeBuffer(const Buffer& buffer, size_t size) {
    uint32_t limit = std::numeric_limits<uint32_t>::max();
    if (size > limit) {
      protocol::TProtocolException::throwExceededSizeLimit(size, limit);
    }
    acc_.beginOrdered();
    writeI32(static_cast<int32_t>(size));
    acc_.combine(buffer);
    acc_.endOrdered();
    return 0;
  }

  template <typename... Types>
  void beginContainer(uint32_t size, Types... types) {
    acc_.beginOrdered();
    (writeByte(static_cast<int8_t>(types)), ...);
    writeI32(static_cast<int32_t>(size));
  }

  uint32_t endContainer() { return (acc_.endOrdered(), 0); }

  Accumulator& acc_;
};

template <typename Accumulator>
uint32_t HashProtocol<Accumulator>::writeFieldBegin(
    const char*, protocol::TType type, int16_t id) {
  acc_.beginOrdered();
  writeByte(static_cast<int8_t>(type));
  writeI16(id);
  return 0;
}

} // namespace apache::thrift::op::detail
