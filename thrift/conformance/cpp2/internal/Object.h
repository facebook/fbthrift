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

#include <stack>
#include <type_traits>

#include <fatal/type/same_reference_as.h>
#include <folly/CPortability.h>
#include <thrift/conformance/cpp2/ThriftTypes.h>
#include <thrift/conformance/if/gen-cpp2/object_types.h>

namespace apache::thrift::conformance::detail {

template <typename C, typename T>
decltype(auto) forward_elem(T& elem) {
  return std::forward<typename fatal::same_reference_as<T, C>::type>(elem);
}

template <typename TT, typename = void>
struct ValueHelper;

template <>
struct ValueHelper<type::bool_t> {
  FOLLY_ERASE static void set(Value& result, bool value) {
    result.set_boolValue(value);
  }
};

template <>
struct ValueHelper<type::byte_t> {
  FOLLY_ERASE static void set(Value& result, int8_t value) {
    result.set_byteValue(value);
  }
};

template <>
struct ValueHelper<type::i16_t> {
  FOLLY_ERASE static void set(Value& result, int16_t value) {
    result.set_i16Value(value);
  }
};

template <>
struct ValueHelper<type::i32_t> {
  FOLLY_ERASE static void set(Value& result, int32_t value) {
    result.set_i32Value(value);
  }
};

template <>
struct ValueHelper<type::i64_t> {
  FOLLY_ERASE static void set(Value& result, int64_t value) {
    result.set_i64Value(value);
  }
};

template <typename ET>
struct ValueHelper<ET, if_base_type<ET, BaseType::Enum>> {
  template <typename E>
  FOLLY_ERASE static void set(Value& result, E value) {
    result.set_i32Value(static_cast<int32_t>(value));
  }
};

template <>
struct ValueHelper<type::float_t> {
  FOLLY_ERASE static void set(Value& result, float value) {
    result.set_floatValue(value);
  }
};

template <>
struct ValueHelper<type::double_t> {
  FOLLY_ERASE static void set(Value& result, float value) {
    result.set_doubleValue(value);
  }
};

template <>
struct ValueHelper<type::string_t> {
  FOLLY_ERASE static void set(Value& result, std::string value) {
    result.set_stringValue(std::move(value));
  }
  FOLLY_ERASE static void set(Value& result, const char* value) {
    set(result, std::string(value));
  }
  FOLLY_ERASE static void set(Value& result, folly::StringPiece value) {
    set(result, std::string(value));
  }
  FOLLY_ERASE static void set(Value& result, std::string_view value) {
    set(result, std::string(value));
  }
};

template <>
struct ValueHelper<type::binary_t> {
  FOLLY_ERASE static void set(Value& result, std::string value) {
    result.set_binaryValue(std::move(value));
  }
  FOLLY_ERASE static void set(Value& result, const char* value) {
    set(result, std::string(value));
  }
  FOLLY_ERASE static void set(Value& result, folly::ByteRange value) {
    set(result, std::string(value));
  }
  static void set(Value& result, const folly::IOBuf& value) {
    folly::IOBufQueue queue;
    queue.append(value);
    std::string strValue;
    queue.appendToString(strValue);
    set(result, std::move(strValue));
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
  static constexpr ProtocolType protocolType() {
    return {};
  }
  static constexpr bool kUsesFieldNames() {
    return true;
  }
  static constexpr bool kOmitsContainerSizes() {
    return false;
  }
  static constexpr bool kSortKeys() {
    return false;
  }
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
  uint32_t writeStructEnd() {
    return endValue(Value::objectValue);
  }

  uint32_t
  writeFieldBegin(const char* name, TType /*fieldType*/, int16_t /*fieldId*/) {
    auto result = cur(Value::objectValue)
                      .mutable_objectValue()
                      .members_ref()
                      ->emplace(name, Value());
    assert(result.second);
    cur_.push(&result.first->second);
    return 0;
  }

  uint32_t writeFieldEnd() {
    assert(!cur_.empty());
    cur_.pop();
    return 0;
  }

  uint32_t writeFieldStop() {
    return 0;
  }

  uint32_t
  writeMapBegin(const TType /*keyType*/, TType /*valType*/, uint32_t /*size*/) {
    beginValue().mapValue_ref().ensure();
    return 0;
  }

  uint32_t writeMapEnd() {
    return endValue(Value::mapValue);
  }

  uint32_t writeListBegin(TType /*elemType*/, uint32_t size) {
    beginValue().listValue_ref().ensure().reserve(size);
    return 0;
  }

  uint32_t writeListEnd() {
    return endValue(Value::listValue);
  }

  uint32_t writeSetBegin(TType /*elemType*/, uint32_t /*size*/) {
    beginValue().setValue_ref().ensure();
    return 0;
  }

  uint32_t writeSetEnd() {
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
    ValueHelper<type::string_t>::set(beginValue(), value);
    return endValue(Value::stringValue);
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
    return 0;
  }
};

// Specialization for all structured types.
template <typename TT>
struct ValueHelper<TT, if_structured<TT>> {
  template <typename T>
  static void set(Value& result, T&& value) {
    // TODO(afuller): Using the Visitor reflection API + ValueHelper instead.
    // This method loses type information (the enum or struct type for
    // example).
    ObjectWriter writer(&result);
    value.write(&writer);
  }
};

} // namespace apache::thrift::conformance::detail
