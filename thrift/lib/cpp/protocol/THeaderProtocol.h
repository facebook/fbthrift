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

#ifndef THRIFT_PROTOCOL_THEADERPROTOCOL_H_
#define THRIFT_PROTOCOL_THEADERPROTOCOL_H_ 1

#include <thrift/lib/cpp/protocol/TProtocol.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/protocol/TVirtualProtocol.h>
#include <thrift/lib/cpp/transport/THeaderTransport.h>
#include <thrift/lib/cpp/util/shared_ptr_util.h>

#include <memory>

#include <bitset>

namespace apache { namespace thrift { namespace protocol {

/**
 * The header protocol for thrift. Reads unframed, framed, header format,
 * and http
 *
 */
class THeaderProtocol
  : public TVirtualProtocol<THeaderProtocol> {
 public:
  explicit THeaderProtocol(const std::shared_ptr<TTransport>& trans,
                           std::bitset<CLIENT_TYPES_LEN>* clientTypes = nullptr,
                           uint16_t protoId = T_COMPACT_PROTOCOL,
                           int8_t protoVersion = -1) :
      TVirtualProtocol<THeaderProtocol>(getTransportWrapper(trans,
                                                            clientTypes))
      , trans_(std::dynamic_pointer_cast<transport::THeaderTransport,
                                         TTransport>(this->getTransport()))
      , protoId_(protoId)
      , protoVersion_(protoVersion)
  {
    trans_->setProtocolId(protoId);
    trans_->setProtocolVersion(protoVersion);
    resetProtocol();
  }

  THeaderProtocol(const std::shared_ptr<TTransport>& inTrans,
                  const std::shared_ptr<TTransport>& outTrans,
                  std::bitset<CLIENT_TYPES_LEN>* clientTypes = nullptr,
                  uint16_t protoId = T_COMPACT_PROTOCOL,
                  int8_t protoVersion = -1) :
      TVirtualProtocol<THeaderProtocol>(getInOutTransportWrapper(inTrans,
                                                                 outTrans,
                                                                 clientTypes))
      , trans_(std::dynamic_pointer_cast<transport::THeaderTransport,
                                         TTransport>(this->getTransport()))
      , protoId_(protoId)
      , protoVersion_(protoVersion)
  {
    trans_->setProtocolId(protoId);
    trans_->setProtocolVersion(protoVersion);
    resetProtocol();
  }

  /**
   * Construct a THeaderProtocol using a raw pointer to the transport.
   *
   * The caller is responsible for ensuring that the transport remains valid
   * for the lifetime of the protocol.
   */
  THeaderProtocol(TTransport* trans,
                  std::bitset<CLIENT_TYPES_LEN>* clientTypes,
                  uint16_t protoId = T_COMPACT_PROTOCOL,
                  int8_t protoVersion = -1) :
      TVirtualProtocol<THeaderProtocol>(
          getTransportWrapper(
              std::shared_ptr<TTransport>(trans,
                                            NoopPtrDestructor<TTransport>()),
              clientTypes))
      , trans_(std::dynamic_pointer_cast<transport::THeaderTransport,
                                         TTransport>(this->getTransport()))
      , protoId_(protoId)
      , protoVersion_(protoVersion)
  {
    trans_->setProtocolId(protoId);
    trans_->setProtocolVersion(protoVersion);
    resetProtocol();
  }

  ~THeaderProtocol() override {}

  /**
   * Functions to work with headers by calling into THeaderTransport
   */
  void setProtocolId(uint16_t protoId) {
    trans_->setProtocolId(protoId);
    resetProtocol();
  }

  void setProtocolVersion(int8_t protoVersion) {
    trans_->setProtocolVersion(protoVersion);
    resetProtocol();
  }

  void resetProtocol();

  typedef transport::THeaderTransport::StringToStringMap StringToStringMap;

  // these work with write headers
  void setHeader(const std::string& key, const std::string& value) {
    trans_->setHeader(key, value);
  }

  void setHeaders(const StringToStringMap& headers) {
    for (const auto& it : headers) {
      setHeader(it.first, it.second);
    }
  }

  bool isWriteHeadersEmpty() {
    return trans_->isWriteHeadersEmpty();
  }

  void setPersistentHeader(const std::string& key, const std::string& value) {
    trans_->setPersistentHeader(key, value);
  }

  void clearHeaders() {
    trans_->clearHeaders();
  }

  void clearPersistentHeaders() {
    trans_->clearPersistentHeaders();
  }

  StringToStringMap& getPersistentWriteHeaders() {
    return trans_->getPersistentWriteHeaders();
  }

  // these work with read headers
  const StringToStringMap& getHeaders() const {
    return trans_->getHeaders();
  }

  void setTransform(uint16_t trans) {
    trans_->setTransform(trans);
  }

  std::string getPeerIdentity() const {
    return trans_->getPeerIdentity();
  }
  void setIdentity(const std::string& identity) {
    trans_->setIdentity(identity);
  }

  /**
   * Writing functions.
   */

  /*ol*/ uint32_t writeMessageBegin(const std::string& name,
                                    const TMessageType messageType,
                                    const int32_t seqId);

  /*ol*/ uint32_t writeMessageEnd();


  uint32_t writeStructBegin(const char* name);

  uint32_t writeStructEnd();

  uint32_t writeFieldBegin(const char* name,
                           const TType fieldType,
                           const int16_t fieldId);

  uint32_t writeFieldEnd();

  uint32_t writeFieldStop();

  uint32_t writeMapBegin(const TType keyType,
                         const TType valType,
                         const uint32_t size);

  uint32_t writeMapEnd();

  uint32_t writeListBegin(const TType elemType, const uint32_t size);

  uint32_t writeListEnd();

  uint32_t writeSetBegin(const TType elemType, const uint32_t size);

  uint32_t writeSetEnd();

  uint32_t writeBool(const bool value);

  uint32_t writeByte(const int8_t byte);

  uint32_t writeI16(const int16_t i16);

  uint32_t writeI32(const int32_t i32);

  uint32_t writeI64(const int64_t i64);

  uint32_t writeDouble(const double dub);

  uint32_t writeFloat(const float flt);

  template<typename StrType>
  uint32_t writeString(const StrType& str) {
    return proto_->writeString(str);
  }

  template<typename StrType>
  uint32_t writeBinary(const StrType& str) {
    return proto_->writeBinary(str);
  }

  /**
   * Reading functions
   */


  /*ol*/ uint32_t readMessageBegin(std::string& name,
                                   TMessageType& messageType,
                                   int32_t& seqId);

  /*ol*/ uint32_t readMessageEnd();

  uint32_t readStructBegin(std::string& name);

  uint32_t readStructEnd();

  uint32_t readFieldBegin(std::string& name,
                          TType& fieldType,
                          int16_t& fieldId);

  uint32_t readFieldEnd();

  uint32_t readMapBegin(TType& keyType,
                        TType& valType,
                        uint32_t& size,
                        bool& sizeUnknown);

  uint32_t readMapEnd();

  uint32_t readListBegin(TType& elemType, uint32_t& size, bool& sizeUnknown);

  uint32_t readListEnd();

  uint32_t readSetBegin(TType& elemType, uint32_t& size, bool& sizeUnknown);

  uint32_t readSetEnd();

  uint32_t readBool(bool& value);
  // Provide the default readBool() implementation for std::vector<bool>
  using TVirtualProtocol< THeaderProtocol >::readBool;

  uint32_t readByte(int8_t& byte);

  uint32_t readI16(int16_t& i16);

  uint32_t readI32(int32_t& i32);

  uint32_t readI64(int64_t& i64);

  uint32_t readDouble(double& dub);

  uint32_t readFloat(float& flt);

  template<typename StrType>
  uint32_t readString(StrType& str) {
    return proto_->readString(str);
  }

  template<typename StrType>
  uint32_t readBinary(StrType& binary) {
    return proto_->readBinary(binary);
  }

 protected:
  std::shared_ptr<TTransport> getTransportWrapper(
    const std::shared_ptr<TTransport>& trans,
    std::bitset<CLIENT_TYPES_LEN>* clientTypes) {
    if (dynamic_cast<transport::THeaderTransport*>(trans.get()) != nullptr) {
      return trans;
    } else {
      return std::shared_ptr<transport::THeaderTransport>(
        new transport::THeaderTransport(trans, clientTypes));
    }
  }

  std::shared_ptr<TTransport> getInOutTransportWrapper(
    const std::shared_ptr<TTransport>& inTrans,
    const std::shared_ptr<TTransport>& outTrans,
    std::bitset<CLIENT_TYPES_LEN>* clientTypes) {
    assert(dynamic_cast<transport::THeaderTransport*>(inTrans.get()) ==
              nullptr &&
           dynamic_cast<transport::THeaderTransport*>(outTrans.get()) ==
              nullptr);

    return std::shared_ptr<transport::THeaderTransport>(
      new transport::THeaderTransport(inTrans, outTrans, clientTypes)
    );
  }

  std::shared_ptr<transport::THeaderTransport> trans_;

  std::shared_ptr<TProtocol> proto_;
  uint32_t protoId_;
  int8_t protoVersion_;
};

/**
 * Constructs header protocol handlers
 */
class THeaderProtocolFactory : public TDuplexProtocolFactory {
 public:
  explicit THeaderProtocolFactory(uint16_t protoId = T_COMPACT_PROTOCOL,
                                  int8_t protoVersion = -1,
                                  bool disableIdentity = false) {
    protoId_ = protoId;
    protoVersion_ = protoVersion;
    setIdentity_ = disableIdentity;
  }

