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
#include <memory>

#include <folly/Range.h>
#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp/protocol/TType.h>

namespace apache {
namespace thrift {
namespace hash {

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
template <typename Accumulator>
class DeterministicProtocol {
 public:
  inline explicit DeterministicProtocol() = default;
  inline explicit DeterministicProtocol(Accumulator accumulator);

  static constexpr bool kSortKeys();

  inline operator Accumulator&&() &&;
  auto getResult() && { return std::move(accumulator_).getResult(); }

  inline std::uint32_t writeStructBegin(const char* name);
  inline std::uint32_t writeStructEnd();
  inline std::uint32_t writeFieldBegin(
      const char* name, protocol::TType fieldType, std::int16_t fieldId);
  inline std::uint32_t writeFieldEnd();
  inline std::uint32_t writeFieldStop();
  inline std::uint32_t writeMapBegin(
      protocol::TType keyType, protocol::TType valType, std::uint32_t size);
  inline std::uint32_t writeMapValueBegin();
  inline std::uint32_t writeMapValueEnd();
  inline std::uint32_t writeMapEnd();
  inline std::uint32_t writeListBegin(
      protocol::TType elemType, std::uint32_t size);
  inline std::uint32_t writeListEnd();
  inline std::uint32_t writeSetBegin(
      protocol::TType elemType, std::uint32_t size);
  inline std::uint32_t writeSetEnd();
  inline std::uint32_t writeBool(bool value);
  inline std::uint32_t writeByte(std::int8_t value);
  inline std::uint32_t writeI16(std::int16_t value);
  inline std::uint32_t writeI32(std::int32_t value);
  inline std::uint32_t writeI64(std::int64_t value);
  inline std::uint32_t writeDouble(double value);
  inline std::uint32_t writeFloat(float value);
  inline std::uint32_t writeString(folly::StringPiece value);
  inline std::uint32_t writeBinary(folly::StringPiece value);
  inline std::uint32_t writeBinary(folly::ByteRange value);
  inline std::uint32_t writeBinary(const std::unique_ptr<folly::IOBuf>& value);
  inline std::uint32_t writeBinary(const folly::IOBuf& value);

 private:
  template <typename Buffer>
  inline void writeBuffer(const Buffer& buffer, std::size_t size);

  Accumulator accumulator_;
};

} // namespace hash
} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/hash/DeterministicProtocol-inl.h>
