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

#pragma once

#include <folly/Conv.h>
#include <string>
#include <thrift/lib/cpp2/Thrift.h>

namespace thrift { namespace test {

/**
 * We serialize the first four bytes of the data_ string into the intData
 * field and the rest into the stringData field of the thrift struct.
 */
class MyCustomStruct {
 public:
  MyCustomStruct() : data_(4, '\0') {}
  /* implicit */ MyCustomStruct(const std::string& data) : data_(data) {}

  bool operator==(const MyCustomStruct& other) const {
    return data_ == other.data_;
  }
  bool operator<(const MyCustomStruct& other) const {
    return data_ < other.data_;
  }

  std::string data_;
};

/**
 * If data_ is parseable into an int, we serialize it as the intData field of
 * the thrift union; otherwise as the stringData field.
 */
class MyCustomUnion {
 public:
  MyCustomUnion() {}
  /* implicit */ MyCustomUnion(const std::string& data) : data_(data) {}

  bool operator==(const MyCustomUnion& other) const {
    return data_ == other.data_;
  }
  bool operator<(const MyCustomUnion& other) const {
    return data_ < other.data_;
  }

  std::string data_;
};

}}

////////////////////////////////////////////////////////////////////////////////

namespace apache { namespace thrift {

template <>
inline void Cpp2Ops< ::thrift::test::MyCustomStruct>::clear(
    ::thrift::test::MyCustomStruct* obj) {
  obj->data_.clear();
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< ::thrift::test::MyCustomStruct>::write(
    Protocol* p,
    const ::thrift::test::MyCustomStruct* obj) {
  uint32_t xfer = 0;
  assert(obj->data_.size() >= sizeof(int));
  const int prefix = *reinterpret_cast<const int*>(&obj->data_[0]);
  std::string suffix = obj->data_.substr(sizeof(int));
  xfer += p->writeStructBegin("MyStruct");
  xfer += p->writeFieldBegin("stringData", protocol::T_STRING, 1);
  xfer += p->writeString(suffix);
  xfer += p->writeFieldEnd();
  xfer += p->writeFieldBegin("intData", protocol::T_I32, 2);
  xfer += p->writeI32(prefix);
  xfer += p->writeFieldEnd();
  xfer += p->writeFieldStop();
  xfer += p->writeStructEnd();
  return xfer;
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< ::thrift::test::MyCustomStruct>::serializedSize(
    Protocol* p,
    const ::thrift::test::MyCustomStruct* obj) {
  uint32_t xfer = 0;
  assert(obj->data_.size() >= sizeof(int));
  const int prefix = *reinterpret_cast<const int*>(&obj->data_[0]);
  std::string suffix = obj->data_.substr(sizeof(int));
  xfer += p->serializedStructSize("MyStruct");
  xfer += p->serializedFieldSize("stringData", protocol::T_STRING, 1);
  xfer += p->serializedSizeString(suffix);
  xfer += p->serializedFieldSize("intData", apache::thrift::protocol::T_I32, 2);
  xfer += p->serializedSizeI32(prefix);
  xfer += p->serializedSizeStop();
  return xfer;
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< ::thrift::test::MyCustomStruct>::serializedSizeZC(
    Protocol* p,
    const ::thrift::test::MyCustomStruct* obj) {
  return Cpp2Ops< ::thrift::test::MyCustomStruct>::serializedSize(p, obj);
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< ::thrift::test::MyCustomStruct>::read(
    Protocol* iprot,
    ::thrift::test::MyCustomStruct* obj) {
  uint32_t xfer = 0;
  std::string fname;
  protocol::TType ftype;
  int16_t fid;
  std::string suffix;
  int prefix;

  xfer += iprot->readStructBegin(fname);

  while (true) {
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    if (ftype == apache::thrift::protocol::T_STOP) {
      break;
    }
    switch (fid) {
      case 1:
        if (ftype == apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(suffix);
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == apache::thrift::protocol::T_I32) {
          xfer += iprot->readI32(prefix);
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
  }
  xfer += iprot->readStructEnd();

  obj->data_ = std::string(reinterpret_cast<const char*>(&prefix),
                           sizeof(int)) +
               suffix;
  return xfer;
}

template <>
inline constexpr apache::thrift::protocol::TType
Cpp2Ops< ::thrift::test::MyCustomStruct>::thriftType() {
  return apache::thrift::protocol::T_STRUCT;
}

////////////////////////////////////////////////////////////////////////////////

template <>
inline void Cpp2Ops< ::thrift::test::MyCustomUnion>::clear(
    ::thrift::test::MyCustomUnion* obj) {
  obj->data_.clear();
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< ::thrift::test::MyCustomUnion>::write(
    Protocol* p,
    const ::thrift::test::MyCustomUnion* obj) {
  uint32_t xfer = 0;
  xfer += p->writeStructBegin("MyStruct");
  try {
    int i = folly::to<int>(obj->data_);
    xfer += p->writeFieldBegin("intData", protocol::T_I32, 2);
    xfer += p->writeI32(i);
    xfer += p->writeFieldEnd();
  } catch (const std::range_error&) {
    xfer += p->writeFieldBegin("stringData", protocol::T_STRING, 1);
    xfer += p->writeString(obj->data_);
    xfer += p->writeFieldEnd();
  }
  xfer += p->writeFieldStop();
  xfer += p->writeStructEnd();
  return xfer;
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< ::thrift::test::MyCustomUnion>::serializedSize(
    Protocol* p,
    const ::thrift::test::MyCustomUnion* obj) {
  uint32_t xfer = 0;
  xfer += p->serializedStructSize("MyStruct");
  try {
    int i = folly::to<int>(obj->data_);
    xfer += p->serializedFieldSize("intData", protocol::T_I32, 2);
    xfer += p->serializedSizeI32(i);
  } catch (const std::range_error&) {
    xfer += p->serializedFieldSize("stringData", protocol::T_STRING, 1);
    xfer += p->serializedSizeString(obj->data_);
  }
  xfer += p->serializedSizeStop();
  return xfer;
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< ::thrift::test::MyCustomUnion>::serializedSizeZC(
    Protocol* p,
    const ::thrift::test::MyCustomUnion* obj) {
  return Cpp2Ops< ::thrift::test::MyCustomUnion>::serializedSize(p, obj);
}

template <>
template <class Protocol>
inline uint32_t Cpp2Ops< ::thrift::test::MyCustomUnion>::read(
    Protocol* iprot,
    ::thrift::test::MyCustomUnion* obj) {
  uint32_t xfer = 0;
  std::string fname;
  protocol::TType ftype;
  int16_t fid;

  xfer += iprot->readStructBegin(fname);

  xfer += iprot->readFieldBegin(fname, ftype, fid);
  if (ftype != apache::thrift::protocol::T_STOP) {
    switch (fid) {
      case 1:
        if (ftype == apache::thrift::protocol::T_STRING) {
          xfer += iprot->readString(obj->data_);

        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      case 2:
        if (ftype == apache::thrift::protocol::T_I32) {
          int i;
          xfer += iprot->readI32(i);
          obj->data_ = folly::to<std::string>(i);
        } else {
          xfer += iprot->skip(ftype);
        }
        break;
      default:
        xfer += iprot->skip(ftype);
        break;
    }
    xfer += iprot->readFieldEnd();
    xfer += iprot->readFieldBegin(fname, ftype, fid);
    xfer += iprot->readFieldEnd();
  }

  xfer += iprot->readStructEnd();
  return xfer;
}

template <>
inline constexpr apache::thrift::protocol::TType
Cpp2Ops< ::thrift::test::MyCustomUnion>::thriftType() {
  return apache::thrift::protocol::T_STRUCT;
}

}}