  ~THeaderProtocolFactory() override {}

  void setClientTypes(std::bitset<CLIENT_TYPES_LEN>& clientTypes) {
    for (int i = 0; i < CLIENT_TYPES_LEN; i++) {
      clientTypes_[i] = clientTypes[i];
    }
  }

  void setIdentity(const std::string& identity) {
    identity_ = identity;
    setIdentity_ = true;
  }

  void setTransform(uint16_t trans) {
    trans_.push_back(trans);
  }

  virtual TProtocolPair getProtocol(
      std::shared_ptr<transport::TTransport> trans) {
    THeaderProtocol* prot = new THeaderProtocol(trans,
                                                &clientTypes_,
                                                protoId_,
                                                protoVersion_);

    if(setIdentity_) {
      prot->setIdentity(identity_);
    }

    for (auto& t : trans_) {
      prot->setTransform(t);
    }

    std::shared_ptr<TProtocol> pprot(prot);
    return TProtocolPair(pprot, pprot);
  }

  TProtocolPair getProtocol(transport::TTransportPair transports) override {
    THeaderProtocol* prot = new THeaderProtocol(transports.first,
                                                transports.second,
                                                &clientTypes_,
                                                protoId_,
                                                protoVersion_);

    if(setIdentity_) {
      prot->setIdentity(identity_);
    }

    for (auto& t : trans_) {
      prot->setTransform(t);
    }

    std::shared_ptr<TProtocol> pprot(prot);
    return TProtocolPair(pprot, pprot);
  }

  // No implementation of getInputProtocolFactory/getOutputProtocolFactory
  // Using base class implementation which return nullptr.

 private:
  std::bitset<CLIENT_TYPES_LEN> clientTypes_;
  uint16_t protoId_;
  int8_t protoVersion_;
  bool setIdentity_;
  std::vector<uint16_t> trans_;
  std::string identity_;
};

}}} // apache::thrift::protocol

#endif // #ifndef THRIFT_PROTOCOL_THEADERPROTOCOL_H_
