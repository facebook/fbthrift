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

#include <folly/MapUtil.h>
#include <folly/Optional.h>
#include <folly/Range.h>
#include <folly/Traits.h>
#include <folly/container/F14Map.h>
#include <folly/io/Cursor.h>
#include <folly/portability/GFlags.h>
#include <thrift/lib/cpp2/protocol/Cpp2Ops.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/protocol/ProtocolReaderStructReadState.h>
#include <thrift/lib/cpp2/protocol/Traits.h>
#include <thrift/lib/cpp2/protocol/detail/ReservedId.h>

DECLARE_bool(thrift_enable_lazy_deserialization);

namespace apache {
namespace thrift {
namespace detail {

/*
 * When index is enabled, we will inject following fields to thrift struct
 *
 * __fbthrift_random_number:
 *   Randomly generated 64 bits integer to validate index field
 *
 * __fbthrift_index_offset:
 *   The size of all thrift fields except index
 *
 * __fbthrift_field_id_to_size:
 *   map<i16, i64> representing field id to field size
 *
 * During deserialization, we will use __fbthrift_index_offset to
 * extract __fbthrift_field_id_to_size before deserializing other fields.
 * Then use __fbthrift_field_id_to_size to skip fields efficiently.
 */

struct InternalField {
  const char* name;
  int16_t id;
  TType type;
};

// This is randomly generated 64 bit integer for each thrift struct
// serialization. The same random number will be added to index field to
// validate whether we are using the correct index field
constexpr auto kExpectedRandomNumberField = InternalField{
    "__fbthrift_random_number",
    static_cast<int16_t>(ReservedId::kExpectedRandomNumber),
    TType::T_I64};

// This is the field id (key) of random number in index field.
// The id is different from random number field id to avoid confusion
// since the value is not size of field, but random number in map
constexpr int16_t kActualRandomNumberFieldId =
    static_cast<int16_t>(ReservedId::kActualRandomNumber);

// The reason this field is T_DOUBLE is because we will backfill it after
// serializing the whole struct, though we will only use the integer part.
constexpr auto kSizeField = InternalField{
    "__fbthrift_index_offset",
    static_cast<int16_t>(ReservedId::kOffset),
    TType::T_DOUBLE};

constexpr auto kIndexField = InternalField{
    "__fbthrift_field_id_to_size",
    static_cast<int16_t>(ReservedId::kIndex),
    TType::T_MAP};

// If user modified lazy fields in serialized data directly without
// deserialization, we should not use index, otherwise we might read the data
// incorrectly.
//
// To validate data integrity, we added checksum of lazy fields in serialized
// data, and stored it as value in index map with kXxh3Checksum as key.
//
// Note this field is not an actual field in serialized data.
// It's only a sentinel to check whether checksum exists.
//
// The reason we don't use actual field to store checksum is because
// user might modify the serialized data and delete checksum field,
// in which case we might still use index even data is tampered
constexpr int16_t kXxh3ChecksumFieldId =
    static_cast<int16_t>(ReservedId::kXxh3Checksum);

class Xxh3Hasher {
 public:
  Xxh3Hasher();
  ~Xxh3Hasher();
  void update(folly::io::Cursor);
  operator int64_t();

 private:
  void* state;
};

struct DummyIndexWriter {
  DummyIndexWriter(void*, uint32_t&, bool) {}

  void recordFieldStart() {}

  template <class TypeClass, class Type>
  void recordFieldEnd(TypeClass, Type&&, int16_t) {}

  void finalize() {}
};

int64_t random_64bits_integer();

// Utility class to inject index fields to serialized data
template <class Protocol>
class IndexWriterImpl {
 public:
  IndexWriterImpl(
      Protocol* prot, uint32_t& writtenBytes, bool writeValidationFields)
      : prot_(prot),
        writtenBytes_(writtenBytes),
        writeValidationFields_(writeValidationFields) {
    if (writeValidationFields_) {
      writeRandomNumberField();
    }
    writeIndexOffsetField();
  }

  void recordFieldStart() { fieldStart_ = writtenBytes_; }

  template <class TypeClass, class Type>
  void recordFieldEnd(TypeClass, Type&&, int16_t id) {
    if (!std::is_same<TypeClass, type_class::integral>{} &&
        !fixed_cost_skip_v<
            typename Protocol::ProtocolReader,
            TypeClass,
            folly::remove_cvref_t<Type>>) {
      fieldIdAndSize_.push_back({id, writtenBytes_ - fieldStart_});
    }
  }

  void finalize() {
    prot_->rewriteDouble(
        writtenBytes_ - sizeFieldEnd_, writtenBytes_ - indexOffsetLocation_);
    writeIndexField();
  }

 private:
  struct FieldIndex {
    int16_t fieldId;
    int64_t fieldSize;
  };

  void writeIndexOffsetField() {
    writtenBytes_ +=
        prot_->writeFieldBegin(kSizeField.name, kSizeField.type, kSizeField.id);
    indexOffsetLocation_ = writtenBytes_;
    writtenBytes_ += prot_->writeDouble(0);
    writtenBytes_ += prot_->writeFieldEnd();
    sizeFieldEnd_ = writtenBytes_;
  }

