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

#ifndef THRIFT_TRANSPORT_THEADER_H_
#define THRIFT_TRANSPORT_THEADER_H_ 1

#include <functional>

#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/protocol/TCompactProtocol.h"
#include "thrift/lib/cpp/protocol/TProtocolTypes.h"
#include "thrift/lib/cpp/concurrency/ThreadManager.h"

#include "folly/io/IOBuf.h"
#include "folly/io/IOBufQueue.h"

#include <bitset>
#include "boost/scoped_array.hpp"
#include <pwd.h>
#include <unistd.h>
#include <chrono>

// Don't include the unknown client.
#define CLIENT_TYPES_LEN 6

// These are local to this build and never appear on the wire.
enum CLIENT_TYPE {
  THRIFT_HEADER_CLIENT_TYPE = 0,
  THRIFT_FRAMED_DEPRECATED = 1,
  THRIFT_UNFRAMED_DEPRECATED = 2,
  THRIFT_HTTP_CLIENT_TYPE = 3,
  THRIFT_FRAMED_COMPACT = 4,
  THRIFT_HEADER_SASL_CLIENT_TYPE = 5,
  THRIFT_UNKNOWN_CLIENT_TYPE = 6,
};

// These appear on the wire.
enum HEADER_FLAGS {
  HEADER_FLAG_SUPPORT_OUT_OF_ORDER = 0x01,
  // stream flags use 2 bits: bits 2 and 3 (mask = 0x06)
  HEADER_FLAG_STREAM_BEGIN = 0x02,
  HEADER_FLAG_STREAM_PIECE = 0x06,
  HEADER_FLAG_STREAM_END = 0x04,
};

enum THRIFT_SECURITY_POLICY {
  THRIFT_SECURITY_DISABLED = 1,
  THRIFT_SECURITY_PERMITTED = 2,
  THRIFT_SECURITY_REQUIRED = 3,
};

namespace apache { namespace thrift { namespace transport {

using apache::thrift::protocol::T_COMPACT_PROTOCOL;

/**
 * Class that will take an IOBuf and wrap it in some thrift headers.
 * see thrift/doc/HeaderFormat.txt for details.
 *
 * Supports transforms: zlib snappy hmac qlz
 * Supports headers: http-style key/value per request and per connection
 * other: Protocol Id and seq ID in header.
 *
 * Backwards compatible with TFramed format, and unframed format, assuming
 * your server transport is compatible (some server types require 4-byte size
 * at the start, such as TNonblockingServer).
 */
class THeader {
 public:

  virtual ~THeader() {}

  explicit THeader()
    : queue_(new folly::IOBufQueue)
    , protoId_(T_COMPACT_PROTOCOL)
    , protoVersion(-1)
    , clientType(THRIFT_UNKNOWN_CLIENT_TYPE)
    , prevClientType(THRIFT_HEADER_CLIENT_TYPE)
    , seqId(0)
    , flags_(0)
    , identity(s_identity)
    , minCompressBytes_(0)
  {
    setSupportedClients(nullptr);
  }

  explicit THeader(std::bitset<CLIENT_TYPES_LEN> const* clientTypes)
    : queue_(new folly::IOBufQueue)
    , protoId_(T_COMPACT_PROTOCOL)
    , protoVersion(-1)
    , clientType(THRIFT_UNKNOWN_CLIENT_TYPE)
    , prevClientType(THRIFT_HEADER_CLIENT_TYPE)
    , seqId(0)
    , flags_(0)
    , identity(s_identity)
    , minCompressBytes_(0)
  {
    setSupportedClients(clientTypes);
  }

  // If clients is nullptr, a security policy of THRIFT_SECURITY_DISABLED
  // will be used.
  void setSupportedClients(std::bitset<CLIENT_TYPES_LEN> const* clients);
  void setSecurityPolicy(THRIFT_SECURITY_POLICY policy);

  void setClientType(CLIENT_TYPE ct);

  uint16_t getClientType() {
    return clientType;
  }

  bool isSupportedClient();
  void checkSupportedClient();

  uint16_t getProtocolId() const;
  void setProtocolId(uint16_t protoId) { this->protoId_ = protoId; }

