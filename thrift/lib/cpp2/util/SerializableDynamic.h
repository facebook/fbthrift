/*
 * Copyright 2014 Facebook, Inc.
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

#ifndef THRIFT_UTIL_SERIALIZABLEDYNAMIC_H
#define THRIFT_UTIL_SERIALIZABLEDYNAMIC_H

#include <folly/dynamic.h>

namespace apache { namespace thrift {

////////////////////////////////////////////////////////////////////////////////

namespace util {

/**
 *
 */
class SerializableDynamic {
 public:
  SerializableDynamic() : value_(nullptr) {}

  /* implicit */ SerializableDynamic(folly::dynamic value)
      : value_(std::move(value)) {}

  SerializableDynamic& operator=(folly::dynamic value) {
    value_ = std::move(value);
    return *this;
  }

  const folly::dynamic& operator*() const { return value_; }
        folly::dynamic& operator*()       { return value_; }

  const folly::dynamic* operator->() const { return &value_; }
        folly::dynamic* operator->()       { return &value_; }

  bool operator==(const SerializableDynamic& other) const {
    return value_ == other.value_;
  }

  bool operator<(const SerializableDynamic& other) const {
    return value_ < other.value_;
  }

 private:
  folly::dynamic value_;
  friend class ::apache::thrift::Cpp2Ops<SerializableDynamic>;
};

} // namespace util

////////////////////////////////////////////////////////////////////////////////

