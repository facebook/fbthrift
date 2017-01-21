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

#ifndef THRIFT_TRANSPORT_THEADER_H_
#define THRIFT_TRANSPORT_THEADER_H_ 1

#include <functional>
#include <map>
#include <vector>

#include <folly/Optional.h>
#include <folly/String.h>
#include <folly/portability/Unistd.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/concurrency/Thread.h>

#include <bitset>
#include <chrono>

// Don't include the unknown client.
#define CLIENT_TYPES_LEN 7

// These are local to this build and never appear on the wire.
enum CLIENT_TYPE {
  THRIFT_HEADER_CLIENT_TYPE = 0,
  THRIFT_FRAMED_DEPRECATED = 1,
  THRIFT_UNFRAMED_DEPRECATED = 2,
  THRIFT_HTTP_SERVER_TYPE = 3,
  THRIFT_HTTP_CLIENT_TYPE = 4,
  THRIFT_FRAMED_COMPACT = 5,
  THRIFT_HEADER_SASL_CLIENT_TYPE = 6,
  THRIFT_HTTP_GET_CLIENT_TYPE = 7,
  THRIFT_UNKNOWN_CLIENT_TYPE = 8,
  THRIFT_UNFRAMED_COMPACT_DEPRECATED = 9,
};

// These appear on the wire.
enum HEADER_FLAGS {
  HEADER_FLAG_SUPPORT_OUT_OF_ORDER = 0x01,
  // Set for reverse messages (server->client requests, client->server replies)
  HEADER_FLAG_DUPLEX_REVERSE = 0x08,
  HEADER_FLAG_SASL = 0x10,
};

namespace folly {
class IOBuf;
class IOBufQueue;
}

namespace apache { namespace thrift { namespace util {
class THttpClientParser;
}}}

namespace apache { namespace thrift { namespace transport {

using apache::thrift::protocol::T_COMPACT_PROTOCOL;
using apache::thrift::protocol::T_BINARY_PROTOCOL;

/**
 * Class that will take an IOBuf and wrap it in some thrift headers.
 * see thrift/doc/HeaderFormat.txt for details.
 *
 * Supports transforms: zlib snappy zstd
 * Supports headers: http-style key/value per request and per connection
 * other: Protocol Id and seq ID in header.
 *
 * Backwards compatible with TFramed format, and unframed format, assuming
 * your server transport is compatible (some server types require 4-byte size
 * at the start).
 */
class THeader {
 public:

  virtual ~THeader();

  enum {
    ALLOW_BIG_FRAMES = 1 << 0,
  };

  explicit THeader(int options = 0);

  virtual void setClientType(CLIENT_TYPE ct) { this->clientType = ct; }
  // Force using specified client type when using legacy client types
  // i.e. sniffing out client type is disabled.
  void forceClientType(bool enable) { forceClientType_ = enable; }
  CLIENT_TYPE getClientType() const { return clientType; }

  uint16_t getProtocolId() const { return protoId_;}
  void setProtocolId(uint16_t protoId) { this->protoId_ = protoId; }

  int8_t getProtocolVersion() const;
  void setProtocolVersion(uint8_t ver) { this->protoVersion = ver; }

  virtual void resetProtocol();

  uint16_t getFlags() const { return flags_; }
  void setFlags(uint16_t flags) {
    flags_ = flags;
  }

  // Info headers
  typedef std::map<std::string, std::string> StringToStringMap;

  /**
   * We know we got a packet in header format here, try to parse the header
   *
   * @param IObuf of the header + data.  Untransforms the data as appropriate.
   * @return Just the data section in an IOBuf
   */
  std::unique_ptr<folly::IOBuf> readHeaderFormat(
    std::unique_ptr<folly::IOBuf>,
    StringToStringMap& persistentReadHeaders);

  /**
   * Untransform the data based on the received header flags
   * On conclusion of function, setReadBuffer is called with the
   * untransformed data.
   *
   * @param IOBuf input data section
   * @return IOBuf output data section
   */
  static std::unique_ptr<folly::IOBuf> untransform(
    std::unique_ptr<folly::IOBuf>,
    std::vector<uint16_t>& readTrans);

  /**
   * Transform the data based on our write transform flags
   * At conclusion of function the write buffer is set to the
   * transformed data.
   *
   * @param IOBuf to transform.  Returns transformed IOBuf (or chain)
   * @return transformed data IOBuf
   */
  static std::unique_ptr<folly::IOBuf> transform(
    std::unique_ptr<folly::IOBuf>,
    std::vector<uint16_t>& writeTrans,
    size_t minCompressBytes);

  /**
   * Clone a new THeader. Metadata is copied, but not headers.
   */
  std::unique_ptr<THeader> clone();

