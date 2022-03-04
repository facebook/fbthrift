/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <stack>
#include <type_traits>

#include <fatal/type/same_reference_as.h>
#include <folly/CPortability.h>
#include <thrift/conformance/if/gen-cpp2/object_types.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/cpp2/type/Traits.h>

namespace apache::thrift::conformance::detail {

template <typename C, typename T>
decltype(auto) forward_elem(T& elem) {
  return std::forward<typename fatal::same_reference_as<T, C>::type>(elem);
}

template <typename TT, typename = void>
struct ValueHelper {
  template <typename T>
  static void set(Value& result, T&& value) {
    if constexpr (false) {
    } else if constexpr (type::base_type_v<TT> == type::BaseType::Bool) {
      result.set_boolValue(value);
    } else if constexpr (type::base_type_v<TT> == type::BaseType::Byte) {
      result.set_byteValue(value);
    } else if constexpr (type::base_type_v<TT> == type::BaseType::I16) {
      result.set_i16Value(value);
    } else if constexpr (type::base_type_v<TT> == type::BaseType::I32) {
      result.set_i32Value(value);
    } else if constexpr (type::base_type_v<TT> == type::BaseType::I64) {
      result.set_i64Value(value);
    } else if constexpr (type::base_type_v<TT> == type::BaseType::Enum) {
      result.set_i32Value(static_cast<int32_t>(value));
    } else if constexpr (type::base_type_v<TT> == type::BaseType::Float) {
      result.set_floatValue(value);
    } else if constexpr (type::base_type_v<TT> == type::BaseType::Double) {
      result.set_doubleValue(value);
    } else if constexpr (type::base_type_v<TT> == type::BaseType::String) {
      result.set_stringValue(std::forward<T>(value));
    }
  }
};

template <>
struct ValueHelper<type::binary_t> {
  static void set(Value& result, folly::IOBuf value) {
    result.set_binaryValue(std::move(value));
  }
  static void set(Value& result, std::string_view value) {
    result.set_binaryValue(
        folly::IOBuf{folly::IOBuf::COPY_BUFFER, value.data(), value.size()});
  }
  static void set(Value& result, folly::ByteRange value) {
    result.set_binaryValue(
        folly::IOBuf{folly::IOBuf::COPY_BUFFER, value.data(), value.size()});
  }
};

template <typename V>
struct ValueHelper<type::list<V>> {
  template <typename C>
  static void set(Value& result, C&& value) {
    auto& result_list = result.listValue_ref().ensure();
    for (auto& elem : value) {
      ValueHelper<V>::set(result_list.emplace_back(), forward_elem<C>(elem));
    }
  }
};

template <typename V>
struct ValueHelper<type::set<V>> {
  template <typename C>
  static void set(Value& result, C&& value) {
    auto& result_set = result.setValue_ref().ensure();
    for (auto& elem : value) {
      Value elem_val;
      ValueHelper<V>::set(elem_val, forward_elem<C>(elem));
      result_set.emplace(std::move(elem_val));
    }
  }
};

template <typename K, typename V>
struct ValueHelper<type::map<K, V>> {
  template <typename C>
  static void set(Value& result, C&& value) {
    auto& result_map = result.mapValue_ref().ensure();
    for (auto& entry : value) {
      Value key;
      ValueHelper<K>::set(key, entry.first);
      ValueHelper<V>::set(result_map[key], forward_elem<C>(entry.second));
    }
  }
};

class BaseObjectAdapter {
 public:
  static constexpr ProtocolType protocolType() { return {}; }
  static constexpr bool kUsesFieldNames() { return true; }
  static constexpr bool kOmitsContainerSizes() { return false; }
  static constexpr bool kSortKeys() { return false; }
};

class ObjectWriter : public BaseObjectAdapter {
 public:
  ObjectWriter(Value* target) {
    assert(target != nullptr);
    cur_.emplace(target);
  }

  uint32_t writeStructBegin(const char* /*name*/) {
    beginValue().objectValue_ref().ensure();
    return 0;
  }
  uint32_t writeStructEnd() { return endValue(Value::objectValue); }

