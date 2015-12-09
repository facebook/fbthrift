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

#include <thrift/lib/cpp/transport/THeader.h>

#include <folly/io/IOBuf.h>
#include <folly/io/Cursor.h>
#include <folly/Conv.h>
#include <folly/String.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/util/VarintUtils.h>
#include <thrift/lib/cpp/concurrency/Thread.h>
#include "snappy.h"
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/TCompactProtocol.h>
#include <thrift/lib/cpp/util/THttpParser.h>

#ifdef HAVE_QUICKLZ
extern "C" {
#include <quicklz.h> // nolint
}
#endif

#include <algorithm>
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

static const string THRIFT_AUTH_HEADER = "thrift_auth";

THeader::THeader(int options)
  : queue_(new folly::IOBufQueue)
  , protoId_(T_COMPACT_PROTOCOL)
  , protoVersion(-1)
  , clientType(THRIFT_HEADER_CLIENT_TYPE)
  , forceClientType_(false)
  , seqId(0)
  , flags_(0)
  , identity(s_identity)
  , minCompressBytes_(0)
  , allowBigFrames_(options & ALLOW_BIG_FRAMES) {}

THeader::~THeader() {}

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

unique_ptr<IOBuf> THeader::removeUnframed(
    IOBufQueue* queue,
    size_t& needed) {
  protoId_ = T_BINARY_PROTOCOL;
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

  return queue->split(msgSize);
}

unique_ptr<IOBuf> THeader::removeHttpServer(IOBufQueue* queue) {
  protoId_ = T_BINARY_PROTOCOL;
  // Users must explicitly support this.
  return queue->move();
}

unique_ptr<IOBuf> THeader::removeHttpClient(IOBufQueue* queue, size_t& needed) {
  protoId_ = T_BINARY_PROTOCOL;
  TMemoryBuffer memBuffer;
  THttpClientParser parser;
  parser.setDataBuffer(&memBuffer);
  const IOBuf* headBuf = queue->front();
  const IOBuf* nextBuf = headBuf;
  bool success = false;
  do {
    auto remainingDataLen = nextBuf->length();
    size_t offset = 0;
    auto ioBufData = nextBuf->data();
    do {
      void* parserBuf;
      size_t parserBufLen;
      parser.getReadBuffer(&parserBuf, &parserBufLen);
      size_t toCopyLen = std::min(parserBufLen, remainingDataLen);
      memcpy(parserBuf, ioBufData + offset, toCopyLen);
      success |= parser.readDataAvailable(toCopyLen);
      remainingDataLen -= toCopyLen;
      offset += toCopyLen;
    } while (remainingDataLen > 0);
    nextBuf = nextBuf->next();
  } while (nextBuf != headBuf);
  if (!success) {
    // We don't have full data yet and we don't know how many bytes we need,
    // but it is at least 1.
    needed = parser.getMinBytesRequired();
    return nullptr;
  }

  // Empty the queue
  queue->move();
  readHeaders_ = parser.moveReadHeaders();

  return memBuffer.cloneBufferAsIOBuf();
}

unique_ptr<IOBuf> THeader::removeFramed(uint32_t sz, IOBufQueue* queue) {
  // Trim off the frame size.
  queue->trimStart(4);
  return queue->split(sz);
}

folly::Optional<CLIENT_TYPE> THeader::analyzeFirst32bit(uint32_t w) {
  if ((w & TBinaryProtocol::VERSION_MASK) == TBinaryProtocol::VERSION_1) {
    return THRIFT_UNFRAMED_DEPRECATED;
  } else if (w == HTTP_SERVER_MAGIC ||
             w == HTTP_GET_CLIENT_MAGIC ||
             w == HTTP_HEAD_CLIENT_MAGIC) {
    return THRIFT_HTTP_SERVER_TYPE;
  } else if (w == HTTP_CLIENT_MAGIC) {
    return THRIFT_HTTP_CLIENT_TYPE;
  }
  return folly::none;
}

CLIENT_TYPE THeader::analyzeSecond32bit(uint32_t w) {
  if ((w & TBinaryProtocol::VERSION_MASK) == TBinaryProtocol::VERSION_1) {
    return THRIFT_FRAMED_DEPRECATED;
  }
  if (compactFramed(w)) {
    return THRIFT_FRAMED_COMPACT;
  }
  if ((w & HEADER_MASK) == HEADER_MAGIC) {
    if ((w & HEADER_FLAG_SASL) != 0) {
      return THRIFT_HEADER_SASL_CLIENT_TYPE;
    }
    return THRIFT_HEADER_CLIENT_TYPE;
  }
  return THRIFT_UNKNOWN_CLIENT_TYPE;
}

