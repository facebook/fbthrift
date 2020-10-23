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

#include "thrift/lib/cpp2/protocol/TableBasedSerializer.h"

#include "folly/CppAttributes.h"
#include "glog/logging.h"
#include "thrift/lib/cpp2/protocol/BinaryProtocol.h"
#include "thrift/lib/cpp2/protocol/CompactProtocol.h"
#include "thrift/lib/cpp2/protocol/ProtocolReaderStructReadState.h"
#include "thrift/lib/cpp2/protocol/ProtocolReaderWireTypeInfo.h"
#include "thrift/lib/cpp2/protocol/SimpleJSONProtocol.h"

namespace apache {
namespace thrift {
namespace detail {

#define THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(                          \
    TypeClass, Type, ThriftType, TTypeValue)                           \
  const TypeInfo TypeToInfo<type_class::TypeClass, Type>::typeInfo = { \
      protocol::TType::TTypeValue,                                     \
      reinterpret_cast<VoidFuncPtr>(identity(set<Type, ThriftType>)),  \
      reinterpret_cast<VoidFuncPtr>(identity(get<ThriftType, Type>)),  \
      nullptr,                                                         \
  }

// Specialization for numbers.
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::int8_t,
    std::int8_t,
    T_BYTE);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::int16_t,
    std::int16_t,
    T_I16);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::int32_t,
    std::int32_t,
    T_I32);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::int64_t,
    std::int64_t,
    T_I64);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::uint8_t,
    std::int8_t,
    T_BYTE);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::uint16_t,
    std::int16_t,
    T_I16);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::uint32_t,
    std::int32_t,
    T_I32);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(
    integral,
    std::uint64_t,
    std::int64_t,
    T_I64);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(integral, bool, bool, T_BOOL);

THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(floating_point, float, float, T_FLOAT);
THRIFT_DEFINE_PRIMITIVE_TYPE_TO_INFO(floating_point, double, double, T_DOUBLE);

// Specialization for string.
#define THRIFT_DEFINE_STRING_TYPE_TO_INFO(TypeClass, ActualType, ExtVal)     \
  const StringFieldType TypeToInfo<type_class::TypeClass, ActualType>::ext = \
      ExtVal;                                                                \
  const TypeInfo TypeToInfo<type_class::TypeClass, ActualType>::typeInfo = { \
      /* .type */ protocol::TType::T_STRING,                                 \
      /* .set */ nullptr,                                                    \
      /* .get */ nullptr,                                                    \
      /* .typeExt */ &ext,                                                   \
  }

THRIFT_DEFINE_STRING_TYPE_TO_INFO(string, std::string, StringFieldType::String);
THRIFT_DEFINE_STRING_TYPE_TO_INFO(
    string,
    folly::fbstring,
    StringFieldType::String);
THRIFT_DEFINE_STRING_TYPE_TO_INFO(binary, std::string, StringFieldType::String);
THRIFT_DEFINE_STRING_TYPE_TO_INFO(
    binary,
    folly::fbstring,
    StringFieldType::String);
THRIFT_DEFINE_STRING_TYPE_TO_INFO(binary, folly::IOBuf, StringFieldType::IOBuf);
THRIFT_DEFINE_STRING_TYPE_TO_INFO(
    binary,
    std::unique_ptr<folly::IOBuf>,
    StringFieldType::IOBufPtr);

