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

#include "thrift/lib/cpp/transport/THeader.h"

#include "folly/io/IOBuf.h"
#include "folly/io/Cursor.h"
#include "folly/Conv.h"
#include "folly/String.h"
#include "thrift/lib/cpp/TApplicationException.h"
#include "thrift/lib/cpp/protocol/TProtocolTypes.h"
#include "thrift/lib/cpp/transport/TBufferTransports.h"
#include "thrift/lib/cpp/util/VarintUtils.h"
#include "thrift/lib/cpp/concurrency/Thread.h"
#include "snappy.h"

#ifdef HAVE_QUICKLZ
extern "C" {
#include "external/quicklz-1.5b/quicklz.h"
}
#endif

#include <algorithm>
#include <bitset>
#include <cassert>
#include <string>
#include <zlib.h>

using std::map;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;
using std::pair;

namespace apache { namespace thrift { namespace transport {

using namespace apache::thrift::protocol;
using namespace apache::thrift::util;
using namespace folly;
using namespace folly::io;
using apache::thrift::protocol::TBinaryProtocol;

const string THeader::IDENTITY_HEADER = "identity";
const string THeader::ID_VERSION_HEADER = "id_version";
const string THeader::ID_VERSION = "1";
const string THeader::PRIORITY_HEADER = "thrift_priority";
const string THeader::CLIENT_TIMEOUT_HEADER = "client_timeout";

string THeader::s_identity = "";

void THeader::setSupportedClients(std::bitset<CLIENT_TYPES_LEN>
                                  const* clients) {
  if (clients) {
    supported_clients = *clients;
    // Let's support insecure Header if SASL isn't explicitly supported.
    // It's ok for both to be supported by the caller, too.
    if (!supported_clients[THRIFT_HEADER_SASL_CLIENT_TYPE]) {
      supported_clients[THRIFT_HEADER_CLIENT_TYPE] = true;
    }
    setBestClientType();
  } else {
    setSecurityPolicy(THRIFT_SECURITY_DISABLED);
  }
}

void THeader::setSecurityPolicy(THRIFT_SECURITY_POLICY policy) {
  std::bitset<CLIENT_TYPES_LEN> clients;

  switch (policy) {
    case THRIFT_SECURITY_DISABLED: {
      clients[THRIFT_UNFRAMED_DEPRECATED] = true;
      clients[THRIFT_FRAMED_DEPRECATED] = true;
      clients[THRIFT_HTTP_CLIENT_TYPE] = true;
      clients[THRIFT_HEADER_CLIENT_TYPE] = true;
      clients[THRIFT_FRAMED_COMPACT] = true;
      break;
    }
    case THRIFT_SECURITY_PERMITTED: {
      clients[THRIFT_UNFRAMED_DEPRECATED] = true;
      clients[THRIFT_FRAMED_DEPRECATED] = true;
      clients[THRIFT_HTTP_CLIENT_TYPE] = true;
      clients[THRIFT_HEADER_CLIENT_TYPE] = true;
      clients[THRIFT_HEADER_SASL_CLIENT_TYPE] = true;
      clients[THRIFT_FRAMED_COMPACT] = true;
      break;
    }
    case THRIFT_SECURITY_REQUIRED: {
      clients[THRIFT_HEADER_SASL_CLIENT_TYPE] = true;
      break;
    }
  }

  setSupportedClients(&clients);
}

void THeader::setBestClientType() {
  if (supported_clients[THRIFT_HEADER_SASL_CLIENT_TYPE]) {
    setClientType(THRIFT_HEADER_SASL_CLIENT_TYPE);
  } else {
    setClientType(THRIFT_HEADER_CLIENT_TYPE);
  }
}

void THeader::setClientType(CLIENT_TYPE ct) {
  if (!supported_clients[ct]) {
    throw TApplicationException(
      TApplicationException::UNSUPPORTED_CLIENT_TYPE,
      "Transport does not support this client type");
  }

  clientType = ct;
}

uint16_t THeader::getProtocolId() const {
  if (clientType == THRIFT_HEADER_CLIENT_TYPE ||
      clientType == THRIFT_HEADER_SASL_CLIENT_TYPE) {
    return protoId_;
  } if (clientType == THRIFT_FRAMED_COMPACT) {
    return T_COMPACT_PROTOCOL;
  } else {
    return T_BINARY_PROTOCOL; // Assume other transports use TBinary
  }
}

int8_t THeader::getProtocolVersion() const {
  return protoVersion;
}

bool THeader::compactFramed(uint32_t magic) {
  int8_t protocolId = (magic >> 24);
  int8_t protocolVersion =
    (magic >> 16) & (uint32_t)TCompactProtocol::VERSION_MASK;
  return ((protocolId == TCompactProtocol::PROTOCOL_ID) &&
          (protocolVersion <= TCompactProtocol::VERSION_N) &&
          (protocolVersion >= TCompactProtocol::VERSION_LOW));

}

unique_ptr<IOBuf> THeader::removeHeader(IOBufQueue* queue,
                                                 size_t& needed) {
  Cursor c(queue->front());
  size_t chainSize = queue->front()->computeChainDataLength();
  unique_ptr<IOBuf> buf;
  needed = 0;

  if (chainSize < 4) {
    needed = 4 - chainSize;
    return nullptr;
  }

  // Use first word to check type.
  uint32_t sz = c.readBE<uint32_t>();

  if ((sz & TBinaryProtocol::VERSION_MASK) == TBinaryProtocol::VERSION_1) {
    // unframed
    clientType = THRIFT_UNFRAMED_DEPRECATED;
    const_cast<IOBuf*>(queue->front())->coalesce();

    // Test skip using the protocol to detect the end of the message
    TMemoryBuffer memBuffer(const_cast<uint8_t*>(queue->front()->data()),
                            queue->front()->length(), TMemoryBuffer::OBSERVE);
    TBinaryProtocolT<TBufferBase> proto(&memBuffer);
    uint32_t msgSize = 0;
    try {
      std::string name;
      protocol::TMessageType messageType;
      int32_t seqid;
      msgSize += proto.readMessageBegin(name, messageType, seqid);
      msgSize += protocol::skip(proto, protocol::T_STRUCT);
      msgSize += proto.readMessageEnd();
    } catch (const TTransportException& ex) {
      if (ex.getType() == TTransportException::END_OF_FILE) {
        // We don't have the full data yet.  We can't tell exactly
        // how many bytes we need, but it is at least one.
        needed = 1;
        return nullptr;
      }
    }

    buf = std::move(queue->split(msgSize));
  } else if (sz == HTTP_MAGIC) {
    clientType = THRIFT_HTTP_CLIENT_TYPE;

    // TODO: doesn't work with async case.
    // in sync THeader, wraps this in a THttpTransport.
    // Ideally would use THttpParser directly to support async,
    // won't need to call coalesce.

    buf = queue->move();
  } else {
    if (sz > MAX_FRAME_SIZE) {
      std::string err =
        folly::stringPrintf(
          "Header transport frame is too large: %u (hex 0x%08x",
          sz, sz);
      // does it look like ascii?
      if (  // all bytes 0x7f or less; all are > 0x1F
           ((sz & 0x7f7f7f7f) == sz)
        && ((sz >> 24) & 0xff) >= 0x20
        && ((sz >> 16) & 0xff) >= 0x20
        && ((sz >>  8) & 0xff) >= 0x20
        && ((sz >>  0) & 0xff) >= 0x20) {
        char buffer[5];
        uint32_t *asUint32 = reinterpret_cast<uint32_t *> (buffer);
        *asUint32 = htonl(sz);
        buffer[4] = 0;
        folly::stringAppendf(&err, ", ascii '%s'", buffer);
      }
      folly::stringAppendf(&err, ")");
      throw TTransportException(
        TTransportException::INVALID_FRAME_SIZE,
        err);
    }

    // Make sure we have read the whole frame in.
    if (4 + sz > chainSize) {
      needed = sz - chainSize + 4;
      return nullptr;
    }

    // Could be header format or framed. Check next uint32
    uint32_t magic = c.readBE<uint32_t>();

    if ((magic & TBinaryProtocol::VERSION_MASK) == TBinaryProtocol::VERSION_1) {
      // framed
      clientType = THRIFT_FRAMED_DEPRECATED;

      // Trim off the frame size.
      queue->trimStart(4);
      buf = queue->split(sz);
    } else if (compactFramed(magic)) {
      clientType = THRIFT_FRAMED_COMPACT;
            // Trim off the frame size.
      queue->trimStart(4);
      buf = queue->split(sz);
    } else if (HEADER_MAGIC == (magic & HEADER_MASK)) {
      if (sz < 10) {
        throw TTransportException(
          TTransportException::INVALID_FRAME_SIZE,
          folly::stringPrintf("Header transport frame is too small: %u", sz));
      }

      flags_ = magic & FLAGS_MASK;
      seqId = c.readBE<uint32_t>();

      // Trim off the frame size.
      queue->trimStart(4);
      buf = readHeaderFormat(queue->split(sz));

      // auth client?
      auto auth_header = getHeaders().find("thrift_auth");
      if (auth_header != getHeaders().end() && auth_header->second == "1") {
        clientType = THRIFT_HEADER_SASL_CLIENT_TYPE;
      } else {
        clientType = THRIFT_HEADER_CLIENT_TYPE;
      }
    } else {
      clientType = THRIFT_UNKNOWN_CLIENT_TYPE;
      throw TTransportException(
        TTransportException::BAD_ARGS,
        folly::stringPrintf(
          "Could not detect client transport type: magic 0x%08x",
          magic));
    }
  }

  return std::move(buf);
}

bool THeader::isSupportedClient() {
  return supported_clients[clientType];
}

void THeader::checkSupportedClient() {
  if (!isSupportedClient()) {
    throw TApplicationException(TApplicationException::UNSUPPORTED_CLIENT_TYPE,
                                "Transport does not support this client type");
  }
}

string getString(RWPrivateCursor& c, uint32_t sz) {
  char strdata[sz];
  c.pull(strdata, sz);
  string str(strdata, sz);
  return std::move(str);
}

/**
 * Reads a string from ptr, taking care not to reach headerBoundary
 * Advances ptr on success
 *
 * @param RWPrivateCursor        cursor to read from
 */
string readString(RWPrivateCursor& c) {
  return getString(c, readVarint<uint32_t>(c));
}

void readInfoHeaders(RWPrivateCursor& c,
                     THeader::StringToStringMap &headers_) {
  // Process key-value headers
  uint32_t numKVHeaders = readVarint<int32_t>(c);
  // continue until we reach (paded) end of packet
  while (numKVHeaders--) {
    // format: key; value
    // both: length (varint32); value (string)
    string key = readString(c);
    string value = readString(c);
    // save to headers
    headers_[key] = value;
  }
}

unique_ptr<IOBuf> THeader::readHeaderFormat(unique_ptr<IOBuf> buf) {
  readTrans_.clear(); // Clear out any previous transforms.
  readHeaders_.clear(); // Clear out any previous headers.

  // magic(4), seqId(2), flags(2), headerSize(2)
  const uint8_t commonHeaderSize = 10;

  RWPrivateCursor c(buf.get());

  // skip over already processed magic(4), seqId(4), headerSize(2)
  c += commonHeaderSize - 2; // advance to headerSize field
  // On the wire, headerSize is in 4 byte words.  See HeaderFormat.txt
  uint32_t headerSize = 4 * c.readBE<uint16_t>() + commonHeaderSize;
  if (headerSize > buf->computeChainDataLength()) {
    throw TTransportException(TTransportException::INVALID_FRAME_SIZE,
                              "Header size is larger than frame");
  }
  Cursor data(buf.get());
  data += headerSize;
  protoId_ = readVarint<uint16_t>(c);
  int16_t numTransforms = readVarint<uint16_t>(c);

  uint16_t macSz = 0;

  // For now all transforms consist of only the ID, not data.
  for (int i = 0; i < numTransforms; i++) {
    int32_t transId = readVarint<int32_t>(c);

    if (transId == HMAC_TRANSFORM) {
      RWPrivateCursor macCursor(c);
      macSz = c.read<uint8_t>();
      macCursor.write<uint8_t>(0x00);
    } else if(transId == ZLIB_IF_MORE_THAN) {
      setTransform(ZLIB_TRANSFORM);
      minCompressBytes_ = c.readBE<uint32_t>();
    } else {
      readTrans_.push_back(transId);
    }
  }

  // Info headers
  while (data.data() != c.data()) {
    uint32_t infoId = readVarint<int32_t>(c);

    if (infoId == 0) {
      // header padding
      break;
    }
    if (infoId >= infoIdType::END) {
      // cannot handle infoId
      break;
    }
    switch (infoId) {
      case infoIdType::KEYVALUE:
        readInfoHeaders(c, readHeaders_);
        break;
      case infoIdType::PKEYVALUE:
        readInfoHeaders(c, persisReadHeaders_);
        break;
    }
  }

  // if persistent headers are not empty, merge together.
  if (!persisReadHeaders_.empty())
    readHeaders_.insert(persisReadHeaders_.begin(), persisReadHeaders_.end());

  if (verifyCallback_) {
    uint32_t bufLength = buf->computeChainDataLength();
    RWPrivateCursor macCursor(buf.get());

    // Mac callbacks don't have a zero-copy interface
    string verify_data = getString(macCursor, bufLength - macSz);
    string mac = getString(macCursor, macSz);

    if (!verifyCallback_(verify_data, mac)) {
      if (macSz > 0) {
        throw TTransportException(TTransportException::INVALID_STATE,
                                  "mac did not verify");
      } else {
        throw TTransportException(TTransportException::INVALID_STATE,
                                  "Client did not send a mac");
      }
    }
  }

  // Get just the data section using trim on a queue
  unique_ptr<IOBufQueue> msg(new IOBufQueue);
  msg->append(std::move(buf));
  msg->trimStart(headerSize);
  msg->trimEnd(macSz);

  buf = msg->move();
  // msg->move() can return an empty pointer if all the data is
  // trimmed out.  Turn it back into an empty buf.
  if (!buf) {
    buf = IOBuf::create(0);
  }

  // Untransform data section
  buf = untransform(std::move(buf));

  if (protoId_ == T_JSON_PROTOCOL && clientType != THRIFT_HTTP_CLIENT_TYPE) {
    throw TApplicationException(TApplicationException::UNSUPPORTED_CLIENT_TYPE,
                                "Client is trying to send JSON without HTTP");
  }

  return std::move(buf);
}

unique_ptr<IOBuf> THeader::untransform(unique_ptr<IOBuf> buf) {
  for (vector<uint16_t>::const_reverse_iterator it = readTrans_.rbegin();
       it != readTrans_.rend(); ++it) {
    const uint16_t transId = *it;

    if (transId == ZLIB_TRANSFORM) {
      size_t bufSize = 1024;
      unique_ptr<IOBuf> out;

      z_stream stream;
      int err;

      // Setting these to 0 means use the default free/alloc functions
      stream.zalloc = (alloc_func)0;
      stream.zfree = (free_func)0;
      stream.opaque = (voidpf)0;
      err = inflateInit(&stream);
      if (err != Z_OK) {
        throw TApplicationException(TApplicationException::MISSING_RESULT,
                                    "Error while zlib inflate Init");
      }
      do {
        if (nullptr == buf) {
          throw TApplicationException(TApplicationException::MISSING_RESULT,
                                      "Not enough zlib data in message");
        }
        stream.next_in = buf->writableData();
        stream.avail_in = buf->length();
        do {
          unique_ptr<IOBuf> tmp(IOBuf::create(bufSize));

          stream.next_out = tmp->writableData();
          stream.avail_out = bufSize;
          err = inflate(&stream, Z_NO_FLUSH);
          if (err == Z_STREAM_ERROR ||
             err == Z_DATA_ERROR ||
             err == Z_MEM_ERROR) {
            throw TApplicationException(TApplicationException::MISSING_RESULT,
                                        "Error while zlib inflate");
          }
          tmp->append(bufSize - stream.avail_out);
          if (out) {
            // Add buffer to end (circular list, same as prepend)
            out->prependChain(std::move(tmp));
          } else {
            out = std::move(tmp);
          }
        } while (stream.avail_out == 0);
        // try the next buffer
        buf = buf->pop();
      } while (err != Z_STREAM_END);

      err = inflateEnd(&stream);
      if (err != Z_OK) {
        throw TApplicationException(TApplicationException::MISSING_RESULT,
                                    "Error while zlib inflateEnd");
      }

      buf = std::move(out);
    } else if (transId == SNAPPY_TRANSFORM) {
      buf->coalesce(); // required for snappy uncompression
      size_t uncompressed_sz;
      bool result = snappy::GetUncompressedLength((char*)buf->data(),
                                                  buf->length(),
                                                  &uncompressed_sz);
      unique_ptr<IOBuf> out(IOBuf::create(uncompressed_sz));
      out->append(uncompressed_sz);

      result = snappy::RawUncompress((char*)buf->data(), buf->length(),
                                     (char*)out->writableData());
      if (!result) {
        throw TApplicationException(TApplicationException::MISSING_RESULT,
                                    "snappy uncompress failure");
      }

      buf = std::move(out);
    } else if (transId == QLZ_TRANSFORM) {
      buf->coalesce(); // probably needed for uncompression
      const char *src = (const char *)buf->data();
      size_t length = buf->length();
#ifdef HAVE_QUICKLZ
      // according to QLZ spec, the size info is stored in first 9 bytes
      if (length < 9 || qlz_size_compressed(src) != length) {
        throw TApplicationException(TApplicationException::MISSING_RESULT,
                                    "Error in qlz decompress: bad size");
      }

      size_t uncompressed_sz = qlz_size_decompressed(src);
      const unique_ptr<qlz_state_decompress> state(new qlz_state_decompress);

      unique_ptr<IOBuf> out(IOBuf::create(uncompressed_sz));
      out->append(uncompressed_sz);

      bool success = (qlz_decompress(src,
                                     out->writableData(),
                                     state.get()) == uncompressed_sz);
      if (!success) {
        throw TApplicationException(TApplicationException::MISSING_RESULT,
                                    "Error in qlz decompress");
      }
      buf = std::move(out);
#endif

    } else {
      throw TApplicationException(TApplicationException::MISSING_RESULT,
                                "Unknown transform");
    }
  }

  return std::move(buf);
}

unique_ptr<IOBuf> THeader::transform(unique_ptr<IOBuf> buf,
                                     std::vector<uint16_t>& writeTrans) {
  // TODO(davejwatson) look at doing these as stream operations on write
  // instead of memory buffer operations.  Would save a memcpy.
  uint32_t dataSize = buf->computeChainDataLength();

  for (vector<uint16_t>::iterator it = writeTrans.begin();
       it != writeTrans.end(); ) {
    const uint16_t transId = *it;

    if (transId == ZLIB_IF_MORE_THAN) {
      // Applies only to receiver, do nothing.
    } else if (transId == ZLIB_TRANSFORM) {
      if (dataSize < minCompressBytes_) {
        it = writeTrans.erase(it);
        continue;
      }
      size_t bufSize = 1024;
      unique_ptr<IOBuf> out;

      z_stream stream;
      int err;

      stream.next_in = (unsigned char*)buf->data();
      stream.avail_in = buf->length();

      stream.zalloc = (alloc_func)0;
      stream.zfree = (free_func)0;
      stream.opaque = (voidpf)0;
      err = deflateInit(&stream, Z_DEFAULT_COMPRESSION);
      if (err != Z_OK) {
        throw TTransportException(TTransportException::CORRUPTED_DATA,
                                  "Error while zlib deflateInit");
      }

      // Loop until deflate() tells us it's done writing all output
      while (err != Z_STREAM_END) {
        // Create a new output chunk
        unique_ptr<IOBuf> tmp(IOBuf::create(bufSize));
        stream.next_out = tmp->writableData();
        stream.avail_out = bufSize;

        // Loop while the current output chunk still has space, call deflate to
        // try and fill it
        while (stream.avail_out > 0) {
          // When providing the last bit of input data and thereafter, pass
          // Z_FINISH to tell zlib it should flush out remaining compressed
          // data and finish up with an end marker at the end of the output
          // stream
          int flush = (buf && buf->isChained()) ? Z_NO_FLUSH : Z_FINISH;
          err = deflate(&stream, flush);
          if (err == Z_STREAM_ERROR) {
            throw TTransportException(TTransportException::CORRUPTED_DATA,
                                      "Error while zlib deflate");
          }

          if (stream.avail_in == 0) {
            if (buf) {
              buf = buf->pop();
            }
            if (!buf) {
              // No more input chunks left
              break;
            }
            // Prvoide the next input chunk to zlib
            stream.next_in = (unsigned char*) buf->data();
            stream.avail_in = buf->length();
          }
        }

        // Tell the tmp IOBuf we wrote some data into it
        tmp->append(bufSize - stream.avail_out);
        if (out) {
          // Add the IOBuf to the end of the chain
          out->prependChain(std::move(tmp));
        } else {
          // This is the first IOBuf, so start the chain
          out = std::move(tmp);
        }
      }

      err = deflateEnd(&stream);
      if (err != Z_OK) {
        throw TTransportException(TTransportException::CORRUPTED_DATA,
                                  "Error while zlib deflateEnd");
      }

      buf = std::move(out);
    } else if (transId == SNAPPY_TRANSFORM) {
      if (dataSize < minCompressBytes_) {
        it = writeTrans.erase(it);
        continue;
      }

      // Check that we have enough space
      size_t maxCompressedLength = snappy::MaxCompressedLength(buf->length());
      unique_ptr<IOBuf> out(IOBuf::create(maxCompressedLength));

      size_t compressed_sz;
      snappy::RawCompress((char*)buf->data(), buf->length(),
                          (char*)out->writableData(), &compressed_sz);
      out->append(compressed_sz);
      buf = std::move(out);
    } else if (transId == QLZ_TRANSFORM) {
      if (dataSize < minCompressBytes_) {
        it = writeTrans.erase(it);
        continue;
      }

      // max is 400B greater than uncompressed size based on QuickLZ spec
      size_t maxCompressedLength = buf->length() + 400;
      unique_ptr<IOBuf> out(IOBuf::create(maxCompressedLength));

#ifdef HAVE_QUICKLZ
      const unique_ptr<qlz_state_compress> state(new qlz_state_compress);

      const char *src = (const char *)buf->data();
      char *dst = (char *)out->writableData();

      size_t compressed_sz = qlz_compress(
        src, dst, buf->length(), state.get());

      if (compressed_sz > maxCompressedLength) {
        throw TTransportException(TTransportException::CORRUPTED_DATA,
                                  "Error in qlz compress");
      }

      out->append(compressed_sz);
#endif
      buf = std::move(out);
    } else {
      throw TTransportException(TTransportException::CORRUPTED_DATA,
                                "Unknown transform");
    }
    ++it;
  }

  return std::move(buf);
}

void THeader::resetProtocol() {
  // Set to anything except HTTP type so we don't flush again
  clientType = THRIFT_HEADER_CLIENT_TYPE;
}

/**
 * Writes a string to a byte buffer, as size (varint32) + string (non-null
 * terminated)
 * Automatically advances ptr to after the written portion
 */
void writeString(uint8_t* &ptr, const string& str) {
  uint32_t strLen = str.length();
  ptr += writeVarint32(strLen, ptr);
  memcpy(ptr, str.c_str(), strLen); // no need to write \0
  ptr += strLen;
}

/**
 * Writes headers to a byte buffer and clear the header map
 */
void flushInfoHeaders(uint8_t* &pkt,
                      THeader::StringToStringMap &headers_,
                      uint32_t infoIdType) {
    uint32_t headerCount = headers_.size();
    if (headerCount > 0) {
      pkt += writeVarint32(infoIdType, pkt);
      // Write key-value headers count
      pkt += writeVarint32(headerCount, pkt);
      // Write info headers
      map<string, string>::const_iterator it;
      for (it = headers_.begin(); it != headers_.end(); ++it) {
        writeString(pkt, it->first);  // key
        writeString(pkt, it->second); // value
      }
      headers_.clear();
    }
}

void THeader::setHeader(const string& key, const string& value) {
  writeHeaders_[key] = value;
}

void THeader::setPersistentHeader(const string& key,
                                           const string& value) {
  persisWriteHeaders_[key] = value;
}

size_t getInfoHeaderSize(const THeader::StringToStringMap &headers_) {
  size_t maxWriteHeadersSize = 0;
  for (auto& it : headers_) {
    // add sizes of key and value to maxWriteHeadersSize
    // 2 varints32 + the strings themselves
    maxWriteHeadersSize += 5 + 5 + (it.first).length() +
      (it.second).length();
  }
  return maxWriteHeadersSize;
}

size_t THeader::getMaxWriteHeadersSize() const {
  size_t maxWriteHeadersSize = 0;
  maxWriteHeadersSize += getInfoHeaderSize(persisWriteHeaders_);
  maxWriteHeadersSize += getInfoHeaderSize(writeHeaders_);
  return maxWriteHeadersSize;
}

void THeader::clearHeaders() {
  writeHeaders_.clear();
}

void THeader::clearPersistentHeaders() {
  persisWriteHeaders_.clear();
}

string THeader::getPeerIdentity() {
  if (readHeaders_.find(IDENTITY_HEADER) != readHeaders_.end()) {
    if (readHeaders_[ID_VERSION_HEADER] == ID_VERSION) {
      return readHeaders_[IDENTITY_HEADER];
    }
  }
  return "";
}

void THeader::setIdentity(const string& identity) {
  this->identity = identity;
}

unique_ptr<IOBuf> THeader::addHeader(unique_ptr<IOBuf> buf) {
  // We may need to modify some transforms before send.  Make
  // a copy here
  std::vector<uint16_t> writeTrans = writeTrans_;

  if (clientType == THRIFT_HEADER_CLIENT_TYPE ||
      clientType == THRIFT_HEADER_SASL_CLIENT_TYPE) {
    buf = transform(std::move(buf), writeTrans);
  }
  size_t chainSize = buf->computeChainDataLength();

  if (protoId_ == T_JSON_PROTOCOL && clientType != THRIFT_HTTP_CLIENT_TYPE) {
    throw TTransportException(TTransportException::BAD_ARGS,
                              "Trying to send JSON without HTTP");
  }

  if (chainSize > MAX_FRAME_SIZE) {
    throw TTransportException(TTransportException::INVALID_FRAME_SIZE,
                              "Attempting to send frame that is too large");
  }

  // Add in special flags
  // All flags must be added before any calls to getMaxWriteHeadersSize
  if (identity.length() > 0) {
    writeHeaders_[IDENTITY_HEADER] = identity;
    writeHeaders_[ID_VERSION_HEADER] = ID_VERSION;
  }

  if (clientType == THRIFT_HEADER_CLIENT_TYPE ||
      clientType == THRIFT_HEADER_SASL_CLIENT_TYPE) {
    // We set a persistent header so we don't have to include a header in every
    // message on the wire. If clientType changes from last used (e.g. SASL
    // client connects to SASL-disabled server and falls back to non-SASL),
    // replace the header.
    if (prevClientType != clientType) {
      if (clientType == THRIFT_HEADER_SASL_CLIENT_TYPE) {
        setPersistentHeader("thrift_auth", "1");
      } else {
        setPersistentHeader("thrift_auth", "0");
      }
      prevClientType = clientType;
    }
    // header size will need to be updated at the end because of varints.
    // Make it big enough here for max varint size, plus 4 for padding.
    int headerSize = (2 + getNumTransforms(writeTrans) * 2 /* transform data */)
      * 5 + 4;
    // add approximate size of info headers
    headerSize += getMaxWriteHeadersSize();

    // Pkt size
    uint32_t maxSzHbo = headerSize + chainSize // thrift header + payload
                        + 10;                  // common header section
    unique_ptr<IOBuf> header = IOBuf::create(headerSize + 10);
    uint8_t* pkt = header->writableData();
    uint8_t* headerStart;
    uint8_t* headerSizePtr;
    uint8_t* pktStart = pkt;

    uint32_t szHbo;
    uint32_t szNbo;
    uint16_t headerSizeN;

    // Fixup szHbo later
    pkt += sizeof(szNbo);
    uint16_t magicN = htons(HEADER_MAGIC >> 16);
    memcpy(pkt, &magicN, sizeof(magicN));
    pkt += sizeof(magicN);
    uint16_t flagsN = htons(flags_);
    memcpy(pkt, &flagsN, sizeof(flagsN));
    pkt += sizeof(flagsN);
    uint32_t seqIdN = htonl(seqId);
    memcpy(pkt, &seqIdN, sizeof(seqIdN));
    pkt += sizeof(seqIdN);
    headerSizePtr = pkt;
    // Fixup headerSizeN later
    pkt += sizeof(headerSizeN);
    headerStart = pkt;

    pkt += writeVarint32(protoId_, pkt);
    pkt += writeVarint32(getNumTransforms(writeTrans), pkt);

    for (auto& transId : writeTrans) {
      pkt += writeVarint32(transId, pkt);
      if(transId == ZLIB_IF_MORE_THAN) {
        uint32_t minCompressN = htonl(minCompressBytes_);
        memcpy(pkt, &minCompressN, sizeof(minCompressN));
        pkt += sizeof(minCompressN);
      }

    }

    uint8_t* mac_loc = nullptr;
    if (macCallback_) {
      pkt += writeVarint32(HMAC_TRANSFORM, pkt);
      mac_loc = pkt;
      *pkt = 0x00;
      pkt++;
    }

    // write info headers

    // write persistent kv-headers
    flushInfoHeaders(pkt, persisWriteHeaders_, infoIdType::PKEYVALUE);

    //write non-persistent kv-headers
    flushInfoHeaders(pkt, writeHeaders_, infoIdType::KEYVALUE);

    // TODO(davejwatson) optimize this for writing twice/memcopy to pkt buffer.
    // See code in TBufferTransports

    // Fixups after varint size calculations
    headerSize = (pkt - headerStart);
    uint8_t padding = 4 - (headerSize % 4);
    headerSize += padding;

    // Pad out pkt with 0x00
    for (int i = 0; i < padding; i++) {
      *(pkt++) = 0x00;
    }

    // Pkt size
    szHbo = headerSize + chainSize           // thrift header + payload
            + (headerStart - pktStart - 4);  // common header section
    headerSizeN = htons(headerSize / 4);
    memcpy(headerSizePtr, &headerSizeN, sizeof(headerSizeN));

    // hmac calculation should always be last.
    string hmac;
    if (macCallback_) {
      // TODO(davejwatson): refactor macCallback_ interface to take
      // several uint8_t buffers instead of string to avoid the extra copying.

      buf->coalesce(); // Needs to be coalesced for string data anyway.
      // Ignoring 4 bytes of framing.
      string headerData(reinterpret_cast<char*>(pktStart + 4),
                        szHbo - chainSize);
      string data(reinterpret_cast<char*>(buf->writableData()), chainSize);
      hmac = macCallback_(headerData + data);
      *mac_loc = hmac.length(); // Set mac size.
      szHbo += hmac.length();
    }

    // Set framing size.
    szNbo = htonl(szHbo);
    memcpy(pktStart, &szNbo, sizeof(szNbo));

    header->append(szHbo - chainSize + 4 - hmac.length());
    header->prependChain(std::move(buf));
    buf = std::move(header);
    if (hmac.length() > 0) {
      unique_ptr<IOBuf> hmacBuf(IOBuf::wrapBuffer(hmac.data(), hmac.length()));
      buf->prependChain(std::move(hmacBuf));
    }
  } else if ((clientType == THRIFT_FRAMED_DEPRECATED) ||
             (clientType == THRIFT_FRAMED_COMPACT)){
    uint32_t szHbo = (uint32_t)chainSize;
    uint32_t szNbo = htonl(szHbo);

    unique_ptr<IOBuf> header = IOBuf::create(4);
    header->append(4);
    memcpy(header->writableData(), &szNbo, 4);
    header->prependChain(std::move(buf));
    buf = std::move(header);
  } else if (clientType == THRIFT_UNFRAMED_DEPRECATED ||
             clientType == THRIFT_HTTP_CLIENT_TYPE) {
    // We just return buf
    // TODO: IOBufize httpTransport.
  } else {
    throw TTransportException(TTransportException::BAD_ARGS,
                              "Unknown client type");
  }

  return std::move(buf);
}

apache::thrift::concurrency::PriorityThreadManager::PRIORITY
THeader::getCallPriority() {
  const auto& map = getHeaders();
  auto iter = map.find(PRIORITY_HEADER);
  if (iter != map.end()) {
    try {
      unsigned prio = folly::to<unsigned>(iter->second);
      if (prio < apache::thrift::concurrency::N_PRIORITIES) {
        return static_cast<apache::thrift::concurrency::PRIORITY>(prio);
      }
    }
    catch (const std::range_error&) {}
    LOG(INFO) << "Bad method priority " << iter->second << ", using default";
  }
  // no priority
  return apache::thrift::concurrency::N_PRIORITIES;
}

void THeader::setCallPriority(
    apache::thrift::concurrency::PriorityThreadManager::PRIORITY prio) {
  setHeader(PRIORITY_HEADER, folly::to<std::string>(prio));
}

std::chrono::milliseconds THeader::getClientTimeout() {
  const auto& map = getHeaders();
  auto iter = map.find(CLIENT_TIMEOUT_HEADER);
  if (iter != map.end()) {
    try {
      int64_t timeout = folly::to<int64_t>(iter->second);
      return std::chrono::milliseconds(timeout);
    } catch (const std::range_error&) {}
    LOG(INFO) << "Bad client timeout " << iter->second << ", using default";
  }

  return std::chrono::milliseconds(0);
}

void THeader::setClientTimeout(std::chrono::milliseconds timeout) {
  setHeader(CLIENT_TIMEOUT_HEADER, folly::to<std::string>(timeout.count()));
}

}}} // apache::thrift::transport