  int8_t getProtocolVersion() const;

  void setProtocolVersion(uint8_t ver) { this->protoVersion = ver; }

  virtual void resetProtocol();

  uint16_t getFlags() const { return flags_; }
  void setFlags(uint16_t flags) {
    flags_ = flags;
  }

  uint16_t getStreamFlag() const {
    return (getFlags() & STREAM_FLAG_MASK);
  }

  void setStreamFlag(uint16_t streamFlag) {
    clearStreamFlag();
    setFlags(getFlags() | streamFlag);
  }

  void clearStreamFlag() {
    setFlags(getFlags() & ~STREAM_FLAG_MASK);
  }

  /**
   * We know we got a packet in header format here, try to parse the header
   *
   * @param IObuf of the header + data.  Untransforms the data as appropriate.
   * @return Just the data section in an IOBuf
   */
  std::unique_ptr<folly::IOBuf> readHeaderFormat(std::unique_ptr<folly::IOBuf>);

  /**
   * Untransform the data based on the received header flags
   * On conclusion of function, setReadBuffer is called with the
   * untransformed data.
   *
   * @param IOBuf input data section
   * @return IOBuf output data section
   */
  std::unique_ptr<folly::IOBuf> untransform(std::unique_ptr<folly::IOBuf>);

  /**
   * Transform the data based on our write transform flags
   * At conclusion of function the write buffer is set to the
   * transformed data.
   *
   * @param IOBuf to transform.  Returns transformed IOBuf (or chain)
   * @return transformed data IOBuf
   */
  std::unique_ptr<folly::IOBuf> transform(std::unique_ptr<folly::IOBuf>,
                                          std::vector<uint16_t>& transforms);

  uint16_t getNumTransforms(std::vector<uint16_t>& transforms) const {
    int trans = transforms.size();
    if (macCallback_) {
      trans += 1;
    }
    return trans;
  }

  void setTransform(uint16_t transId) {
    for (auto& trans : writeTrans_) {
      if (trans == transId) {
        return;
      }
    }
    writeTrans_.push_back(transId);
  }

  void setTransforms(const std::vector<uint16_t>& trans) {
    writeTrans_ = trans;
  }
  const std::vector<uint16_t>& getTransforms() const { return readTrans_; }

  // Info headers
  typedef std::map<std::string, std::string> StringToStringMap;

  // these work with write headers
  void setHeader(const std::string& key, const std::string& value);
  void setPersistentHeader(const std::string& key, const std::string& value);
  void clearHeaders();
  /**
   * this function only clears the local persistent
   * header. does not affect the persistent header
   * that already set
   */
  void clearPersistentHeaders();

  StringToStringMap& getWriteHeaders() {
    return writeHeaders_;
  }

  StringToStringMap& getPersistentWriteHeaders() {
    return persisWriteHeaders_;
  }

  // these work with read headers
  const StringToStringMap& getHeaders() const {
    return readHeaders_;
  }

  std::string getPeerIdentity();
  void setIdentity(const std::string& identity);

  // accessors for seqId
  int32_t getSequenceNumber() const { return seqId; }
  void setSequenceNumber(int32_t sid) { this->seqId = sid; }

  enum TRANSFORMS {
    NONE = 0x0,
    ZLIB_TRANSFORM = 0x01,
    HMAC_TRANSFORM = 0x02,
    SNAPPY_TRANSFORM = 0x03,
    QLZ_TRANSFORM = 0x04,
    ZLIB_IF_MORE_THAN = 0x05,
  };

  /**
   * Callbacks to get and verify a mac transform.
   *
   * If a mac callback is provided, it will be called with the outgoing packet,
   * with the returned string appended at the end of the data.
   *
   * If a verify callback is provided, all incoming packets will be called with
   * their mac data and packet data to verify.  If false is returned, an
   * exception is thrown. Packets without any mac also throw an exception if a
   * verify function is provided.
   *
   * If no verify callback is provided, and an incoming packet contains a mac,
   * the mac is ignored.
   *
   **/
  typedef std::function<std::string(const std::string&)> MacCallback;
  typedef std::function<
    bool(const std::string&, const std::string)> VerifyMacCallback;