  uint32_t writeFieldBegin(
      const char* /*name*/, TType /*fieldType*/, int16_t fieldId) {
    auto result = cur(Value::objectValue)
                      .mutable_objectValue()
                      .members_ref()
                      ->emplace(fieldId, Value());
    assert(result.second);
    cur_.push(&result.first->second);
    return 0;
  }

  uint32_t writeFieldEnd() { return 0; }

  uint32_t writeFieldStop() { return 0; }

  uint32_t writeMapBegin(
      const TType /*keyType*/, TType /*valType*/, uint32_t size) {
    // We cannot push reference to map elements on stack without first inserting
    // map elements. So push reference to temporary buffer on stack instead.
    allocBufferPushOnStack((size_t)size * 2);
    return 0;
  }

  uint32_t writeMapEnd() {
    // insert elements from buffer into mapValue
    std::vector<Value> mapKeyAndValues = getBufferFromStack();
    assert(mapKeyAndValues.size() % 2 == 0);
    std::map<Value, Value>& mapVal = cur().mapValue_ref().ensure();
    for (size_t i = 0; i < mapKeyAndValues.size(); i += 2) {
      mapVal.emplace(
          std::move(mapKeyAndValues[i]), std::move(mapKeyAndValues[i + 1]));
    }
    return endValue(Value::mapValue);
  }

  uint32_t writeListBegin(TType /*elemType*/, uint32_t size) {
    allocBufferPushOnStack(size);
    return 0;
  }

  uint32_t writeListEnd() { return endValue(Value::listValue); }

  uint32_t writeSetBegin(TType /*elemType*/, uint32_t size) {
    // We cannot push reference to set elements on stack without first inserting
    // set elements. So push reference to temporary buffer on stack instead.
    allocBufferPushOnStack(size);
    return 0;
  }

  uint32_t writeSetEnd() {
    // insert elements from buffer into setValue
    std::vector<Value> setValues = getBufferFromStack();
    std::set<Value>& setVal = cur().setValue_ref().ensure();
    for (size_t i = 0; i < setValues.size(); i++) {
      setVal.emplace(std::move(setValues[i]));
    }
    return endValue(Value::setValue);
  }

  uint32_t writeBool(bool value) {
    ValueHelper<type::bool_t>::set(beginValue(), value);
    return endValue(Value::boolValue);
  }

  uint32_t writeByte(int8_t value) {
    ValueHelper<type::byte_t>::set(beginValue(), value);
    return endValue(Value::byteValue);
  }

  uint32_t writeI16(int16_t value) {
    ValueHelper<type::i16_t>::set(beginValue(), value);
    return endValue(Value::i16Value);
  }

  uint32_t writeI32(int32_t value) {
    ValueHelper<type::i32_t>::set(beginValue(), value);
    return endValue(Value::i32Value);
  }

  uint32_t writeI64(int64_t value) {
    ValueHelper<type::i64_t>::set(beginValue(), value);
    return endValue(Value::i64Value);
  }

  uint32_t writeFloat(float value) {
    ValueHelper<type::float_t>::set(beginValue(), value);
    return endValue(Value::floatValue);
  }

  int32_t writeDouble(double value) {
    ValueHelper<type::double_t>::set(beginValue(), value);
    return endValue(Value::doubleValue);
  }

  uint32_t writeString(folly::StringPiece value) {
    // TODO: set in stringValue if UTF8
    return writeBinary(value);
  }

  uint32_t writeBinary(folly::ByteRange value) {
    ValueHelper<type::binary_t>::set(beginValue(), value);
    return endValue(Value::binaryValue);
  }

  uint32_t writeBinary(const folly::IOBuf& value) {
    ValueHelper<type::binary_t>::set(beginValue(), value);
    return endValue(Value::binaryValue);
  }

  uint32_t writeBinary(const std::unique_ptr<folly::IOBuf>& str) {
    assert(str != nullptr);
    if (!str) {
      return 0;
    }
    return writeBinary(*str);
  }