namespace {
constexpr TypeInfo kStopType = {protocol::TType::T_STOP,
                                nullptr,
                                nullptr,
                                nullptr};
constexpr FieldInfo kStopMarker = {0, false, nullptr, 0, 0, &kStopType};

template <class Protocol_>
void skip(
    Protocol_* iprot,
    ProtocolReaderStructReadState<Protocol_>& readState) {
  readState.skip(iprot);
  readState.readFieldEnd(iprot);
  readState.readFieldBeginNoInline(iprot);
}

const void* getMember(const FieldInfo& fieldInfo, const void* object) {
  return static_cast<const char*>(object) + fieldInfo.memberOffset;
}

void* getMember(const FieldInfo& fieldInfo, void* object) {
  return static_cast<char*>(object) + fieldInfo.memberOffset;
}

const OptionalThriftValue getValue(
    const TypeInfo& typeInfo,
    const void* object) {
  if (typeInfo.get) {
    // Handle smart pointer and numerical types.
    return reinterpret_cast<OptionalThriftValue (*)(const void*)>(typeInfo.get)(
        object);
  }
  // Handle others.
  if (object) {
    return folly::make_optional<ThriftValue>(object);
  }
  return folly::none;
}

FOLLY_ERASE void* invokeSet(VoidFuncPtr set, void* object) {
  return reinterpret_cast<void* (*)(void*)>(set)(object);
}

template <class Protocol_>
const FieldInfo* FOLLY_NULLABLE findFieldInfo(
    Protocol_* iprot,
    ProtocolReaderStructReadState<Protocol_>& readState,
    const StructInfo& structInfo) {
  auto* end = structInfo.fieldInfos + structInfo.numFields;
  if (iprot->kUsesFieldNames()) {
    const FieldInfo* found =
        std::find_if(structInfo.fieldInfos, end, [&](const FieldInfo& val) {
          return val.name == readState.fieldName();
        });
    if (found != end) {
      readState.fieldId = found->id;
      readState.fieldType = found->typeInfo->type;
      if (readState.isCompatibleWithType(iprot, found->typeInfo->type)) {
        return found;
      }
    }
  } else {
    const FieldInfo* found = std::lower_bound(
        structInfo.fieldInfos,
        end,
        readState.fieldId,
        [](const FieldInfo& lhs, FieldID rhs) { return lhs.id < rhs; });
    if (found != end && found->id == readState.fieldId &&
        readState.isCompatibleWithType(iprot, found->typeInfo->type)) {
      return found;
    }
  }
  return nullptr;
}

const FieldID& activeUnionMemberId(const void* object, ptrdiff_t offset) {
  return *reinterpret_cast<const FieldID*>(
      offset + static_cast<const char*>(object));
}

const bool& fieldIsSet(const void* object, ptrdiff_t offset) {
  return *reinterpret_cast<const bool*>(
      offset + static_cast<const char*>(object));
}

template <class Protocol_>
void read(
    Protocol_* iprot,
    const TypeInfo& typeInfo,
    ProtocolReaderStructReadState<Protocol_>& readState,
    void* object) {
  using WireTypeInfo = ProtocolReaderWireTypeInfo<Protocol_>;
  using WireType = typename WireTypeInfo::WireType;
  switch (typeInfo.type) {
    case protocol::TType::T_STRUCT:
      readState.beforeSubobject(iprot);
      read<Protocol_>(
          iprot,
          *static_cast<const StructInfo*>(typeInfo.typeExt),
          typeInfo.set ? invokeSet(typeInfo.set, object) : object);
      readState.afterSubobject(iprot);
      break;
    case protocol::TType::T_I64: {
      std::int64_t temp;
      iprot->readI64(temp);
      reinterpret_cast<void (*)(void*, std::int64_t)>(typeInfo.set)(
          object, temp);
      break;
    }
    case protocol::TType::T_I32: {
      std::int32_t temp;
      iprot->readI32(temp);
      reinterpret_cast<void (*)(void*, std::int32_t)>(typeInfo.set)(
          object, temp);
      break;
    }
    case protocol::TType::T_I16: {
      std::int16_t temp;
      iprot->readI16(temp);
      reinterpret_cast<void (*)(void*, std::int16_t)>(typeInfo.set)(
          object, temp);
      break;
    }
    case protocol::TType::T_BYTE: {
      std::int8_t temp;
      iprot->readByte(temp);
      reinterpret_cast<void (*)(void*, std::int8_t)>(typeInfo.set)(
          object, temp);
      break;
    }
    case protocol::TType::T_BOOL: {
      bool temp;
      iprot->readBool(temp);
      reinterpret_cast<void (*)(void*, bool)>(typeInfo.set)(object, temp);
      break;
    }
    case protocol::TType::T_DOUBLE: {
      double temp;
      iprot->readDouble(temp);
      reinterpret_cast<void (*)(void*, double)>(typeInfo.set)(object, temp);
      break;
    }
    case protocol::TType::T_FLOAT: {
      float temp;
      iprot->readFloat(temp);
      reinterpret_cast<void (*)(void*, float)>(typeInfo.set)(object, temp);
      break;
    }
    case protocol::TType::T_STRING: {
      switch (*static_cast<const StringFieldType*>(typeInfo.typeExt)) {
        case StringFieldType::String:
          iprot->readString(*static_cast<std::string*>(object));
          break;
        case StringFieldType::IOBuf:
          iprot->readBinary(*static_cast<folly::IOBuf*>(object));
          break;
        case StringFieldType::IOBufPtr:
          iprot->readBinary(
              *static_cast<std::unique_ptr<folly::IOBuf>*>(object));
          break;
      }
      break;
    }
    case protocol::TType::T_MAP: {
      readState.beforeSubobject(iprot);
      // Initialize the container to clear out current values.
      auto* actualObject = invokeSet(typeInfo.set, object);
      const MapFieldExt& ext =
          *static_cast<const MapFieldExt*>(typeInfo.typeExt);
      std::uint32_t size = ~0;
      WireType reportedKeyType = WireTypeInfo::defaultValue();
      WireType reportedMappedType = WireTypeInfo::defaultValue();
      iprot->readMapBegin(reportedKeyType, reportedMappedType, size);
      struct Context {
        const TypeInfo* keyInfo;
        const TypeInfo* valInfo;
        Protocol_* iprot;
        ProtocolReaderStructReadState<Protocol_>& readState;
      };
      const Context context = {
          ext.keyInfo,
          ext.valInfo,
          iprot,
          readState,
      };
      auto const keyReader = [](const void* context, void* key) {
        const auto& typedContext = *static_cast<const Context*>(context);
        read(
            typedContext.iprot,
            *typedContext.keyInfo,
            typedContext.readState,
            key);
      };
      auto const valueReader = [](const void* context, void* val) {
        const auto& typedContext = *static_cast<const Context*>(context);
        read(
            typedContext.iprot,
            *typedContext.valInfo,
            typedContext.readState,
            val);
      };
      if (iprot->kOmitsContainerSizes()) {
        while (iprot->peekMap()) {
          ext.consumeElem(&context, actualObject, keyReader, valueReader);
        }
      } else {
        if (size > 0 &&
            (ext.keyInfo->type != reportedKeyType ||
             ext.valInfo->type != reportedMappedType)) {
          skip_n(*iprot, size, {reportedKeyType, reportedMappedType});
        } else {
          if (!canReadNElements(
                  *iprot, size, {reportedKeyType, reportedMappedType})) {
            protocol::TProtocolException::throwTruncatedData();
          }
          ext.readMap(&context, actualObject, size, keyReader, valueReader);
        }
      }
      iprot->readMapEnd();
      readState.afterSubobject(iprot);
      break;
    }
    case protocol::TType::T_SET: {
      readState.beforeSubobject(iprot);
      // Initialize the container to clear out current values.
      auto* actualObject = invokeSet(typeInfo.set, object);
      const SetFieldExt& ext =
          *static_cast<const SetFieldExt*>(typeInfo.typeExt);
      std::uint32_t size = ~0;
      WireType reportedType = WireTypeInfo::defaultValue();
      iprot->readSetBegin(reportedType, size);
      struct Context {
        const TypeInfo* valInfo;
        Protocol_* iprot;
        ProtocolReaderStructReadState<Protocol_>& readState;
      };
      const Context context = {
          ext.valInfo,
          iprot,
          readState,
      };
      auto const reader = [](const void* context, void* value) {
        const auto& typedContext = *static_cast<const Context*>(context);
        read(
            typedContext.iprot,
            *typedContext.valInfo,
            typedContext.readState,
            value);
      };
      if (iprot->kOmitsContainerSizes()) {
        while (iprot->peekSet()) {
          ext.consumeElem(&context, actualObject, reader);
        }
      } else {
        if (reportedType != ext.valInfo->type) {
          skip_n(*iprot, size, {reportedType});
        } else {
          if (!canReadNElements(*iprot, size, {reportedType})) {
            protocol::TProtocolException::throwTruncatedData();
          }
          ext.readSet(&context, actualObject, size, reader);
        }
      }
      iprot->readSetEnd();
      readState.afterSubobject(iprot);
      break;
    }
    case protocol::TType::T_LIST: {
      readState.beforeSubobject(iprot);
      // Initialize the container to clear out current values.
      auto* actualObject = invokeSet(typeInfo.set, object);
      const ListFieldExt& ext =
          *static_cast<const ListFieldExt*>(typeInfo.typeExt);
      std::uint32_t size = ~0;
      WireType reportedType = WireTypeInfo::defaultValue();

      iprot->readListBegin(reportedType, size);
      struct Context {
        const TypeInfo* valInfo;
        Protocol_* iprot;
        ProtocolReaderStructReadState<Protocol_>& readState;
      };
      const Context context = {
          ext.valInfo,
          iprot,
          readState,
      };
      auto const reader = [](const void* context, void* value) {
        const auto& typedContext = *static_cast<const Context*>(context);
        read(
            typedContext.iprot,
            *typedContext.valInfo,
            typedContext.readState,
            value);
      };
      if (iprot->kOmitsContainerSizes()) {
        while (iprot->peekList()) {
          ext.consumeElem(&context, actualObject, reader);
        }
      } else {
        if (reportedType != ext.valInfo->type) {
          skip_n(*iprot, size, {reportedType});
        } else {
          if (!canReadNElements(*iprot, size, {reportedType})) {
            protocol::TProtocolException::throwTruncatedData();
          }
          ext.readList(&context, actualObject, size, reader);
        }
      }
      iprot->readListEnd();
      readState.afterSubobject(iprot);
      break;
    }
    case protocol::TType::T_STOP:
    case protocol::TType::T_VOID:
    case protocol::TType::T_UTF8:
    case protocol::TType::T_U64:
    case protocol::TType::T_UTF16:
    case protocol::TType::T_STREAM:
      skip(iprot, readState);
  }
}

template <class Protocol_>
size_t write(Protocol_* iprot, const TypeInfo& typeInfo, ThriftValue value) {
  switch (typeInfo.type) {
    case protocol::TType::T_STRUCT:
      return write(
          iprot,
          *static_cast<const StructInfo*>(typeInfo.typeExt),
          value.object);
    case protocol::TType::T_I64:
      return iprot->writeI64(value.int64Value);
    case protocol::TType::T_I32:
      return iprot->writeI32(value.int32Value);
    case protocol::TType::T_I16:
      return iprot->writeI16(value.int16Value);
    case protocol::TType::T_BYTE:
      return iprot->writeByte(value.int8Value);
    case protocol::TType::T_BOOL:
      return iprot->writeBool(value.boolValue);
    case protocol::TType::T_DOUBLE:
      return iprot->writeDouble(value.doubleValue);
    case protocol::TType::T_FLOAT:
      return iprot->writeFloat(value.floatValue);
    case protocol::TType::T_STRING: {
      switch (*static_cast<const StringFieldType*>(typeInfo.typeExt)) {
        case StringFieldType::String:
          return iprot->writeString(
              *static_cast<const std::string*>(value.object));
        case StringFieldType::IOBuf:
          return iprot->writeBinary(
              *static_cast<const folly::IOBuf*>(value.object));
        case StringFieldType::IOBufPtr:
          return iprot->writeBinary(
              *static_cast<const std::unique_ptr<folly::IOBuf>*>(value.object));
      };
    }
      // For container types, when recursively writing with lambdas we
      // intentionally skip checking OptionalThriftValue.hasValue and treat it
      // as a user error if the value is a nullptr.
    case protocol::TType::T_MAP: {
      const auto& ext = *static_cast<const MapFieldExt*>(typeInfo.typeExt);
      size_t written = iprot->writeMapBegin(
          ext.keyInfo->type, ext.valInfo->type, ext.size(value.object));

      struct Context {
        const TypeInfo* keyInfo;
        const TypeInfo* valInfo;
        Protocol_* iprot;
      };
      const Context context = {
          ext.keyInfo,
          ext.valInfo,
          iprot,
      };
      written += ext.writeMap(
          &context,
          value.object,
          iprot->kSortKeys(),
          [](const void* context, const void* key, const void* val) {
            const auto& typedContext = *static_cast<const Context*>(context);
            const TypeInfo& keyInfo = *typedContext.keyInfo;
            const TypeInfo& valInfo = *typedContext.valInfo;
            return write(typedContext.iprot, keyInfo, *getValue(keyInfo, key)) +
                write(typedContext.iprot,
                      *typedContext.valInfo,
                      *getValue(valInfo, val));
          });
      written += iprot->writeMapEnd();
      return written;
    }
    case protocol::TType::T_SET: {
      const auto& ext = *static_cast<const SetFieldExt*>(typeInfo.typeExt);
      size_t written =
          iprot->writeSetBegin(ext.valInfo->type, ext.size(value.object));

      struct Context {
        const TypeInfo* valInfo;
        Protocol_* iprot;
      };
      const Context context = {
          ext.valInfo,
          iprot,
      };
      written += ext.writeSet(
          &context,
          value.object,
          iprot->kSortKeys(),
          [](const void* context, const void* value) {
            const auto& typedContext = *static_cast<const Context*>(context);
            const TypeInfo& valInfo = *typedContext.valInfo;
            return write(
                typedContext.iprot, valInfo, *getValue(valInfo, value));
          });
      written += iprot->writeSetEnd();
      return written;
    }
    case protocol::TType::T_LIST: {
      const auto& ext = *static_cast<const ListFieldExt*>(typeInfo.typeExt);
      size_t written =
          iprot->writeListBegin(ext.valInfo->type, ext.size(value.object));

      struct Context {
        const TypeInfo* valInfo;
        Protocol_* iprot;
      };
      const Context context = {
          ext.valInfo,
          iprot,
      };
      written += ext.writeList(
          &context, value.object, [](const void* context, const void* value) {
            const auto& typedContext = *static_cast<const Context*>(context);
            const TypeInfo& valInfo = *typedContext.valInfo;
            return write(
                typedContext.iprot, valInfo, *getValue(valInfo, value));
          });
      written += iprot->writeListEnd();
      return written;
    }
    case protocol::TType::T_STOP:
    case protocol::TType::T_VOID:
    case protocol::TType::T_STREAM:
    case protocol::TType::T_UTF8:
    case protocol::TType::T_U64:
    case protocol::TType::T_UTF16:
      DCHECK(false);
      break;
  }
  return 0;
}

template <class Protocol_>
size_t writeField(
    Protocol_* iprot,
    const FieldInfo& fieldInfo,
    const ThriftValue& value) {
  size_t written = iprot->writeFieldBegin(
      fieldInfo.name, fieldInfo.typeInfo->type, fieldInfo.id);
  written += write(iprot, *fieldInfo.typeInfo, value);
  written += iprot->writeFieldEnd();
  return written;
}
} // namespace