CLIENT_TYPE THeader::getClientType(uint32_t f, uint32_t s) {
  auto res = analyzeFirst32bit(f);
  if (res) {
    return *res;
  }
  return analyzeSecond32bit(s);
}

bool THeader::isFramed(CLIENT_TYPE type) {
  switch (type) {
  case THRIFT_FRAMED_DEPRECATED:
  case THRIFT_FRAMED_COMPACT:
    return true;
  default:
    return false;
  };

}

unique_ptr<IOBuf> THeader::removeNonHeader(IOBufQueue* queue,
                                           size_t& needed,
                                           CLIENT_TYPE type,
                                           uint32_t sz) {
  switch (type) {
  case THRIFT_FRAMED_DEPRECATED:
    protoId_ = T_BINARY_PROTOCOL;
    return removeFramed(sz, queue);
  case THRIFT_FRAMED_COMPACT:
    protoId_ = T_COMPACT_PROTOCOL;
    return removeFramed(sz, queue);
  case THRIFT_UNFRAMED_DEPRECATED:
    return removeUnframed(queue, needed);
  case THRIFT_HTTP_SERVER_TYPE:
    return removeHttpServer(queue);
  case THRIFT_HTTP_CLIENT_TYPE:
    return removeHttpClient(queue, needed);
  default:
    // Fallback to sniffing out the magic for Header
    return nullptr;
  };
}

unique_ptr<IOBuf> THeader::removeHeader(
  IOBufQueue* queue,
  size_t& needed,
  StringToStringMap& persistentReadHeaders) {
  Cursor c(queue->front());
  size_t remaining = queue->front()->computeChainDataLength();
  size_t frameSizeBytes = 4;
  needed = 0;

  if (remaining < 4) {
    needed = 4 - remaining;
    return nullptr;
  }

  // Use first word to check type.
  uint32_t sz32 = c.readBE<uint32_t>();
  remaining -= 4;

  if (forceClientType_) {
    // Make sure we have read the whole frame in.
    if (isFramed(clientType) && (sz32 > remaining)) {
      needed = sz32 - remaining;
      return nullptr;
    }
    unique_ptr<IOBuf> buf = THeader::removeNonHeader(queue,
                                                     needed,
                                                     clientType,
                                                     sz32);
    if(buf) {
      return buf;
    }
  }

  auto clientT = THeader::analyzeFirst32bit(sz32);
  if (clientT) {
    clientType = *clientT;
    return THeader::removeNonHeader(queue, needed, clientType, sz32);
  }

  size_t sz = sz32;
  if (sz32 > MAX_FRAME_SIZE) {
    if (sz32 == BIG_FRAME_MAGIC) {
      if (!allowBigFrames_) {
        throw TTransportException(
            TTransportException::INVALID_FRAME_SIZE,
            "Big frames not allowed");
      }
      if (8 > remaining) {
        needed = 8 - remaining;
        return nullptr;
      }
      sz = c.readBE<uint64_t>();
      remaining -= 8;
      frameSizeBytes += 8;
    } else {
      std::string err =
        folly::stringPrintf(
          "Header transport frame is too large: %u (hex 0x%08x",
          sz32, sz32);
      // does it look like ascii?
      if (  // all bytes 0x7f or less; all are > 0x1F
           ((sz32 & 0x7f7f7f7f) == sz32)
        && ((sz32 >> 24) & 0xff) >= 0x20
        && ((sz32 >> 16) & 0xff) >= 0x20
        && ((sz32 >>  8) & 0xff) >= 0x20
        && ((sz32 >>  0) & 0xff) >= 0x20) {
        char buffer[5];
        uint32_t *asUint32 = reinterpret_cast<uint32_t *> (buffer);
        *asUint32 = htonl(sz32);
        buffer[4] = 0;
        folly::stringAppendf(&err, ", ascii '%s'", buffer);
      }
      folly::stringAppendf(&err, ")");
      throw TTransportException(
        TTransportException::INVALID_FRAME_SIZE,
        err);
    }
  } else {
    sz = sz32;
  }

  // Make sure we have read the whole frame in.
  if (sz > remaining) {
    needed = sz - remaining;
    return nullptr;
  }

  // Could be header format or framed. Check next uint32
  uint32_t magic = c.readBE<uint32_t>();
  remaining -= 4;
  clientType = analyzeSecond32bit(magic);
  unique_ptr<IOBuf> buf = THeader::removeNonHeader(queue,
                                                   needed,
                                                   clientType,
                                                   sz);
  if(buf) {
    return buf;
  }

  if (clientType == THRIFT_UNKNOWN_CLIENT_TYPE) {
    throw TTransportException(
      TTransportException::BAD_ARGS,
      folly::stringPrintf(
        "Could not detect client transport type: magic 0x%08x",
        magic));
  }

  if (sz < 10) {
    throw TTransportException(
       TTransportException::INVALID_FRAME_SIZE,
       folly::stringPrintf("Header transport frame is too small: %zu", sz));
  }
  flags_ = magic & FLAGS_MASK;
  seqId = c.readBE<uint32_t>();

  // Trim off the frame size.
  queue->trimStart(frameSizeBytes);
  buf = readHeaderFormat(queue->split(sz), persistentReadHeaders);

  // auth client?
  auto auth_header = getHeaders().find(THRIFT_AUTH_HEADER);

  // Correct client type if needed
  if (auth_header != getHeaders().end()) {
    if (auth_header->second == "1") {
      clientType = THRIFT_HEADER_SASL_CLIENT_TYPE;
    } else {
      clientType = THRIFT_HEADER_CLIENT_TYPE;
    }
    readHeaders_.erase(auth_header);
  }
  return buf;
}