  uint32_t writeBinary(folly::StringPiece value) {
    return writeBinary(folly::ByteRange(value));
  }

 protected:
  std::stack<Value*> cur_;

  void checkCur(Value::Type required) {
    (void)required;
    assert(cur().getType() == required);
  }

  Value& cur(Value::Type required) {
    checkCur(required);
    return *cur_.top();
  }

  Value& cur() {
    assert(!cur_.empty());
    return *cur_.top();
  }

  Value& beginValue() {
    checkCur(Value::__EMPTY__);
    return cur();
  }

  uint32_t endValue(Value::Type required) {
    checkCur(required);
    cur_.pop();
    return 0;
  }

  // Allocated temporary buffer in cur() and pushes buffer references on stack
  void allocBufferPushOnStack(size_t n) {
    // using listVal as temporary buffer
    std::vector<Value>& listVal = beginValue().listValue_ref().ensure();
    listVal.resize(n);
    for (auto itr = listVal.rbegin(); itr != listVal.rend(); ++itr) {
      cur_.push(&*itr);
    }
  }

  // Get temporary buffer from cur()
  std::vector<Value> getBufferFromStack() {
    return std::move(*cur(Value::listValue).listValue_ref());
  }
};

// Specialization for all structured types.
template <typename TT>
struct ValueHelper<TT, type::if_structured<TT>> {
  template <typename T>
  static void set(Value& result, T&& value) {
    // TODO(afuller): Using the Visitor reflection API + ValueHelper instead.
    // This method loses type information (the enum or struct type for
    // example).
    ObjectWriter writer(&result);
    value.write(&writer);
  }
};

// Schemaless deserialization of thrift serialized data of specified
// thrift type into conformance::Value
// Protocol: protocol to use eg. apache::thrift::BinaryProtocolReader
// TODO: handle jsonprotocol
template <class Protocol>
Value parseValue(Protocol& prot, TType arg_type) {
  Value result;
  switch (arg_type) {
    case protocol::T_BOOL: {
      bool boolv;
      prot.readBool(boolv);
      result.boolValue_ref() = boolv;
      return result;
    }
    case protocol::T_BYTE: {
      int8_t bytev = 0;
      prot.readByte(bytev);
      result.byteValue_ref() = bytev;
      return result;
    }
    case protocol::T_I16: {
      int16_t i16;
      prot.readI16(i16);
      result.i16Value_ref() = i16;
      return result;
    }
    case protocol::T_I32: {
      int32_t i32;
      prot.readI32(i32);
      result.i32Value_ref() = i32;
      return result;
    }
    case protocol::T_I64: {
      int64_t i64;
      prot.readI64(i64);
      result.i64Value_ref() = i64;
      return result;
    }
    case protocol::T_DOUBLE: {
      double dub;
      prot.readDouble(dub);
      result.doubleValue_ref() = dub;
      return result;
    }
    case protocol::T_FLOAT: {
      float flt;
      prot.readFloat(flt);
      result.floatValue_ref() = flt;
      return result;
    }
    case protocol::T_STRING: {
      auto& binaryValue = result.binaryValue_ref().ensure();
      prot.readBinary(binaryValue);
      return result;
    }
    case protocol::T_STRUCT: {
      std::string name;
      int16_t fid;
      TType ftype;
      auto& objectValue = result.objectValue_ref().ensure();
      prot.readStructBegin(name);
      while (true) {
        prot.readFieldBegin(name, ftype, fid);
        if (ftype == protocol::T_STOP) {
          break;
        }
        auto val = parseValue(prot, ftype);
        objectValue.members_ref()->emplace(fid, val);
        prot.readFieldEnd();
      }
      prot.readStructEnd();
      return result;
    }
    case protocol::T_MAP: {
      TType keyType;
      TType valType;
      uint32_t size;
      auto& mapValue = result.mapValue_ref().ensure();
      prot.readMapBegin(keyType, valType, size);
      for (uint32_t i = 0; i < size; i++) {
        auto key = parseValue(prot, keyType);
        auto val = parseValue(prot, valType);
        mapValue.emplace(key, val);
      }
      prot.readMapEnd();
      return result;
    }
    case protocol::T_SET: {
      TType elemType;
      uint32_t size;
      auto& setValue = result.setValue_ref().ensure();
      prot.readSetBegin(elemType, size);
      for (uint32_t i = 0; i < size; i++) {
        auto val = parseValue(prot, elemType);
        setValue.emplace(val);
      }
      prot.readSetEnd();
      return result;
    }
    case protocol::T_LIST: {
      TType elemType;
      uint32_t size;
      prot.readListBegin(elemType, size);
      auto& listValue = result.listValue_ref().ensure();
      for (uint32_t i = 0; i < size; i++) {
        auto val = parseValue(prot, elemType);
        listValue.emplace_back(val);
      }
      prot.readListEnd();
      return result;
    }
    default: {
      TProtocolException::throwInvalidSkipType(arg_type);
    }
  }
}

inline TType getTType(const Value& val) {
  return toTType(static_cast<type::BaseType>(val.getType()));
}

template <class Protocol>
void serializeValue(Protocol& prot, const Value& value) {
  switch (value.getType()) {
    case Value::Type::boolValue:
      prot.writeBool(*value.boolValue_ref());
      return;
    case Value::Type::byteValue:
      prot.writeByte(*value.byteValue_ref());
      return;
    case Value::Type::i16Value:
      prot.writeI16(*value.i16Value_ref());
      return;
    case Value::Type::i32Value:
      prot.writeI32(*value.i32Value_ref());
      return;
    case Value::Type::i64Value:
      prot.writeI64(*value.i64Value_ref());
      return;
    case Value::Type::floatValue:
      prot.writeFloat(*value.floatValue_ref());
      return;
    case Value::Type::doubleValue:
      prot.writeDouble(*value.doubleValue_ref());
      return;
    case Value::Type::stringValue:
      prot.writeString(*value.stringValue_ref());
      return;
    case Value::Type::binaryValue:
      prot.writeBinary(*value.binaryValue_ref());
      return;
    case Value::Type::listValue: {
      TType elemType = protocol::T_I64;
      uint32_t size = value.listValue_ref()->size();
      if (size > 0) {
        elemType = getTType(value.listValue_ref()->at(0));
      }
      prot.writeListBegin(elemType, size);
      for (uint32_t i = 0; i < size; i++) {
        serializeValue(prot, value.listValue_ref()->at(i));
      }
      prot.writeListEnd();
      return;
    }
    case Value::Type::mapValue: {
      TType keyType = protocol::T_STRING;
      TType valueType = protocol::T_I64;
      uint32_t size = value.mapValue_ref()->size();
      if (size > 0) {
        keyType = getTType(value.mapValue_ref()->begin()->first);
        valueType = getTType(value.mapValue_ref()->begin()->second);
      }
      prot.writeMapBegin(keyType, valueType, size);
      for (auto const& [key, val] : *value.mapValue_ref()) {
        serializeValue(prot, key);
        serializeValue(prot, val);
      }
      prot.writeMapEnd();
      return;
    }
    case Value::Type::setValue: {
      TType elemType = protocol::T_I64;
      uint32_t size = value.setValue_ref()->size();
      if (size > 0) {
        elemType = getTType(*value.setValue_ref()->begin());
      }
      prot.writeSetBegin(elemType, size);
      for (auto const& val : *value.setValue_ref()) {
        serializeValue(prot, val);
      }
      prot.writeSetEnd();
      return;
    }
    case Value::Type::objectValue: {
      prot.writeStructBegin("");
      for (auto const& [fieldID, fieldVal] :
           *value.objectValue_ref()->members_ref()) {
        auto fieldType = getTType(fieldVal);
        prot.writeFieldBegin("", fieldType, fieldID);
        serializeValue(prot, fieldVal);
        prot.writeFieldEnd();
      }
      prot.writeFieldStop();
      prot.writeStructEnd();
      return;
    }
    default: {
      TProtocolException::throwInvalidFieldData();
    }
  }
}

} // namespace apache::thrift::conformance::detail