template <>
inline void Cpp2Ops< util::SerializableDynamic>::clear(
    util::SerializableDynamic* obj) {
  obj->value_ = nullptr;
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< util::SerializableDynamic>::write(
    Protocol* p,
    const util::SerializableDynamic* obj) {
  uint32_t xfer = 0;
  xfer += p->writeStructBegin("Variant");
  switch (obj->value_.type()) {
    case folly::dynamic::Type::NULLT:
      break;

    case folly::dynamic::Type::BOOL:
      xfer += p->writeFieldBegin("boolean", protocol::T_BOOL, 1);
      xfer += p->writeBool(obj->value_.asBool());
      xfer += p->writeFieldEnd();
      break;

    case folly::dynamic::Type::INT64:
      xfer += p->writeFieldBegin("integer", protocol::T_I64, 2);
      xfer += p->writeI64(obj->value_.asInt());
      xfer += p->writeFieldEnd();
      break;

    case folly::dynamic::Type::DOUBLE:
      xfer += p->writeFieldBegin("boolean", protocol::T_DOUBLE, 3);
      xfer += p->writeDouble(obj->value_.asDouble());
      xfer += p->writeFieldEnd();
      break;

    case folly::dynamic::Type::STRING:
      xfer += p->writeFieldBegin("boolean", protocol::T_STRING, 4);
      xfer += p->writeString(obj->value_.asString());
      xfer += p->writeFieldEnd();
      break;

    case folly::dynamic::Type::ARRAY:
      xfer += p->writeFieldBegin("array", protocol::T_LIST, 5);
      xfer += p->writeListBegin(protocol::T_STRUCT, obj->value_.size());
      for (const auto& item : obj->value_) {
        util::SerializableDynamic wrappedItem(item);
        xfer += Cpp2Ops<util::SerializableDynamic>::write(p, &wrappedItem);
      }
      xfer += p->writeListEnd();
      xfer += p->writeFieldEnd();
      break;

    case folly::dynamic::Type::OBJECT:
      xfer += p->writeFieldBegin("object", protocol::T_MAP, 6);
      xfer += p->writeMapBegin(protocol::T_STRING,
                               protocol::T_STRUCT,
                               obj->value_.size());
      for (const auto& item : obj->value_.items()) {
        util::SerializableDynamic wrappedItem(item.second);
        xfer += p->writeString(item.first.asString());
        xfer += Cpp2Ops<util::SerializableDynamic>::write(p, &wrappedItem);
      }
      xfer += p->writeMapEnd();
      xfer += p->writeFieldEnd();
      break;
  }

  xfer += p->writeFieldStop();
  xfer += p->writeStructEnd();
  return xfer;
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< util::SerializableDynamic>::serializedSize(
    Protocol* p,
    const util::SerializableDynamic* obj) {
  uint32_t xfer = 0;
  xfer += p->serializedStructSize("Variant");
  switch (obj->value_.type()) {
    case folly::dynamic::Type::NULLT:
      break;

    case folly::dynamic::Type::BOOL:
      xfer += p->serializedFieldSize("boolean", protocol::T_BOOL, 1);
      xfer += p->serializedSizeBool(obj->value_.asBool());
      break;

    case folly::dynamic::Type::INT64:
      xfer += p->serializedFieldSize("integer", protocol::T_I64, 2);
      xfer += p->serializedSizeI64(obj->value_.asInt());
      break;

    case folly::dynamic::Type::DOUBLE:
      xfer += p->serializedFieldSize("boolean", protocol::T_DOUBLE, 3);
      xfer += p->serializedSizeDouble(obj->value_.asDouble());
      break;

    case folly::dynamic::Type::STRING:
      xfer += p->serializedFieldSize("boolean", protocol::T_STRING, 4);
      xfer += p->serializedSizeString(obj->value_.asString());
      break;

    case folly::dynamic::Type::ARRAY:
      xfer += p->serializedFieldSize("array", protocol::T_LIST, 5);
      xfer += p->serializedSizeListBegin(protocol::T_STRUCT,
                                         obj->value_.size());
      for (const auto& item : obj->value_) {
        util::SerializableDynamic wrappedItem(item);
        xfer += Cpp2Ops<util::SerializableDynamic>::serializedSize(
            p, &wrappedItem);
      }
      xfer += p->serializedSizeListEnd();
      break;

    case folly::dynamic::Type::OBJECT:
      xfer += p->serializedFieldSize("object", protocol::T_MAP, 6);
      xfer += p->serializedSizeMapBegin(protocol::T_STRING,
                               protocol::T_STRUCT,
                               obj->value_.size());
      for (const auto& item : obj->value_.items()) {
        util::SerializableDynamic wrappedItem(item.second);
        xfer += p->serializedSizeString(item.first.asString());
        xfer += Cpp2Ops<util::SerializableDynamic>::serializedSize(
            p, &wrappedItem);
      }
      xfer += p->serializedSizeMapEnd();
      break;
  }
  xfer += p->serializedSizeStop();
  return xfer;
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< util::SerializableDynamic>::serializedSizeZC(
    Protocol* p,
    const util::SerializableDynamic* obj) {
  return Cpp2Ops< util::SerializableDynamic>::serializedSize(p, obj);
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< util::SerializableDynamic>::read(
    Protocol* iprot,
    util::SerializableDynamic* obj) {
  uint32_t xfer = 0;
  std::string fname;
  protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);
  xfer += iprot->readFieldBegin(fname, ftype, fid);
  if (ftype == protocol::T_STOP) {
    Cpp2Ops<util::SerializableDynamic>::clear(obj);
  } else {
    switch (fid) {
      case 1:
      {
        if (ftype == protocol::T_BOOL) {
          bool value;
          xfer += iprot->readBool(value);
          obj->value_ = value;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      }
      case 2:
      {
        if (ftype == protocol::T_I64) {
          int64_t value;
          xfer += iprot->readI64(value);
          obj->value_ = value;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      }
      case 3:
     {
        if (ftype == protocol::T_DOUBLE) {
          double value;
          xfer += iprot->readDouble(value);
          obj->value_ = value;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      }
      case 4:
      {
        if (ftype == protocol::T_STRING) {
          std::string value;
          xfer += iprot->readString(value);
          obj->value_ = value;
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      }
      case 5:
      {
        if (ftype == protocol::T_LIST) {
          obj->value_ = {};
          uint32_t size;
          protocol::TType etype;
          xfer += iprot->readListBegin(etype, size);
          for (uint32_t i = 0; i < size; ++i) {
            util::SerializableDynamic item;
            xfer += Cpp2Ops<util::SerializableDynamic>::read(iprot, &item);
            obj->value_.push_back(std::move(item.value_));
          }
          xfer += iprot->readListEnd();
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      }
      case 6:
      {
        if (ftype == protocol::T_MAP) {
          obj->value_ = folly::dynamic::object;
          uint32_t size;
          protocol::TType ktype;
          protocol::TType vtype;
          xfer += iprot->readMapBegin(ktype, vtype, size);
          for (uint32_t i = 0; i < size; ++i) {
            std::string key;
            xfer += iprot->readString(key);
            util::SerializableDynamic val;
            xfer += Cpp2Ops<util::SerializableDynamic>::read(iprot, &val);
            obj->value_[std::move(key)] = std::move(val.value_);
          }
          xfer += iprot->readMapEnd();
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      }
      default:
      {
        xfer += iprot->skip(ftype);
        break;
      }
    }
    xfer += iprot->readFieldEnd();
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    xfer += iprot->readFieldEnd();
  }
  xfer += iprot->readStructEnd();

  return xfer;
}

}}

#endif