static string getString(RWPrivateCursor& c, size_t sz) {
  char strdata[sz];
  c.pull(strdata, sz);
  string str(strdata, sz);
  return str;
}

/**
 * Reads a string from ptr, taking care not to reach headerBoundary
 * Advances ptr on success
 *
 * @param RWPrivateCursor        cursor to read from
 */
static string readString(RWPrivateCursor& c) {
  return getString(c, readVarint<uint32_t>(c));
}

static void readInfoHeaders(RWPrivateCursor& c,
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

unique_ptr<IOBuf> THeader::readHeaderFormat(
  unique_ptr<IOBuf> buf,
  StringToStringMap& persistentReadHeaders) {
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
    readTrans_.push_back(transId);
    setTransform(transId);
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
        readInfoHeaders(c, persistentReadHeaders);
        break;
    }
  }

  // if persistent headers are not empty, merge together.
  if (!persistentReadHeaders.empty()) {
    readHeaders_.insert(persistentReadHeaders.begin(),
                        persistentReadHeaders.end());
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
  buf = untransform(std::move(buf), readTrans_);

  if (protoId_ == T_JSON_PROTOCOL && clientType != THRIFT_HTTP_SERVER_TYPE) {
    throw TApplicationException(TApplicationException::UNSUPPORTED_CLIENT_TYPE,
                                "Client is trying to send JSON without HTTP");
  }

  return buf;
}