  void setHmac(MacCallback macCb, VerifyMacCallback verifyCb) {
    macCallback_ = macCb;
    verifyCallback_ = verifyCb;
  }

  /* IOBuf interface */

  /**
   * Adds the header based on the type of transport:
   * unframed - does nothing.
   * framed - prepends frame size
   * header - prepends header, optionally appends mac
   * http - only supported for sync case, prepends http header.
   *
   * @return IOBuf chain with header _and_ data.  Data is not copied
   */
  std::unique_ptr<folly::IOBuf> addHeader(std::unique_ptr<folly::IOBuf>);
  /**
   * Given an IOBuf Chain, remove the header.  Supports unframed (sync
   * only), framed, header, and http (sync case only).  This doesn't
   * check if the client type implied by the header is valid.
   * isSupportedClient() or checkSupportedClient() should be called
   * after.
   *
   * @param IOBufQueue - queue to try to read message from.
   *
   * @param needed - if the return is nullptr (i.e. we didn't read a full
   *                 message), needed is set to the number of bytes needed
   *                 before you should call removeHeader again.
   *
   * @return IOBuf - the message chain.  May be shared, may be chained.
   *                 If nullptr, we didn't get enough data for a whole message,
   *                 call removeHeader again after reading needed more bytes.
   */
  std::unique_ptr<folly::IOBuf> removeHeader(folly::IOBufQueue*,
                                             size_t& needed);


  void setMinCompressBytes(uint32_t bytes) {
    minCompressBytes_ = bytes;
  }

  apache::thrift::concurrency::PriorityThreadManager::PRIORITY
  getCallPriority();

  void setCallPriority(
      apache::thrift::concurrency::PriorityThreadManager::PRIORITY prio);

  std::chrono::milliseconds getClientTimeout();

  void setClientTimeout(std::chrono::milliseconds timeout);

 protected:

  void setBestClientType();

  std::bitset<CLIENT_TYPES_LEN> supported_clients;

  std::unique_ptr<folly::IOBufQueue> queue_;

  // 0 and 16th bits must be 0 to differentiate from framed & unframed
  static const uint32_t HEADER_MAGIC = 0x0FFF0000;
  static const uint32_t HEADER_MASK = 0xFFFF0000;
  static const uint32_t FLAGS_MASK = 0x0000FFFF;
  static const uint16_t STREAM_FLAG_MASK = 0x0006;
  static const uint32_t HTTP_MAGIC = 0x504F5354; // POST

  static const uint32_t MAX_FRAME_SIZE = 0x3FFFFFFF;

  int16_t protoId_;
  int8_t protoVersion;
  uint16_t clientType;
  uint16_t prevClientType;
  uint32_t seqId;
  uint16_t flags_;
  std::string identity;

  std::vector<uint16_t> readTrans_;
  std::vector<uint16_t> writeTrans_;

  // Map to use for headers
  StringToStringMap readHeaders_;
  StringToStringMap writeHeaders_;

  // Map to use for persistent headers
  StringToStringMap persisReadHeaders_;
  StringToStringMap persisWriteHeaders_;

  static const std::string IDENTITY_HEADER;
  static const std::string ID_VERSION_HEADER;
  static const std::string ID_VERSION;
  static const std::string PRIORITY_HEADER;
  static const std::string CLIENT_TIMEOUT_HEADER;

  static std::string s_identity;

  MacCallback macCallback_;
  VerifyMacCallback verifyCallback_;

  uint32_t minCompressBytes_;

  /**
   * Returns the maximum number of bytes that write k/v headers can take
   */
  size_t getMaxWriteHeadersSize() const;

  /**
   * Returns whether the 1st byte of the protocol payload should be hadled
   * as compact framed.
   */
  bool compactFramed(uint32_t magic);

  struct infoIdType {
    enum idType {
      // start at 1 to avoid confusing header padding for an infoId
      KEYVALUE = 1,
      // for persistent header
      PKEYVALUE = 2,
      END        // signal the end of infoIds we can handle
    };
  };
};

}}} // apache::thrift::transport

#endif // #ifndef THRIFT_TRANSPORT_THEADER_H_
