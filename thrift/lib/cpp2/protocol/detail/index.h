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

#include <folly/Range.h>
#include <folly/Traits.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/protocol/Traits.h>

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

} // namespace detail
} // namespace thrift
} // namespace apache