unique_ptr<IOBuf> THeader::untransform(
  unique_ptr<IOBuf> buf, std::vector<uint16_t>& readTrans) {
  for (vector<uint16_t>::const_reverse_iterator it = readTrans.rbegin();
       it != readTrans.rend(); ++it) {
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
      {
        SCOPE_EXIT { err = inflateEnd(&stream); };
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
      }
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

  return buf;
}

unique_ptr<IOBuf> THeader::transform(unique_ptr<IOBuf> buf,
                                     std::vector<uint16_t>& writeTrans,
                                     size_t minCompressBytes) {
  size_t dataSize = buf->computeChainDataLength();

  for (vector<uint16_t>::iterator it = writeTrans.begin();
       it != writeTrans.end(); ) {
    const uint16_t transId = *it;

    if (transId == ZLIB_TRANSFORM) {
      if (dataSize < minCompressBytes) {
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
      if (dataSize < minCompressBytes) {
        it = writeTrans.erase(it);
        continue;
      }

      buf->coalesce(); // required for snappy compression

      // Check that we have enough space
      size_t maxCompressedLength = snappy::MaxCompressedLength(buf->length());
      unique_ptr<IOBuf> out(IOBuf::create(maxCompressedLength));

      size_t compressed_sz;
      snappy::RawCompress((char*)buf->data(), buf->length(),
                          (char*)out->writableData(), &compressed_sz);
      out->append(compressed_sz);
      buf = std::move(out);
    } else if (transId == QLZ_TRANSFORM) {
      if (dataSize < minCompressBytes) {
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

  return buf;
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
static void writeString(uint8_t* &ptr, const string& str) {
  DCHECK_LT(str.length(), std::numeric_limits<uint32_t>::max());
  uint32_t strLen = str.length();
  ptr += writeVarint32(strLen, ptr);
  memcpy(ptr, str.c_str(), strLen); // no need to write \0
  ptr += strLen;
}

/**
 * Writes headers to a byte buffer and clear the header map
 */
static void flushInfoHeaders(uint8_t* &pkt,
                             THeader::StringToStringMap &headers,
                             uint32_t infoIdType,
                             bool clearAfterFlush=true) {
  uint32_t headerCount = headers.size();
  if (headerCount > 0) {
    pkt += writeVarint32(infoIdType, pkt);
    // Write key-value headers count
    pkt += writeVarint32(headerCount, pkt);
    // Write info headers
    map<string, string>::const_iterator it;
    for (it = headers.begin(); it != headers.end(); ++it) {
      writeString(pkt, it->first);  // key
      writeString(pkt, it->second); // value
    }
    if (clearAfterFlush) {
      headers.clear();
    }
  }
}

void THeader::setHeader(const string& key, const string& value) {
  writeHeaders_[key] = value;
}

void THeader::setHeader(const char* key,
                        size_t keyLength,
                        const char* value,
                        size_t valueLength) {
  writeHeaders_.emplace(std::make_pair(
        std::string(key, keyLength),
        std::string(value, valueLength)));
}

void THeader::setHeaders(THeader::StringToStringMap&& headers) {
  writeHeaders_ = std::move(headers);
}

void THeader::setReadHeaders(THeader::StringToStringMap&& headers) {
  readHeaders_ = std::move(headers);
}

static size_t getInfoHeaderSize(const THeader::StringToStringMap &headers) {
  if (headers.empty()) {
    return 0;
  }
  size_t maxWriteHeadersSize = 5 + 5;  // type and count (2 varints32)
  for (const auto& it : headers) {
    // add sizes of key and value to maxWriteHeadersSize
    // 2 varints32 + the strings themselves
    maxWriteHeadersSize += 5 + 5 + (it.first).length() +
      (it.second).length();
  }
  return maxWriteHeadersSize;
}

size_t THeader::getMaxWriteHeadersSize(
  const StringToStringMap& persistentWriteHeaders) const {
  size_t maxWriteHeadersSize = 0;
  maxWriteHeadersSize += getInfoHeaderSize(persistentWriteHeaders);
  maxWriteHeadersSize += getInfoHeaderSize(writeHeaders_);
  if (extraWriteHeaders_) {
    maxWriteHeadersSize += getInfoHeaderSize(*extraWriteHeaders_);
  }
  return maxWriteHeadersSize;
}

void THeader::clearHeaders() {
  writeHeaders_.clear();
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

unique_ptr<IOBuf> THeader::addHeader(unique_ptr<IOBuf> buf,
                                     StringToStringMap& persistentWriteHeaders,
                                     bool transform) {
  // We may need to modify some transforms before send.  Make
  // a copy here
  std::vector<uint16_t> writeTrans = writeTrans_;

  if (clientType == THRIFT_HEADER_CLIENT_TYPE ||
      clientType == THRIFT_HEADER_SASL_CLIENT_TYPE) {
    if (transform) {
      buf = THeader::transform(std::move(buf), writeTrans, minCompressBytes_);
    }
  }
  size_t chainSize = buf->computeChainDataLength();

  if (protoId_ == T_JSON_PROTOCOL && clientType != THRIFT_HTTP_SERVER_TYPE) {
    throw TTransportException(TTransportException::BAD_ARGS,
                              "Trying to send JSON without HTTP");
  }

  if (chainSize > MAX_FRAME_SIZE &&
      clientType != THRIFT_HEADER_CLIENT_TYPE &&
      clientType != THRIFT_HEADER_SASL_CLIENT_TYPE) {
    throw TTransportException(
        TTransportException::INVALID_FRAME_SIZE,
        "Attempting to send non-header frame that is too large");
  }

  // Add in special flags
  // All flags must be added before any calls to getMaxWriteHeadersSize
  if (identity.length() > 0) {
    writeHeaders_[IDENTITY_HEADER] = identity;
    writeHeaders_[ID_VERSION_HEADER] = ID_VERSION;
  }

  if (clientType == THRIFT_HEADER_CLIENT_TYPE ||
      clientType == THRIFT_HEADER_SASL_CLIENT_TYPE) {
    // header size will need to be updated at the end because of varints.
    // Make it big enough here for max varint size, plus 4 for padding.
    int headerSize = (2 + getNumTransforms(writeTrans) * 2 /* transform data */)
      * 5 + 4;
    // add approximate size of info headers
    headerSize += getMaxWriteHeadersSize(persistentWriteHeaders);

    // Pkt size
    unique_ptr<IOBuf> header = IOBuf::create(22 + headerSize);
    // 8 bytes of headroom, we'll use them if we go over MAX_FRAME_SIZE
    header->advance(8);

    uint8_t* pkt = header->writableData();
    uint8_t* headerStart;
    uint8_t* headerSizePtr;
    uint8_t* pktStart = pkt;

    size_t szHbo;
    uint32_t szNbo;
    uint16_t headerSizeN;

    // Fixup szNbo later
    pkt += sizeof(szNbo);
    uint16_t magicN = htons(HEADER_MAGIC >> 16);
    memcpy(pkt, &magicN, sizeof(magicN));
    pkt += sizeof(magicN);
    uint16_t flagsN = (clientType == THRIFT_HEADER_SASL_CLIENT_TYPE) ?
                      htons(flags_ | HEADER_FLAG_SASL) :
                      htons(flags_);
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
    }

    // write info headers

    // write persistent kv-headers
    flushInfoHeaders(pkt, persistentWriteHeaders, infoIdType::PKEYVALUE);

    // write non-persistent kv-headers
    flushInfoHeaders(pkt, writeHeaders_, infoIdType::KEYVALUE);
    if (extraWriteHeaders_) {
      flushInfoHeaders(pkt, *extraWriteHeaders_, infoIdType::KEYVALUE, false);
    }

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
    assert(pkt - pktStart <= header->capacity());

    // Pkt size
    szHbo = headerSize + chainSize           // thrift header + payload
            + (headerStart - pktStart - 4);  // common header section
    headerSizeN = htons(headerSize / 4);
    memcpy(headerSizePtr, &headerSizeN, sizeof(headerSizeN));

    // Set framing size.
    if (szHbo > MAX_FRAME_SIZE) {
      if (!allowBigFrames_) {
        throw TTransportException(
            TTransportException::INVALID_FRAME_SIZE,
            "Big frames not allowed");
      }
      header->prepend(8);
      pktStart -= 8;
      szNbo = htonl(BIG_FRAME_MAGIC);
      memcpy(pktStart, &szNbo, sizeof(szNbo));
      uint64_t s = folly::Endian::big<uint64_t>(szHbo);
      memcpy(pktStart + 4, &s, sizeof(s));
    } else {
      szNbo = htonl(szHbo);
      memcpy(pktStart, &szNbo, sizeof(szNbo));
    }

    header->append(szHbo - chainSize + 4);
    header->prependChain(std::move(buf));
    buf = std::move(header);
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
             clientType == THRIFT_HTTP_SERVER_TYPE) {
    // We just return buf
    // TODO: IOBufize httpTransport.
  } else if (clientType == THRIFT_HTTP_CLIENT_TYPE) {
    CHECK(httpClientParser_.get() != nullptr);
    buf = httpClientParser_->constructHeader(std::move(buf),
                                             persistentWriteHeaders,
                                             writeHeaders_,
                                             extraWriteHeaders_);
    writeHeaders_.clear();
  } else {
    throw TTransportException(TTransportException::BAD_ARGS,
                              "Unknown client type");
  }

  return buf;
}

apache::thrift::concurrency::PRIORITY
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

std::chrono::milliseconds THeader::getClientTimeout() const {
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

void THeader::setHttpClientParser(shared_ptr<THttpClientParser> parser) {
  CHECK(clientType == THRIFT_HTTP_CLIENT_TYPE);
  httpClientParser_ = parser;
}

}}} // apache::thrift::transport