template <class Protocol_>
void read(Protocol_* iprot, const StructInfo& structInfo, void* object) {
  DCHECK(object);
  ProtocolReaderStructReadState<Protocol_> readState;
  readState.readStructBegin(iprot);

  if (UNLIKELY(structInfo.unionExt != nullptr)) {
    readState.fieldId = 0;
    readState.readFieldBegin(iprot);
    if (readState.atStop()) {
      structInfo.unionExt->clear(object);
      readState.readStructEnd(iprot);
      return;
    }
    const auto* fieldInfo = findFieldInfo(iprot, readState, structInfo);
    // Found it.
    if (fieldInfo) {
      void* unionVal = getMember(*fieldInfo, object);
      // Default construct and placement new into the member union.
      structInfo.unionExt->initMember[fieldInfo - structInfo.fieldInfos](
          unionVal);
      read(iprot, *fieldInfo->typeInfo, readState, unionVal);
      const_cast<FieldID&>(activeUnionMemberId(
          object, structInfo.unionExt->unionTypeOffset)) = fieldInfo->id;
    } else {
      skip(iprot, readState);
    }
    readState.readFieldEnd(iprot);
    readState.readFieldBegin(iprot);
    if (UNLIKELY(!readState.atStop())) {
      TProtocolException::throwUnionMissingStop();
    }
    readState.readStructEnd(iprot);
    return;
  }

  // Define out of loop to call advanceToNextField after the loop ends.
  FieldID prevFieldId = 0;

  // The index of the expected field in the struct layout.
  std::int16_t index = 0;

  // Every time advanceToNextField reports a field mismatch, either because the
  // field is missing or if the serialized fields are not sorted (protocols
  // don't guarantee a specific field order), we search for the field info
  // matching the read bytes. Then we resume from the one past the found field
  // to reduce the number of scans we have to do if the fields are sorted which
  // is a common case. When we increment index past the number of fields we
  // utilize the same search logic with a field info of type TType::T_STOP.
  for (;; ++index) {
    auto* fieldInfo = index < structInfo.numFields
        ? &structInfo.fieldInfos[index]
        : &kStopMarker;
    // Try to match the next field in order against the current bytes.
    if (UNLIKELY(!readState.advanceToNextField(
            iprot, prevFieldId, fieldInfo->id, fieldInfo->typeInfo->type))) {
      // Loop to skip until we find a match for both field id/name and type.
      for (;;) {
        readState.afterAdvanceFailure(iprot);
        if (readState.atStop()) {
          // Already at stop, return immediately.
          readState.readStructEnd(iprot);
          return;
        }
        fieldInfo = findFieldInfo(iprot, readState, structInfo);
        // Found it.
        if (fieldInfo) {
          // Set the index to the field next in order to the found field.
          index = fieldInfo - structInfo.fieldInfos;
          break;
        }
        skip(iprot, readState);
      }
    } else if (UNLIKELY(index >= structInfo.numFields)) {
      // We are at stop and have tried all of the fields, so return.
      readState.readStructEnd(iprot);
      return;
    }
    // Id and type are what we expect, try read.
    prevFieldId = fieldInfo->id;
    read(iprot, *fieldInfo->typeInfo, readState, getMember(*fieldInfo, object));
    if (fieldInfo->issetOffset > 0) {
      const_cast<bool&>(fieldIsSet(object, fieldInfo->issetOffset)) = true;
    }
  }
}

