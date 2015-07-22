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

#ifndef CPP2_PROTOCOL_DEBUGPROTOCOL_H_
#define CPP2_PROTOCOL_DEBUGPROTOCOL_H_

#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <folly/Format.h>
#include <thrift/lib/cpp2/Thrift.h>

namespace apache { namespace thrift {

template <class T>
std::string debugString(const T& obj);

class DebugProtocolWriter {
 public:
  explicit DebugProtocolWriter(
      ExternalBufferSharing sharing = COPY_EXTERNAL_BUFFER /* ignored */);

  static inline ProtocolType protocolType() {
    return ProtocolType::T_DEBUG_PROTOCOL;
  }

  void setOutput(folly::IOBufQueue* queue,
                 size_t maxGrowth = std::numeric_limits<size_t>::max());

  uint32_t writeMessageBegin(const std::string& name,
                             MessageType messageType,
                             int32_t seqid);
  uint32_t writeMessageEnd();
  uint32_t writeStructBegin(const char* name);
  uint32_t writeStructEnd();
  uint32_t writeFieldBegin(const char* name, TType fieldType, int16_t fieldId);
  uint32_t writeFieldEnd();
  uint32_t writeFieldStop();
  uint32_t writeMapBegin(TType keyType, TType valType, uint32_t size);
  uint32_t writeMapEnd();
  uint32_t writeListBegin(TType elemType, uint32_t size);
  uint32_t writeListEnd();
  uint32_t writeSetBegin(TType elemType, uint32_t size);
  uint32_t writeSetEnd();
  uint32_t writeBool(bool value);
  uint32_t writeByte(int8_t byte);
  uint32_t writeI16(int16_t i16);
  uint32_t writeI32(int32_t i32);
  uint32_t writeI64(int64_t i64);
  uint32_t writeDouble(double dub);
  uint32_t writeFloat(float flt);
  template <typename StrType>
  uint32_t writeString(const StrType& str) { writeSP(str); return 0; }
  template <typename StrType>
  uint32_t writeBinary(const StrType& str) { writeSP(str); return 0; }
  uint32_t writeBinary(const std::unique_ptr<folly::IOBuf>& str);
  uint32_t writeBinary(const folly::IOBuf& str);
  uint32_t writeSerializedData(const std::unique_ptr<folly::IOBuf>& /*data*/) {
    //TODO
    return 0;
  }

 private:
  void indentUp();
  void indentDown();
  void startItem();
  void endItem();

  void writeSP(folly::StringPiece sp);

  void writeRaw(folly::StringPiece sp) {
    out_->append(sp.data(), sp.size());
  }

  template <class... Args>
  void writePlain(Args&&... args) {
    const auto& fmt = folly::format(std::forward<Args>(args)...);
    auto cb = [this] (folly::StringPiece sp) { this->writeRaw(sp); };
    fmt(cb);
  }

  void writeIndent() { writeRaw(indent_); }

  template <class... Args>
  void writeIndented(Args&&... args) {
    writeIndent();
    writePlain(std::forward<Args>(args)...);
  }

  template <class... Args>
  void writeItem(Args&&... args) {
    startItem();
    writePlain(std::forward<Args>(args)...);
    endItem();
  }

  enum ItemType {
    STRUCT,
    SET,
    MAP_KEY,
    MAP_VALUE,
    LIST
  };

  struct WriteState {
    /* implicit */ WriteState(ItemType t) : type(t), index(0) { }

    ItemType type;
    int index;
  };

  void pushState(ItemType t);
  void popState();

  folly::IOBufQueue* out_;
  std::string indent_;
  std::vector<WriteState> writeState_;
};

template <class T>
std::string debugString(const T& obj) {
  folly::IOBufQueue queue;
  DebugProtocolWriter proto;
  proto.setOutput(&queue);
  Cpp2Ops<T>::write(&proto, &obj);
  auto buf = queue.move();
  auto br = buf->coalesce();
  return std::string(reinterpret_cast<const char*>(br.data()), br.size());
}

}}  // namespace thrift

#endif /* CPP2_PROTOCOL_DEBUGPROTOCOL_H_ */
