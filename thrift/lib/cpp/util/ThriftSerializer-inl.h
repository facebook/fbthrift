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

#ifndef COMMON_STRINGS_THRIFT_SERIALIZER_INL_H
#define COMMON_STRINGS_THRIFT_SERIALIZER_INL_H

#include <stdexcept>
#include <string>
#include <thrift/lib/cpp/util/ThriftSerializer.h>

namespace apache { namespace thrift { namespace util {

using apache::thrift::protocol::TMessageType;

template <typename Dummy, typename P>
template <typename T, class String>
void
ThriftSerializer<Dummy, P>::serialize(const T& fields, String* serialized)
{
  // prepare or reset buffer
  //
  // note: Three cases:
  //
  //   1) The buffer has never been prepared.  Call prepare().
  //
  //   2) The buffer was last used for a deserialize() call.  In this
  //      case, the buffer is pointing to a now-invalid constant
  //      string, and the buffer should be reallocated.
  //
  //   3) The buffer was last used for a serialize() call.  In this
  //      case, the buffer is still pointing to a valid buffer, but it
  //      contains the serialization from the last serialize(), and
  //      the buffer just needs to be reset.
  //
  // This would be a little simpler if we allocated separate buffers
  // and protocols for serialization and deserialization, but it
  // seemed to me that this fit the common use cases better.  The case
  // that performs poorly (20% slower) is doing many alternating
  // serializations and deserializations.
  if (!prepared_ || lastDeserialized_) {
    prepare();
  } else {
    buffer_->resetBuffer();
  }
  lastDeserialized_ = false;

  // serialize protocol version if necessary
  if (serializeVersion_) {
    protocol_->writeMessageBegin("", TMessageType (0), 0);
  }

  // serialize fields into buffer
  fields.write(protocol_.get());

  // write the end of message
  if (serializeVersion_) {
    protocol_->writeMessageEnd();
  }

  // assign buffer to string
  uint8_t *byteBuffer;
  uint32_t byteBufferSize;
  buffer_->getBuffer(&byteBuffer, &byteBufferSize);
  serialized->assign((const char*)byteBuffer, byteBufferSize);
}

template <typename Dummy, typename P>
template <typename T>
void
ThriftSerializer<Dummy, P>::serialize(const T& fields,
                                      const uint8_t** serializedBuffer,
                                      size_t* serializedLen) {
  CHECK(serializedBuffer);
  CHECK(serializedLen);

  // prepare or reset buffer
  if (!prepared_ || lastDeserialized_) {
    prepare();
  } else {
    buffer_->resetBuffer();
  }
  lastDeserialized_ = false;

  // if required serialize protocol version
  if (serializeVersion_) {
    protocol_->writeMessageBegin("", TMessageType (0), 0);
  }

  // serialize fields into buffer
  fields.write(protocol_.get());

  // write the end of message
  if (serializeVersion_) {
    protocol_->writeMessageEnd();
  }

  // assign buffer to string
  uint8_t *byteBuffer;
  uint32_t byteBufferSize;
  buffer_->getBuffer(&byteBuffer, &byteBufferSize);
  *serializedBuffer = byteBuffer;
  *serializedLen =  byteBufferSize;
}


// Deserializes a thrift object, but assumes that the object passed in the
// fields argument is "clean". If your thrift class contains optional fields,
// you should use deserialize() instead (see below), otherwise optional fields
// that were previously set by other calls to this function won't get reset,
// and will appear to have been desearialized along with the other fields.
template <typename Dummy, typename P>
template <typename T, class String>
uint32_t
ThriftSerializer<Dummy, P>::deserializeClean(const String& serialized,
                                             T* fields)
{
  // prepare buffer if necessary
  if (!prepared_) {
    prepare();
  }
  lastDeserialized_ = true;

  // reset buffer transport to passed string
  buffer_->resetBuffer((uint8_t*)serialized.data(), serialized.size());

  // read the protocol version if necessary
  if (serializeVersion_) {
    std::string name = "";
    TMessageType mt = (TMessageType) 0;
    int32_t seq_id = 0;
    protocol_->readMessageBegin(name, mt, seq_id);
  }

  // deserialize buffer into fields
  uint32_t len = fields->read(protocol_.get());

  // read the end of message
  if (serializeVersion_) {
    protocol_->readMessageEnd();
  }

  return len;
}

template <typename Dummy, typename P>
template <typename T>
uint32_t
ThriftSerializer<Dummy, P>::deserialize(const uint8_t* serializedBuffer,
                                        size_t length,
                                        T* fields)
{
  // prepare buffer if necessary
  if (!prepared_) {
    prepare();
  }
  lastDeserialized_ = true;

  // reset buffer transport to passed string
  buffer_->resetBuffer((uint8_t*)serializedBuffer, length);

  // need to clean the existing structure, as fields->read() will not overwrite
  // the optional fields that were not set in the serialized object.
  T emptyFields;
  swap(emptyFields, *fields);

  // read the protocol version if necessary
  if (serializeVersion_) {
    std::string name = "";
    TMessageType mt = (TMessageType) 0;
    int32_t seq_id = 0;
    protocol_->readMessageBegin(name, mt, seq_id);
  }

  // deserialize buffer into fields
  uint32_t len = fields->read(protocol_.get());

  // read the end of message
  if (serializeVersion_) {
    protocol_->readMessageEnd();
  }

  return len;
}

template <typename Dummy, typename P>
void
ThriftSerializer<Dummy, P>::setSerializeVersion(bool value)
{
  serializeVersion_ = value;
}

template <typename Dummy, typename P>
void
ThriftSerializer<Dummy, P>::setVersion(int8_t version)
{
  version_ = version;
  setVersion_ = true;
}

template <typename Dummy, typename P>
void
ThriftSerializer<Dummy, P>::prepare()
{
  // create memory buffer to use as transport
  //
  // note: TMemoryBuffer won't write to the buffer unless it is the
  // owner of it, which means that we can't pass in our own local
  // buffer.  Either must malloc a buffer which TMemoryBuffer will
  // free, or else just let it malloc its own.  This has the added
  // benefit that TMemoryBuffer will grow the buffer size as
  // necessary.
  //
  // The initial allocation of the memory buffer might seem
  // unnecessary in case of parsing (since the memory buffer will be
  // reset to point to the string passed to parseSerializer()), but
  // it's apparently necessary for some protocols.
  buffer_.reset(new TMemoryBuffer());

  // create a protocol for the memory buffer transport
  protocol_.reset(new Protocol(buffer_));
  if (setVersion_) {
    protocol_->setVersion(version_);
  }

  prepared_ = true;
}


}}} // namespace apache::thrift::util


#endif