template <class Protocol_>
size_t
write(Protocol_* iprot, const StructInfo& structInfo, const void* object) {
  DCHECK(object);
  size_t written = iprot->writeStructBegin(structInfo.name);
  if (UNLIKELY(structInfo.unionExt != nullptr)) {
    const FieldInfo* end = structInfo.fieldInfos + structInfo.numFields;
    const auto& unionId =
        activeUnionMemberId(object, structInfo.unionExt->unionTypeOffset);
    const FieldInfo* found = std::lower_bound(
        structInfo.fieldInfos,
        end,
        unionId,
        [](const FieldInfo& lhs, FieldID rhs) { return lhs.id < rhs; });
    if (found < end && found->id == unionId) {
      const OptionalThriftValue value = getValue(*found->typeInfo, object);
      if (value.hasValue()) {
        written += writeField(iprot, *found, value.value());
      } else if (found->typeInfo->type == protocol::TType::T_STRUCT) {
        written += iprot->writeFieldBegin(
            found->name, found->typeInfo->type, found->id);
        written += iprot->writeStructBegin(found->name);
        written += iprot->writeStructEnd();
        written += iprot->writeFieldStop();
        written += iprot->writeFieldEnd();
      }
    }
  } else {
    for (std::int16_t index = 0; index < structInfo.numFields; index++) {
      const auto& fieldInfo = structInfo.fieldInfos[index];
      if (fieldInfo.isUnqualified || fieldInfo.issetOffset == 0 ||
          fieldIsSet(object, fieldInfo.issetOffset)) {
        const OptionalThriftValue value =
            getValue(*fieldInfo.typeInfo, getMember(fieldInfo, object));
        if (value.hasValue()) {
          written += writeField(iprot, fieldInfo, value.value());
        }
      }
    }
  }

  written += iprot->writeFieldStop();
  written += iprot->writeStructEnd();
  return written;
}

template void read<CompactProtocolReader>(
    CompactProtocolReader* iprot,
    const StructInfo& structInfo,
    void* object);
template size_t write<CompactProtocolWriter>(
    CompactProtocolWriter* iprot,
    const StructInfo& structInfo,
    const void* object);
template void read<BinaryProtocolReader>(
    BinaryProtocolReader* iprot,
    const StructInfo& structInfo,
    void* object);
template size_t write<BinaryProtocolWriter>(
    BinaryProtocolWriter* iprot,
    const StructInfo& structInfo,
    const void* object);
template void read<SimpleJSONProtocolReader>(
    SimpleJSONProtocolReader* iprot,
    const StructInfo& structInfo,
    void* object);
template size_t write<SimpleJSONProtocolWriter>(
    SimpleJSONProtocolWriter* iprot,
    const StructInfo& structInfo,
    const void* object);
} // namespace detail
} // namespace thrift
} // namespace apache
