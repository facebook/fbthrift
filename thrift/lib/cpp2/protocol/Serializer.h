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
#ifndef CPP2_SERIALIZER_H
#define CPP2_SERIALIZER_H

#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/JSONProtocol.h>
#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <folly/io/IOBuf.h>

namespace apache { namespace thrift {

template <typename Reader, typename Writer>
struct Serializer {
  template <class T>
  static uint32_t deserialize(
      const folly::IOBuf* buf, T& obj,
      ExternalBufferSharing sharing = COPY_EXTERNAL_BUFFER) {
    Reader reader(sharing);
    reader.setInput(buf);

    // This can be obj.read(&reader);
    // if you don't need to support thrift1-compatibility types
    return apache::thrift::Cpp2Ops<T>::read(&reader, &obj);
  }

  template <class T>
  static uint32_t deserialize(
      folly::ByteRange range, T& obj,
      ExternalBufferSharing sharing = COPY_EXTERNAL_BUFFER) {
    folly::IOBuf buf(folly::IOBuf::WRAP_BUFFER, range);
    return deserialize(&buf, obj, sharing);
  }

  template <class T>
  static uint32_t deserialize(
      folly::StringPiece range, T& obj,
      ExternalBufferSharing sharing = COPY_EXTERNAL_BUFFER) {
    return deserialize(folly::ByteRange(range), obj, sharing);
  }

  template <class T>
  static T deserialize(
      const folly::IOBuf* buf, uint32_t* size = nullptr) {
    return returning<T>([&](T& obj) { set(size, deserialize(buf, obj)); });
  }

  template <class T>
  static T deserialize(
      folly::ByteRange range, uint32_t* size = nullptr,
      ExternalBufferSharing sharing = COPY_EXTERNAL_BUFFER) {
    return returning<T>([&](T& obj) {
        set(size, deserialize(range, obj, sharing));
    });
  }

  template <class T>
  static T deserialize(
      folly::StringPiece range, uint32_t* size = nullptr,
      ExternalBufferSharing sharing = COPY_EXTERNAL_BUFFER) {
    return deserialize<T>(folly::ByteRange(range), size, sharing);
  }

  template <class T>
  static void serialize(
      const T& obj, folly::IOBufQueue* out,
      ExternalBufferSharing sharing = COPY_EXTERNAL_BUFFER) {
    Writer writer(sharing);
    writer.setOutput(out);

    // This can be obj.write(&writer);
    // if you don't need to support thrift1-compatibility types
    apache::thrift::Cpp2Ops<T>::write(&writer, &obj);
  }

  template <class T>
  static void serialize(const T& obj, std::string* out) {
    folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
    // Okay to share any external buffers, as we'll copy them to *out
    // immediately afterwards.
    serialize(obj, &queue, SHARE_EXTERNAL_BUFFER);
    queue.appendToString(*out);
  }

  template <class R, class T, typename... Args>
  static R serialize(const T& obj, Args&&... args) {
    R _return;
    serialize(obj, &_return, std::forward<Args>(args)...);
    return _return;
  }

 private:

  template <typename T>
  static void set(T* t, T&& v) {
    if (t != nullptr) {
      *t = std::forward<T>(v);
    }
  }

  template <typename R, typename F>
  static R returning(F&& f) {
    R _return;
    f(_return);
    return _return;
  }
};

typedef Serializer<CompactProtocolReader, CompactProtocolWriter>
  CompactSerializer;
typedef Serializer<BinaryProtocolReader, BinaryProtocolWriter>
  BinarySerializer;
typedef Serializer<JSONProtocolReader, JSONProtocolWriter>
  JSONSerializer;
typedef Serializer<SimpleJSONProtocolReader, SimpleJSONProtocolWriter>
  SimpleJSONSerializer;

// Serialization code specific to handling errors
template<typename ProtIn, typename ProtOut>
std::unique_ptr<folly::IOBuf> serializeErrorProtocol(
    TApplicationException obj, folly::IOBuf* req) {
  ProtIn iprot;
  std::string fname;
  apache::thrift::MessageType mtype;
  int32_t protoSeqId = 0;
  iprot.setInput(req);
  iprot.readMessageBegin(fname, mtype, protoSeqId);

  ProtOut prot;
  size_t bufSize = obj.serializedSizeZC(&prot);
  bufSize += prot.serializedMessageSize(fname);
  folly::IOBufQueue queue;
  prot.setOutput(&queue, bufSize);
  prot.writeMessageBegin(fname,
                         apache::thrift::T_EXCEPTION, protoSeqId);
  obj.write(&prot);
  prot.writeMessageEnd();
  return queue.move();
}

std::unique_ptr<folly::IOBuf> serializeError(
  int protId, TApplicationException obj, folly::IOBuf* buf);

}} // namespace apache::thrift
#endif
