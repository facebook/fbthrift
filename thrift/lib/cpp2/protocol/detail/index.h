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

#include <folly/Optional.h>
#include <folly/Range.h>
#include <folly/Traits.h>
#include <folly/io/Cursor.h>
#include <folly/portability/GFlags.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/protocol/ProtocolReaderStructReadState.h>
#include <thrift/lib/cpp2/protocol/Traits.h>

DECLARE_bool(thrift_enable_lazy_deserialization);

namespace apache {
namespace thrift {
namespace detail {

/*
 * When index is enabled, we will inject following fields to thrift struct
 *
 * __fbthrift_index_offset:
 *   The size of all thrift fields except index
 *
 * __fbthrift_field_id_to_size:
 *   map<i16, i64> representing field id to field size
 *
 * During deserialization, we will use __fbthrift_index_offset to
 * extract __fbthrift_field_id_to_size before deserializating other fields.
 * Then use __fbthrift_field_id_to_size to skip fields efficiently.
 */

struct InternalField {
  const char* name;
  int16_t id;
  TType type;
};

// The reason this field is T_DOUBLE is because we will backfill it after
// serializing the whole struct, though we will only use the integer part.
constexpr auto kSizeField =
    InternalField{"__fbthrift_index_offset", -32768, TType::T_DOUBLE};

constexpr auto kIndexField =
    InternalField{"__fbthrift_field_id_to_size", -32767, TType::T_MAP};

struct DummyIndexWriter {
  DummyIndexWriter(void*, uint32_t&) {}

  void recordFieldStart() {}

  template <class TypeClass, class Type>
  void recordFieldEnd(TypeClass, Type&&, int16_t) {}

  void finalize() {}
};

// Utility class to inject index fields to serialized data
template <class Protocol>
class IndexWriterImpl {
 public:
  IndexWriterImpl(Protocol* prot, uint32_t& writtenBytes)
      : prot_(prot), writtenBytes_(writtenBytes) {
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

  void readFieldEnd(Protocol* iprot) {
    tryAdvanceIndex(Base::fieldId);
    Base::readFieldEnd(iprot);
  }

  FOLLY_ALWAYS_INLINE bool advanceToNextField(
      Protocol* iprot,
      int32_t currFieldId,
      int32_t nextFieldId,
      TType nextFieldType) {
    if (currFieldId == 0 && foundSizeField_) {
      currFieldId = detail::kSizeField.id;
    }
    tryAdvanceIndex(currFieldId);
    return Base::advanceToNextField(
        iprot, currFieldId, nextFieldId, nextFieldType);
  }

  FOLLY_ALWAYS_INLINE folly::Optional<folly::IOBuf> tryFastSkip(
      Protocol* iprot, int16_t id, TType type, bool fixedCostSkip) {
    if (!FLAGS_thrift_enable_lazy_deserialization) {
      return {};
    }

    if (fixedCostSkip) {
      return tryFastSkipImpl(iprot, [&] { iprot->skip(type); });
    }

    if (foundSizeField_ && currentIndexFieldId_ == id) {
      return tryFastSkipImpl(
          iprot, [&] { iprot->skipBytes(currentIndexFieldSize_); });
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
    indexReader_.setInput(std::move(structBegin));
    if (auto serializedDataSize = readIndexOffsetField(indexReader_)) {
      foundSizeField_ = true;
      if (!FLAGS_thrift_enable_lazy_deserialization) {
        return;
      }

      indexReader_.skipBytes(*serializedDataSize);
      if (auto count = readIndexField(indexReader_)) {
        indexFieldCount_ = *count;
        tryAdvanceIndex(0); // Read first (field id, size) pair
        return;
      }
    }
  }

  FOLLY_ALWAYS_INLINE void tryAdvanceIndex(int16_t id) {
    if (indexFieldCount_ > 0 && id == currentIndexFieldId_) {
      indexReader_.readI16(currentIndexFieldId_);
      indexReader_.readI64(currentIndexFieldSize_);
      --indexFieldCount_;
    }
  }

  static folly::Optional<int64_t> readIndexOffsetField(Protocol& p) {
    std::string name;
    TType fieldType;
    int16_t fieldId;

    p.readFieldBegin(name, fieldType, fieldId);
    if (fieldType != kSizeField.type || fieldId != kSizeField.id) {
      return {};
    }

    double indexOffset;
    p.readDouble(indexOffset);
    p.readFieldEnd();
    return indexOffset;
  }

  static folly::Optional<uint32_t> readIndexField(Protocol& p) {
    std::string name;
    TType fieldType;
    int16_t fieldId;
    p.readFieldBegin(name, fieldType, fieldId);
    if (fieldType != kIndexField.type) {
      return {};
    }

    TType key;
    TType value;
    uint32_t fieldCount;
    p.readMapBegin(key, value, fieldCount);
    if (key != TType::T_I16 || value != TType::T_I64) {
      return {};
    }
    return fieldCount;
  }

  Protocol indexReader_;
  bool foundSizeField_ = false;
  uint32_t indexFieldCount_ = 0;
  int16_t currentIndexFieldId_ = 0;
  int64_t currentIndexFieldSize_ = 0;
};

template <class Protocol>
using ProtocolReaderStructReadStateWithIndex = std::conditional_t<
    Protocol::ProtocolWriter::kHasIndexSupport(),
    ProtocolReaderStructReadStateWithIndexImpl<Protocol>,
    ProtocolReaderStructReadState<Protocol>>;

} // namespace detail
} // namespace thrift
} // namespace apache
