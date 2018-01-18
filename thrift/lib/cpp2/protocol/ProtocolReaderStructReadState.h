/*
 * Copyright 2018-present Facebook, Inc.
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

namespace apache {
namespace thrift {
namespace detail {

/**
 * Default stub implementation for StructReadState for protocols that don't
 * support it.
 */
template <class Protocol>
struct ProtocolReaderStructReadState {
  std::string fieldName_;
  int16_t fieldId;
  apache::thrift::protocol::TType fieldType;

  void readStructBegin(Protocol* iprot) {
    iprot->readStructBegin(fieldName_);
  }

  void readStructEnd(Protocol* iprot) {
    iprot->readStructEnd();
  }

  void readFieldBegin(Protocol* iprot) {
    iprot->readFieldBegin(fieldName_, fieldType, fieldId);
  }

  void readFieldBeginNoInline(Protocol* iprot) {
    iprot->readFieldBegin(fieldName_, fieldType, fieldId);
  }

  void readFieldEnd(Protocol* iprot) {
    iprot->readFieldEnd();
  }

  /**
   * Transition to parsing next field. Under the hood it handles
   * (readFieldEnd + readFieldBegin).
   *
   * The return value indicates if the next field on the wire is the expected
   * one (i.e. same field type and field id as nextFieldType and nextFieldId),
   * but may have false negatives.
   *
   * Note, on success (i.e. return value is true), the read state is unchanged.
   *
   * Note2, this function is for making transitions faster and is BEST EFFORT.
   * It may return `false` even if the next field is the expected one, but it's
   * guaranteed to never return `true` when the next field is not the expected
   * one.
   *
   * @return true  if the next field on the wire is the expected one.
   */
  FOLLY_ALWAYS_INLINE bool advanceToNextField(
      Protocol* iprot,
      int32_t currFieldId,
      int32_t /*nextFieldId*/,
      TType /*nextFieldType*/) {
    if (currFieldId != 0) {
      iprot->readFieldEnd();
    }
    iprot->readFieldBegin(fieldName_, fieldType, fieldId);
    return false;
  }

  std::string& fieldName() {
    return fieldName_;
  }
};

} // namespace detail
} // namespace thrift
} // namespace apache