  void writeRandomNumberField() {
    const int64_t randomNumber = random_64bits_integer();
    writtenBytes_ += prot_->writeFieldBegin(
        kExpectedRandomNumberField.name,
        kExpectedRandomNumberField.type,
        kExpectedRandomNumberField.id);
    writtenBytes_ += prot_->writeI64(randomNumber);
    writtenBytes_ += prot_->writeFieldEnd();
    fieldIdAndSize_.push_back({kActualRandomNumberFieldId, randomNumber});
  }

  void writeIndexField() {
    writtenBytes_ += prot_->writeFieldBegin(
        kIndexField.name, kIndexField.type, kIndexField.id);
    writtenBytes_ += prot_->writeMapBegin(
        TType::T_I16, TType::T_I64, fieldIdAndSize_.size());
    for (auto index : fieldIdAndSize_) {
      writtenBytes_ += prot_->writeI16(index.fieldId);
      writtenBytes_ += prot_->writeI64(index.fieldSize);
    }
    writtenBytes_ += prot_->writeMapEnd();
    writtenBytes_ += prot_->writeFieldEnd();
  }

  Protocol* prot_;
  uint32_t& writtenBytes_;
  bool writeValidationFields_;
  uint32_t indexOffsetLocation_ = 0;
  uint32_t sizeFieldEnd_ = 0;
  uint32_t fieldStart_ = 0;
  std::vector<FieldIndex> fieldIdAndSize_;
};

template <class Protocol>
using IndexWriter = std::conditional_t<
    Protocol::kHasIndexSupport(),
    IndexWriterImpl<Protocol>,
    DummyIndexWriter>;

template <class Protocol>
class ProtocolReaderStructReadStateWithIndexImpl
    : public ProtocolReaderStructReadState<Protocol> {
 private:
  using Base = ProtocolReaderStructReadState<Protocol>;

 public:
  void readStructBegin(Protocol* iprot) {
    readStructBeginWithIndex(iprot->getCursor());
    Base::readStructBegin(iprot);
  }

  FOLLY_ALWAYS_INLINE folly::Optional<folly::IOBuf> tryFastSkip(
      Protocol* iprot, int16_t id, TType type, bool fixedCostSkip) {
    if (!FLAGS_thrift_enable_lazy_deserialization) {
      return {};
    }

    if (fixedCostSkip) {
      return tryFastSkipImpl(iprot, [&] { iprot->skip(type); });
    }

    if (auto p = folly::get_ptr(fieldIdToSize_, id)) {
      return tryFastSkipImpl(iprot, [&] { iprot->skipBytes(*p); });
    }

    return {};
  }

 private:
  template <class Skip>
  folly::IOBuf tryFastSkipImpl(Protocol* iprot, Skip skip) {
    auto cursor = iprot->getCursor();
    skip();
    folly::IOBuf buf;
    cursor.clone(buf, iprot->getCursor() - cursor);
    buf.makeManaged();
    return buf;
  }

  void readStructBeginWithIndex(folly::io::Cursor structBegin) {
    if (!FLAGS_thrift_enable_lazy_deserialization) {
      return;
    }

    Protocol indexReader;
    indexReader.setInput(std::move(structBegin));
    if (!readHeadField(indexReader)) {
      return;
    }

    indexReader.skipBytes(*indexOffset_);
    readIndexField(indexReader);
  }

  bool readHeadField(Protocol& p) {
    std::string name;
    TType fieldType;
    int16_t fieldId;

    p.readFieldBegin(name, fieldType, fieldId);

    if (fieldId == kExpectedRandomNumberField.id) {
      if (fieldType != kExpectedRandomNumberField.type) {
        return false;
      }
      p.readI64(randomNumber_.emplace());
      p.readFieldEnd();
      p.readFieldBegin(name, fieldType, fieldId);
    }

    if (fieldType != kSizeField.type || fieldId != kSizeField.id) {
      return false;
    }

    double indexOffset;
    p.readDouble(indexOffset);
    p.readFieldEnd();
    indexOffset_ = indexOffset;
    return true;
  }

  using FieldIdToSize = folly::F14FastMap<int16_t, int64_t>;

  void readIndexField(Protocol& p) {
    std::string name;
    TType fieldType;
    int16_t fieldId;
    p.readFieldBegin(name, fieldType, fieldId);
    if (fieldId != kIndexField.id || fieldType != kIndexField.type) {
      return;
    }

    Cpp2Ops<FieldIdToSize>::read(&p, &fieldIdToSize_);

    if (randomNumber_) {
      auto i = fieldIdToSize_.find(kActualRandomNumberFieldId);
      if (i == fieldIdToSize_.end() || i->second != randomNumber_) {
        fieldIdToSize_.clear();
      }
    }
  }

  FieldIdToSize fieldIdToSize_;
  folly::Optional<int64_t> indexOffset_;
  folly::Optional<int64_t> randomNumber_;
};

template <class Protocol>
using ProtocolReaderStructReadStateWithIndex = std::conditional_t<
    Protocol::ProtocolWriter::kHasIndexSupport(),
    ProtocolReaderStructReadStateWithIndexImpl<Protocol>,
    ProtocolReaderStructReadState<Protocol>>;

} // namespace detail
} // namespace thrift
} // namespace apache