  static uint16_t getNumTransforms(const std::vector<uint16_t>& transforms) {
    return transforms.size();
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
  std::vector<uint16_t>& getWriteTransforms() { return writeTrans_; }

  // these work with write headers
  void setHeader(const std::string& key, const std::string& value);
  void setHeader(const char* key, size_t keyLength, const char* value,
                 size_t valueLength);
  void setHeaders(StringToStringMap&&);
  void clearHeaders();
  bool isWriteHeadersEmpty() {
    return writeHeaders_.empty();
  }

  StringToStringMap&& releaseWriteHeaders() {
    return std::move(writeHeaders_);
  }

  // these work with read headers
  void setReadHeaders(StringToStringMap&&);
  void eraseReadHeader(const std::string& key);
  const StringToStringMap& getHeaders() const { return readHeaders_; }

  StringToStringMap releaseHeaders() {
    StringToStringMap headers;
    readHeaders_.swap(headers);
    return headers;
  }

  void setExtraWriteHeaders(StringToStringMap* extraWriteHeaders) {
    extraWriteHeaders_ = extraWriteHeaders;
  }

  std::string getPeerIdentity();
  void setIdentity(const std::string& identity);

  // accessors for seqId
  uint32_t getSequenceNumber() const { return seqId; }
  void setSequenceNumber(uint32_t sid) { this->seqId = sid; }

  enum TRANSFORMS {
    NONE = 0x00,
    ZLIB_TRANSFORM = 0x01,
    HMAC_TRANSFORM = 0x02,         // Deprecated and no longer supported
    SNAPPY_TRANSFORM = 0x03,
    QLZ_TRANSFORM = 0x04,          // Deprecated and no longer supported
    ZSTD_TRANSFORM = 0x05,

    // DO NOT USE. Sentinel value for enum count. Always keep as last value.
    TRANSFORM_LAST_FIELD = 0x06,
  };

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
  std::unique_ptr<folly::IOBuf> addHeader(
    std::unique_ptr<folly::IOBuf>,
    StringToStringMap& persistentWriteHeaders,
    bool transform=true);
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
  std::unique_ptr<folly::IOBuf> removeHeader(
    folly::IOBufQueue*,
    size_t& needed,
    StringToStringMap& persistentReadHeaders);

  void setMinCompressBytes(uint32_t bytes) {
    minCompressBytes_ = bytes;
  }

  uint32_t getMinCompressBytes() {
    return minCompressBytes_;
  }

  apache::thrift::concurrency::PRIORITY getCallPriority();

  std::chrono::milliseconds getTimeoutFromHeader(
    const std::string header
  ) const;

  std::chrono::milliseconds getClientTimeout() const;

  std::chrono::milliseconds getClientQueueTimeout() const;

  void setHttpClientParser(
      std::shared_ptr<apache::thrift::util::THttpClientParser>);

  // Utility method for converting TRANSFORMS enum to string
  static const folly::StringPiece getStringTransform(
      const TRANSFORMS transform);

  static CLIENT_TYPE getClientType(uint32_t f, uint32_t s);

  // 0 and 16th bits must be 0 to differentiate from framed & unframed
  static const uint32_t HEADER_MAGIC = 0x0FFF0000;
  static const uint32_t HEADER_MASK = 0xFFFF0000;
  static const uint32_t FLAGS_MASK = 0x0000FFFF;
  static const uint32_t HTTP_SERVER_MAGIC = 0x504F5354; // POST
  static const uint32_t HTTP_CLIENT_MAGIC = 0x48545450; // HTTP
  static const uint32_t HTTP_GET_CLIENT_MAGIC = 0x47455420; // GET
  static const uint32_t HTTP_HEAD_CLIENT_MAGIC = 0x48454144; // HEAD
  static const uint32_t BIG_FRAME_MAGIC = 0x42494746;  // BIGF

  static const uint32_t MAX_FRAME_SIZE = 0x3FFFFFFF;
  static const std::string PRIORITY_HEADER;
  static const std::string& CLIENT_TIMEOUT_HEADER;
  static const std::string QUEUE_TIMEOUT_HEADER;

 protected:
  bool isFramed(CLIENT_TYPE clientType);

  // Use first 64 bits to determine client protocol
  static folly::Optional<CLIENT_TYPE> analyzeFirst32bit(uint32_t w);
  static CLIENT_TYPE analyzeSecond32bit(uint32_t w);

  // Calls appropriate method based on client type
  // returns nullptr if Header of Unknown type
  std::unique_ptr<folly::IOBuf> removeNonHeader(folly::IOBufQueue* queue,
                                                size_t& needed,
                                                CLIENT_TYPE clientType,
                                                uint32_t sz);

  template<template <class BaseProt> class ProtocolClass,
           protocol::PROTOCOL_TYPES ProtocolID>
  std::unique_ptr<folly::IOBuf> removeUnframed(folly::IOBufQueue* queue,
                                               size_t& needed);
  std::unique_ptr<folly::IOBuf> removeHttpServer(folly::IOBufQueue* queue);
  std::unique_ptr<folly::IOBuf> removeHttpClient(folly::IOBufQueue* queue,
                                                 size_t& needed);
  std::unique_ptr<folly::IOBuf> removeFramed(uint32_t sz,
                                             folly::IOBufQueue* queue);

  void setBestClientType();

  std::unique_ptr<folly::IOBufQueue> queue_;

  // Http client parser
  std::shared_ptr<apache::thrift::util::THttpClientParser> httpClientParser_;

  int16_t protoId_;
  int8_t protoVersion;
  CLIENT_TYPE clientType;
  bool forceClientType_;
  uint32_t seqId;
  uint16_t flags_;
  std::string identity;

  std::vector<uint16_t> readTrans_;
  std::vector<uint16_t> writeTrans_;

  // Map to use for headers
  StringToStringMap readHeaders_;
  StringToStringMap writeHeaders_;

  // Won't be cleared when flushing
  StringToStringMap* extraWriteHeaders_{nullptr};

  static const std::string IDENTITY_HEADER;
  static const std::string ID_VERSION_HEADER;
  static const std::string ID_VERSION;

  uint32_t minCompressBytes_;
  bool allowBigFrames_;

  /**
   * Returns the maximum number of bytes that write k/v headers can take
   */
  size_t getMaxWriteHeadersSize(
    const StringToStringMap& persistentWriteHeaders) const;

  /**
   * Returns whether the 1st byte of the protocol payload should be hadled
   * as compact framed.
   */
  static bool compactFramed(uint32_t magic);

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
