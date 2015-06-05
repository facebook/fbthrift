/*
 * Copyright 2015 Facebook, Inc.
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

#include <thrift/lib/cpp2/Thrift.h>
#include <type_traits>

namespace apache { namespace thrift {

namespace detail {
template <int N, int Size, class F, class Tuple>
struct ForEachImpl {
  static uint32_t forEach(Tuple&& tuple, F&& f) {
    uint32_t res = f(std::get<N>(tuple), N);
    res += ForEachImpl<N+1, Size, F, Tuple>::
        forEach(std::forward<Tuple>(tuple), std::forward<F>(f));
    return res;
  }
};
template <int Size, class F, class Tuple>
struct ForEachImpl<Size, Size, F, Tuple> {
  static uint32_t forEach(Tuple&& tuple, F&& f) {
    return 0;
  }
};

template <int N=0, class F, class Tuple>
uint32_t forEach(Tuple&& tuple, F&& f) {
  return ForEachImpl<N, std::tuple_size<
      typename std::remove_reference<Tuple>::type>::value, F, Tuple>::
      forEach(std::forward<Tuple>(tuple), std::forward<F>(f));
}

template <typename Protocol>
struct Writer {
  Writer(Protocol* prot, const bool* isset) : prot_(prot), isset_(isset) {}
  template <typename FieldData>
  uint32_t operator()(const FieldData& fieldData, int index) {
    if (!isset_[index]) {
      return 0;
    }

    int16_t fid = FieldData::fid;
    const auto& ex = fieldData.ref();

    uint32_t xfer = 0;
    xfer += prot_->writeFieldBegin("", Cpp2Ops<typename FieldData::ref_type>::thriftType(), fid);
    xfer += Cpp2Ops<typename FieldData::ref_type>::write(prot_, &ex);
    xfer += prot_->writeFieldEnd();
    return xfer;
  }
 private:
  Protocol* prot_;
  const bool* isset_;
};

template <typename Protocol>
struct Sizer {
  Sizer(Protocol* prot, const bool* isset) : prot_(prot), isset_(isset) {}
  template <typename FieldData>
  uint32_t operator()(const FieldData& fieldData, int index) {
    if (!isset_[index]) {
      return 0;
    }

    int16_t fid = FieldData::fid;
    const auto& ex = fieldData.ref();

    uint32_t xfer = 0;
    xfer += prot_->serializedFieldSize("", Cpp2Ops<typename FieldData::ref_type>::thriftType(), fid);
    xfer += Cpp2Ops<typename FieldData::ref_type>::serializedSize(prot_, &ex);
    return xfer;
  }
 private:
  Protocol* prot_;
  const bool* isset_;
};

template <typename Protocol>
struct SizerZC {
  SizerZC(Protocol* prot, const bool* isset) : prot_(prot), isset_(isset) {}
  template <typename FieldData>
  uint32_t operator()(const FieldData& fieldData, int index) {
    if (!isset_[index]) {
      return 0;
    }

    int16_t fid = FieldData::fid;
    const auto& ex = fieldData.ref();

    uint32_t xfer = 0;
    xfer += prot_->serializedFieldSize("", Cpp2Ops<typename FieldData::ref_type>::thriftType(), fid);
    xfer += Cpp2Ops<typename FieldData::ref_type>::serializedSizeZC(prot_, &ex);
    return xfer;
  }
 private:
  Protocol* prot_;
  const bool* isset_;
};

template <typename Protocol>
struct Reader {
  Reader(Protocol* prot, bool* isset, int16_t fid, protocol::TType ftype, bool& success)
    : prot_(prot), isset_(isset), fid_(fid), ftype_(ftype), success_(success)
  {}
  template <typename FieldData>
  uint32_t operator()(FieldData& fieldData, int index) {
    if (ftype_ != Cpp2Ops<typename FieldData::ref_type>::thriftType()) {
      return 0;
    }

    int16_t myfid = FieldData::fid;
    auto& ex = fieldData.ref();
    if (myfid != fid_) {
      return 0;
    }

    success_ = true;
    isset_[index] = true;
    return Cpp2Ops<typename FieldData::ref_type>::read(prot_, &ex);
  }
 private:
  Protocol* prot_;
  bool* isset_;
  int16_t fid_;
  protocol::TType ftype_;
  bool& success_;
};

template <typename T>
T& maybe_remove_pointer(T& x) { return x; }

template <typename T>
T& maybe_remove_pointer(T* x) { return *x; }

template <class T, class NameGetter>
class has_member_impl {
    template <class U> static char test(typename NameGetter::template get<U>*);
    template <class U> static long test(...);
public:
    enum { value = sizeof(test<T>(nullptr)) == sizeof(char) };
};

template <class T, class NameGetter>
struct has_member : std::integral_constant<bool,
                        has_member_impl<T, NameGetter>::value> {};

struct push_back_checker {
  template <class T, void (T::*)(typename T::value_type&&) = &T::push_back>
  struct get {};
};

struct insert_checker {
  template <class T, std::pair<typename T::iterator,bool>
                     (T::*)(typename T::value_type&&) = &T::insert>
  struct get {};
};

template <class C>
using is_vector_like = has_member<C, push_back_checker>;

template <class C>
using is_set_like = std::integral_constant<bool,
      has_member<C, insert_checker>::value &&
      std::is_same<typename C::key_type, typename C::value_type>::value>;

template <class C>
using is_map_like = std::integral_constant<bool,
      has_member<C, insert_checker>::value &&
      !std::is_same<typename C::key_type, typename C::value_type>::value>;

struct resize_checker {
  template <class T, void (T::*)(typename T::size_type) = &T::resize>
  struct get {};
};

struct op_index_checker {
  template <class T, typename T::reference
                     (T::*)(typename T::size_type) = &T::operator[]>
  struct get {};
};

template <class T, class = void>
struct Reserver {
  static void reserve(T& container, uint32_t size) {}
};

struct reserve_checker {
  template <class T, void (T::*)(typename T::size_type) = &T::reserve>
  struct get {};
};

template <class T>
struct Reserver<T, typename std::enable_if<has_member<T, reserve_checker>::value>::type> {
  static void reserve(T& container, uint32_t size) {
    container.reserve(size);
  }
};

}

template <int16_t Fid, protocol::TType Ttype, typename T>
struct FieldData {
  static const constexpr int16_t fid = Fid;
  static const constexpr protocol::TType ttype = Ttype;
  typedef T type;
  typedef typename std::remove_pointer<T>::type ref_type;
  T value;
  ref_type& ref() { return detail::maybe_remove_pointer(value); }
  const ref_type& ref() const { return detail::maybe_remove_pointer(value); }
};

template <typename... Field>
class ThriftPresult {
 public:
  std::tuple<Field...> fields;
  bool isset[sizeof...(Field)] = {};

  template <class Protocol>
  uint32_t read(Protocol* prot) {
    uint32_t xfer = 0;
    std::string fname;
    apache::thrift::protocol::TType ftype;
    int16_t fid;

    xfer += prot->readStructBegin(fname);

    while (true) {
      xfer += prot->readFieldBegin(fname, ftype, fid);
      if (ftype == apache::thrift::protocol::T_STOP) {
        break;
      }
      bool readSomething = false;
      xfer += detail::forEach(fields, detail::Reader<Protocol>(
          prot, isset, fid, ftype, readSomething));
      if (!readSomething) {
        xfer += prot->skip(ftype);
      }
      xfer += prot->readFieldEnd();
    }
    xfer += prot->readStructEnd();

    return xfer;
  }

  template <class Protocol>
  uint32_t serializedSize(Protocol* prot) const {
    uint32_t xfer = 0;
    xfer += prot->serializedStructSize("");
    xfer += detail::forEach(fields, detail::Sizer<Protocol>(prot, isset));
    xfer += prot->serializedSizeStop();
    return xfer;
  }

  template <class Protocol>
  uint32_t serializedSizeZC(Protocol* prot) const {
    uint32_t xfer = 0;
    xfer += prot->serializedStructSize("");
    xfer += detail::forEach(fields, detail::SizerZC<Protocol>(prot, isset));
    xfer += prot->serializedSizeStop();
    return xfer;
  }

  template <class Protocol>
  uint32_t write(Protocol* prot) const {
    uint32_t xfer = 0;
    xfer += prot->writeStructBegin("");
    xfer += detail::forEach(fields, detail::Writer<Protocol>(prot, isset));
    xfer += prot->writeFieldStop();
    xfer += prot->writeStructEnd();
    return xfer;
  }
};

template <class... Args>
class Cpp2Ops<ThriftPresult<Args...>> {
 public:
  typedef ThriftPresult<Args...> Presult;
  static constexpr protocol::TType thriftType() {
    return protocol::T_STRUCT;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Presult* value) {
    return value->write(prot);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Presult* value) {
    return value->read(prot);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Presult* value) {
    return value->serializedSize(prot);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Presult* value) {
    return value->serializedSizeZC(prot);
  }
};

template <>
class Cpp2Ops<folly::fbstring> {
 public:
  typedef folly::fbstring Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_STRING;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeString(*value);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readString(*value);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeString(*value);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeString(*value);
  }
};

template <>
class Cpp2Ops<std::string> {
 public:
  typedef std::string Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_STRING;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeString(*value);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readString(*value);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeString(*value);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeString(*value);
  }
};

template <>
class Cpp2Ops<int8_t> {
 public:
  typedef int8_t Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_BYTE;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeByte(*value);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readByte(*value);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeByte(*value);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeByte(*value);
  }
};

template <>
class Cpp2Ops<int16_t> {
 public:
  typedef int16_t Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_I16;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeI16(*value);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readI16(*value);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeI16(*value);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeI16(*value);
  }
};

template <>
class Cpp2Ops<int32_t> {
 public:
  typedef int32_t Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_I32;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeI32(*value);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readI32(*value);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeI32(*value);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeI32(*value);
  }
};

template <>
class Cpp2Ops<int64_t> {
 public:
  typedef int64_t Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_I64;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeI64(*value);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readI64(*value);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeI64(*value);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeI64(*value);
  }
};

template <>
class Cpp2Ops<bool> {
 public:
  typedef bool Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_BOOL;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeBool(*value);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readBool(*value);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeBool(*value);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeBool(*value);
  }
};

template <>
class Cpp2Ops<double> {
 public:
  typedef double Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_DOUBLE;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeDouble(*value);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readDouble(*value);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeDouble(*value);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeDouble(*value);
  }
};

template <class E>
class Cpp2Ops<E, typename std::enable_if<std::is_enum<E>::value>::type> {
 public:
  typedef E Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_I32;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeI32(static_cast<int32_t>(*value));
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readI32(reinterpret_cast<int32_t&>(*value));
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeI32(static_cast<int32_t>(*value));
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeI32(static_cast<int32_t>(*value));
  }
};

template <>
class Cpp2Ops<float> {
 public:
  typedef float Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_FLOAT;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeFloat(*value);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readFloat(*value);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeFloat(*value);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeFloat(*value);
  }
};


template <class L>
class Cpp2Ops<L, typename std::enable_if<detail::is_vector_like<L>::value>::type> {
 public:
  typedef L Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_LIST;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    typedef typename Type::value_type ElemType;
    uint32_t xfer = 0;
    xfer += prot->writeListBegin(Cpp2Ops<ElemType>::thriftType(), value->size());
    for (const auto& e: *value) {
      xfer += Cpp2Ops<ElemType>::write(prot, &e);
    }
    xfer += prot->writeListEnd();
    return xfer;
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    typedef typename Type::value_type ElemType;
    value->clear();
    uint32_t xfer = 0;
    uint32_t size;
    protocol::TType etype;
    xfer += prot->readListBegin(etype, size);
    value->resize(size);
    for (auto& e : *value) {
      xfer += Cpp2Ops<ElemType>::read(prot, &e);
    }
    xfer += prot->readListEnd();
    return xfer;
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    typedef typename Type::value_type ElemType;
    uint32_t xfer = 0;
    xfer += prot->serializedSizeListBegin(Cpp2Ops<ElemType>::thriftType(), value->size());
    for (const auto& e: *value) {
      xfer += Cpp2Ops<ElemType>::serializedSize(prot, &e);
    }
    xfer += prot->serializedSizeListEnd();
    return xfer;
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    typedef typename Type::value_type ElemType;
    uint32_t xfer = 0;
    xfer += prot->serializedSizeListBegin(Cpp2Ops<ElemType>::thriftType(), value->size());
    for (const auto& e: *value) {
      xfer += Cpp2Ops<ElemType>::serializedSizeZC(prot, &e);
    }
    xfer += prot->serializedSizeListEnd();
    return xfer;
  }
};

template <class S>
class Cpp2Ops<S, typename std::enable_if<detail::is_set_like<S>::value>::type> {
 public:
  typedef S Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_SET;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    typedef typename Type::key_type ElemType;
    uint32_t xfer = 0;
    xfer += prot->writeSetBegin(Cpp2Ops<ElemType>::thriftType(), value->size());
    for (const auto& e: *value) {
      xfer += Cpp2Ops<ElemType>::write(prot, &e);
    }
    xfer += prot->writeSetEnd();
    return xfer;
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    typedef typename Type::key_type ElemType;
    value->clear();
    uint32_t xfer = 0;
    uint32_t size;
    protocol::TType etype;
    xfer += prot->readSetBegin(etype, size);
    detail::Reserver<Type>::reserve(*value, size);
    for (uint32_t i = 0; i < size; i++) {
      ElemType elem;
      xfer += Cpp2Ops<ElemType>::read(prot, &elem);
      value->insert(std::move(elem));
    }
    xfer += prot->readSetEnd();
    return xfer;
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    typedef typename Type::key_type ElemType;
    uint32_t xfer = 0;
    xfer += prot->serializedSizeSetBegin(Cpp2Ops<ElemType>::thriftType(), value->size());
    for (const auto& e: *value) {
      xfer += Cpp2Ops<ElemType>::serializedSize(prot, &e);
    }
    xfer += prot->serializedSizeSetEnd();
    return xfer;
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    typedef typename Type::key_type ElemType;
    uint32_t xfer = 0;
    xfer += prot->serializedSizeSetBegin(Cpp2Ops<ElemType>::thriftType(), value->size());
    for (const auto& e: *value) {
      xfer += Cpp2Ops<ElemType>::serializedSizeZC(prot, &e);
    }
    xfer += prot->serializedSizeSetEnd();
    return xfer;
  }
};

template <class M>
class Cpp2Ops<M, typename std::enable_if<detail::is_map_like<M>::value>::type> {
 public:
  typedef M Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_MAP;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    typedef typename Type::key_type KeyType;
    typedef typename std::remove_cv<typename std::remove_reference<decltype(*value->begin())>::type>::type PairType;
    typedef typename PairType::second_type ValueType;
    uint32_t xfer = 0;
    xfer += prot->writeMapBegin(Cpp2Ops<KeyType>::thriftType(), Cpp2Ops<ValueType>::thriftType(), value->size());
    for (const auto& e: *value) {
      xfer += Cpp2Ops<KeyType>::write(prot, &e.first);
      xfer += Cpp2Ops<ValueType>::write(prot, &e.second);
    }
    xfer += prot->writeMapEnd();
    return xfer;
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    typedef typename Type::key_type KeyType;
    // We do this dance with decltype rather than just using Type::mapped_type
    // because different map implementations (such as Google's dense_hash_map)
    // call it data_type.
    typedef typename std::remove_cv<typename std::remove_reference<decltype(*value->begin())>::type>::type PairType;
    typedef typename PairType::second_type ValueType;
    value->clear();
    uint32_t xfer = 0;
    uint32_t size;
    protocol::TType keytype, valuetype;
    xfer += prot->readMapBegin(keytype, valuetype, size);
    detail::Reserver<Type>::reserve(*value, size);
    for (uint32_t i = 0; i < size; i++) {
      KeyType key;
      xfer += Cpp2Ops<KeyType>::read(prot, &key);
      auto& val = (*value)[std::move(key)];
      xfer += Cpp2Ops<ValueType>::read(prot, &val);
    }
    xfer += prot->readMapEnd();
    return xfer;
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    typedef typename Type::key_type KeyType;
    typedef typename std::remove_cv<typename std::remove_reference<decltype(*value->begin())>::type>::type PairType;
    typedef typename PairType::second_type ValueType;
    uint32_t xfer = 0;
    xfer += prot->serializedSizeMapBegin(Cpp2Ops<KeyType>::thriftType(), Cpp2Ops<ValueType>::thriftType(), value->size());
    for (const auto& e: *value) {
      xfer += Cpp2Ops<KeyType>::serializedSize(prot, &e.first);
      xfer += Cpp2Ops<ValueType>::serializedSize(prot, &e.second);
    }
    xfer += prot->serializedSizeMapEnd();
    return xfer;
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    typedef typename Type::key_type KeyType;
    typedef typename std::remove_cv<typename std::remove_reference<decltype(*value->begin())>::type>::type PairType;
    typedef typename PairType::second_type ValueType;
    uint32_t xfer = 0;
    xfer += prot->serializedSizeMapBegin(Cpp2Ops<KeyType>::thriftType(), Cpp2Ops<ValueType>::thriftType(), value->size());
    for (const auto& e: *value) {
      xfer += Cpp2Ops<KeyType>::serializedSizeZC(prot, &e.first);
      xfer += Cpp2Ops<ValueType>::serializedSizeZC(prot, &e.second);
    }
    xfer += prot->serializedSizeMapEnd();
    return xfer;
  }
};

template <>
class Cpp2Ops<folly::IOBuf> {
 public:
  typedef folly::IOBuf Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_STRING;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeBinary(*value);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readBinary(*value);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeBinary(*value);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeZCBinary(*value);
  }
};

template <>
class Cpp2Ops<std::unique_ptr<folly::IOBuf>> {
 public:
  typedef std::unique_ptr<folly::IOBuf> Type;
  static constexpr protocol::TType thriftType() {
    return protocol::T_STRING;
  }
  template <class Protocol>
  static uint32_t write(Protocol* prot, const Type* value) {
    return prot->writeBinary(*value);
  }
  template <class Protocol>
  static uint32_t read(Protocol* prot, Type* value) {
    return prot->readBinary(*value);
  }
  template <class Protocol>
  static uint32_t serializedSize(Protocol* prot, const Type* value) {
    return prot->serializedSizeBinary(*value);
  }
  template <class Protocol>
  static uint32_t serializedSizeZC(Protocol* prot, const Type* value) {
    return prot->serializedSizeZCBinary(*value);
  }
};

}} // apache::thrift
