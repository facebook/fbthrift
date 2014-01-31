/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

// @author Karl Voskuil (karl@facebook.com)
// @author Mark Rabkin (mrabkin@facebook.com)
//

#ifndef COMMON_STRINGS_THRIFT_SERIALIZER_H
#define COMMON_STRINGS_THRIFT_SERIALIZER_H

#include <string>
#include <memory>

#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"  // for serialization
#include "thrift/lib/cpp/protocol/TCompactProtocol.h"  // for serialization
#include "thrift/lib/cpp/protocol/TJSONProtocol.h"    // for serialization
#include "thrift/lib/cpp/protocol/TSimpleJSONProtocol.h"    // for serialization
#include "thrift/lib/cpp/transport/TTransportUtils.h" // for serialization

namespace apache { namespace thrift { namespace util {

using apache::thrift::protocol::TBinaryProtocolT;
using apache::thrift::protocol::TCompactProtocolT;
using apache::thrift::protocol::TCompactProtocol;
using apache::thrift::protocol::TSimpleJSONProtocol;
using apache::thrift::protocol::TJSONProtocol;
using apache::thrift::transport::TBufferBase;
using apache::thrift::transport::TTransport;
using apache::thrift::transport::TMemoryBuffer;
using std::shared_ptr;

/**
 * Encapsulates methods to serialize or deserialize a code-gen-ed
 * Thrift class to and from a string.  For example:
 *
 *    extern StatsMcValue data;
 *
 *    ThriftSerializerBinary<> serializer;
 *    string serialized;
 *    serializer.serialize(data, &serialized);
 *
 *    StatsMcValue result;
 *    try {
 *      if (serializer.deserialize(serialized, &result) !=
 *          serialized.length()) {
 *        // Handle deserialization error, not all data consumed.
 *      }
 *    } catch (TProtocolException& tpe) {
 *      // Handle deserialization error, underlying protocol threw.
 *    }
 *    // Use deserialized thrift object.
 *    ...
 *
 * @author Karl Voskuil
 */

// This used to be templated on the type of the thrift value to be
// serialized or deserialized, but instead, we now make the serialize
// and deserialize methods templated on this type.  This allows a
// single object to be used polymorphically.  We keep the old template
// signature on the class to avoid needing to change a ton of code
// elsewhere, but provide a default where it trails, so it can be
// declared without a type. If in the future all usage sites are
// cleaned up, then we can eliminate the Dummy template arg here, and
// make the classes below non-template classes.

template <typename Dummy, typename P>
class ThriftSerializer {
 public:
  ThriftSerializer()
  : prepared_(false)
  , lastDeserialized_(false)
  , setVersion_(false)
  , serializeVersion_(false) {}

  /**
   * Serializes the passed type into the passed string.
   *
   * @author Karl Voskuil
   */
  template <typename T, class String>
  void serialize(const T& fields, String* serialized);

  /**
   * Serializes the passed type into the internal buffer
   * and returns a pointer to the internal buffer and its size.
   *
   * @author Yuri Putivsky
   */
  template <typename T>
  void serialize(const T& fields, const uint8_t** serializedBuffer,
                 size_t* serializedLen);

  /**
   * Deserializes the passed string into the passed type, returns the number of
   * bytes that have been consumed from the passed string.
   *
   * The return value can be used to verify if the deserialization is successful
   * or not.  When a type is serialized then deserialized back, the number of
   * consumed bytes must equal to the size of serialized string.  But please
   * note, equality of these two values doesn't guarantee the serialized string
   * isn't corrupt.  It's up to the underlying implementation of the type and
   * thrift protocol to detect and handle invalid serialized string, they may
   * throw exception or just ignore the unrecognized data.
   * @author Karl Voskuil
   */
  template <typename T, class String>
  uint32_t deserialize(const String& serialized, T* fields)
  {
    return deserialize((const uint8_t*)serialized.data(),
                       serialized.size(),
                       fields);
  }

  /**
   * Deserializes the passed char array into the passed type, returns the number
   * of bytes that have been consumed from the passed string.
   *
   * See notes on return value for:
   *   deserilize(const String* serialized, T* fields)
   */
  template <typename T>
  uint32_t deserialize(const uint8_t* serializedBuffer,
                       size_t length,
                       T* fields);

  /**
   * Same as deserialize() above, but won't touch/reset any optional fields
   * that are not present in 'serialized'. So, if T has any optional fields,
   * the caller is responsible for resetting those (or somehow handling the
   * potentially dirty data.)
   *
   * Use this method if:
   *  1) your thrift class doesn't contain any optional fields, and
   *  2) you are trying to avoid memory allocations/fragmentation during
   *     deserialization of thrift objects
   *
   * @author Rafael Sagula
   */
  template <typename T, class String>
  uint32_t deserializeClean(const String& serialized, T* fields);

  /**
   * Set version of protocol data to read/write.  This is only necessary
   * for data that will be saved to disk between protocol versions!
   * You probably don't need to use this unless you know what you are doing
   *
   * @author davejwatson
   */
  void setVersion(int8_t version);

  void setSerializeVersion(bool value);


 private:
  void prepare();

 private:
  typedef P Protocol;

  bool prepared_;
  bool lastDeserialized_;
  shared_ptr<TMemoryBuffer> buffer_;
  shared_ptr<Protocol> protocol_;
  int8_t version_;
  bool setVersion_;
  bool serializeVersion_;
};

template <typename Dummy = void>
struct ThriftSerializerBinary
  : public ThriftSerializer<Dummy, TBinaryProtocolT<TBufferBase> >
{ };

template <typename Dummy = void>
struct ThriftSerializerCompact
  : public ThriftSerializer<Dummy, TCompactProtocolT<TBufferBase> >
{ };

/**
 * This version is deprecated.  Please do not use it anymore,
 * unless you have data already serialized to disk in this format.
 * Doubles are not serialized in correct network order, so making RPC
 * calls with this data will not work.
 */
template <typename Dummy = void>
struct ThriftSerializerCompactDeprecated
  : public ThriftSerializer<Dummy, TCompactProtocolT<TBufferBase> >
{
 public:
  ThriftSerializerCompactDeprecated() {
    this->setVersion(TCompactProtocol::VERSION_LOW);
  }
};

template <typename Dummy = void>
struct ThriftSerializerJson
  : public ThriftSerializer<Dummy, TJSONProtocol>
{ };

template <typename Dummy = void>
struct ThriftSerializerSimpleJson
  : public ThriftSerializer<Dummy, TSimpleJSONProtocol>
{ };

}}} // namespace apache::thrift:util

namespace apache { namespace thrift {

template<typename ThriftStruct>
  std::string ThriftJSONString(const ThriftStruct& ts) {
  using namespace apache::thrift::protocol;
  using namespace apache::thrift::util;
  ThriftSerializer<ThriftStruct, TJSONProtocol> serializer;
  std::string serialized;
  serializer.serialize(ts, &serialized);
  return serialized;
}

template<typename ThriftStruct>
  std::string ThriftSimpleJSONString(const ThriftStruct& ts) {
  using namespace apache::thrift::protocol;
  using namespace apache::thrift::util;
  ThriftSerializer<ThriftStruct, TSimpleJSONProtocol> serializer;
  std::string serialized;
  serializer.serialize(ts, &serialized);
  return serialized;
}

}} // apache::thrift


#include "thrift/lib/cpp/util/ThriftSerializer-inl.h"

#endif
