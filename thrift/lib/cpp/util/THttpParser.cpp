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

#include <thrift/lib/cpp/util/THttpParser.h>

#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp/transport/TTransportException.h>
#include <cstdlib>
#include <sstream>
#include <cassert>
#include <boost/algorithm/string.hpp>

namespace apache { namespace thrift { namespace util {

using namespace std;
using namespace folly;
using apache::thrift::transport::TTransportException;
using apache::thrift::transport::TMemoryBuffer;

const int THttpParser::CRLF_LEN = 2;
const char* const CRLF = "\r\n";

THttpParser::THttpParser()
  : httpPos_(0)
  , httpBufLen_(0)
  , httpBufSize_(1024)
  , state_(HTTP_PARSE_START)
  , maxSize_(0x7fffffff)
  , dataBuf_(nullptr) {
  httpBuf_ = (char*)std::malloc(httpBufSize_ + 1);
  if (httpBuf_ == nullptr) {
    throw std::bad_alloc();
  }
  httpBuf_[httpBufLen_] = '\0';
}

THttpParser::~THttpParser() {
  if (httpBuf_ != nullptr) {
    std::free(httpBuf_);
    httpBuf_ = nullptr;
  }
}

void THttpParser::getReadBuffer(void** bufReturn,
                                size_t* lenReturn) {
  assert(httpBufLen_ <= httpBufSize_);
  uint32_t avail = httpBufSize_ - httpBufLen_;
  if (avail <= (httpBufSize_ / 4)) {
    httpBufSize_ *= 2;
    httpBuf_ = (char*)std::realloc(httpBuf_, httpBufSize_ + 1);
    if (httpBuf_ == nullptr) {
      throw std::bad_alloc();
    }
  }
  *bufReturn = httpBuf_ + httpBufLen_;
  *lenReturn = httpBufSize_ - httpBufLen_;
}

bool THttpParser::readDataAvailable(size_t len) {
  assert(httpBufLen_ + len <= httpBufSize_);
  httpBufLen_ += len;
  httpBuf_[httpBufLen_] = '\0';
  while (true) {
    THttpParser::HttpParseResult result;
    switch (state_) {
      case HTTP_PARSE_START:
        result = parseStart();
        break;

      case HTTP_PARSE_HEADER:
        result = parseHeader();
        break;

      case HTTP_PARSE_CHUNK:
        result = parseChunk();
        break;

      case HTTP_PARSE_CONTENT:
        result = parseContent();
        break;

      case HTTP_PARSE_CHUNKFOOTER:
        result = parseChunkFooter();
        break;

      case HTTP_PARSE_TRAILING:
        result = parseTrailing();
        break;

      default:
        throw TTransportException("Unknown state");
    }

    if (result == HTTP_PARSE_RESULT_CONTINUE) {
      if (state_ == HTTP_PARSE_START) {
        // parse the whole message
        return true;
      }
    } else {
      // need read more data
      assert(result == HTTP_PARSE_RESULT_BLOCK);
      assert(httpBufLen_ >= httpPos_);
      checkMessageSize(httpBufLen_ - httpPos_, false);
      return false;
    }
  }
}

char* THttpParser::readLine() {
  char* eol = nullptr;
  eol = strstr(httpBuf_ + httpPos_, CRLF);
  if (eol != nullptr) {
    *eol = '\0';
    char* line = httpBuf_ + httpPos_;
    uint32_t oldHttpPos = httpPos_;
    httpPos_ = (eol - httpBuf_) + CRLF_LEN;
    assert(httpPos_ >= oldHttpPos);
    checkMessageSize(httpPos_ - oldHttpPos, true);
    assert(httpPos_ <= httpBufLen_);
    return line;
  } else {
    shift();
    return nullptr;
  }
}

void THttpParser::checkMessageSize(uint32_t more, bool added) {
  uint32_t messageSize = partialMessageSize_ + more;
  if (messageSize > maxSize_) {
    T_ERROR("THttpParser: partial message size of %d rejected", messageSize);
    throw TTransportException(TTransportException::CORRUPTED_DATA,
                              "rejected overly large  http message");
  }
  if (added) {
    partialMessageSize_ = messageSize;
  }
}

void THttpParser::shift() {
  assert(httpBufLen_ >= httpPos_);
  if (httpBufLen_ > httpPos_) {
    // Shift down remaining data and read more
    uint32_t length = httpBufLen_ - httpPos_;
    memmove(httpBuf_, httpBuf_ + httpPos_, length);
    httpBufLen_ = length;
  } else {
    httpBufLen_ = 0;
  }
  httpPos_ = 0;
  httpBuf_[httpBufLen_] = '\0';
}

THttpParser::HttpParseResult THttpParser::parseStart() {
  contentLength_ = 0;
  chunked_ = false;

  statusLine_ = true;
  finished_ = false;

  partialMessageSize_ = 0;

  state_ = THttpParser::HTTP_PARSE_HEADER;
  return THttpParser::HTTP_PARSE_RESULT_CONTINUE;
}

THttpParser::HttpParseResult THttpParser::parseHeader() {
  // Loop until headers are finished
  while (true) {
    char* line = readLine();
    // no line is found, need wait for more data.
    if (line == nullptr) {
      return HTTP_PARSE_RESULT_BLOCK;
    }

    if (strlen(line) == 0) {
      if (finished_) {
        // go to the next state
        if (chunked_) {
          state_ = THttpParser::HTTP_PARSE_CHUNK;
        } else {
          state_ = THttpParser::HTTP_PARSE_CONTENT;
        }
        return THttpParser::HTTP_PARSE_RESULT_CONTINUE;
      } else {
        // Must have been an HTTP 100, keep going for another status line
        statusLine_ = true;
      }
    } else {
      if (statusLine_) {
        statusLine_ = false;
        finished_ = parseStatusLine(line);
      } else {
        parseHeaderLine(line);
      }
    }
  }
}

THttpParser::HttpParseResult THttpParser::parseChunk() {
  char* line = readLine();
  if (line == nullptr) {
    return THttpParser::HTTP_PARSE_RESULT_BLOCK;
  }

  char* semi = strchr(line, ';');
  if (semi != nullptr) {
    *semi = '\0';
  }
  uint32_t size = 0;
  sscanf(line, "%x", &size);
  if (size == 0) {
    state_ = THttpParser::HTTP_PARSE_CHUNKFOOTER;
  } else {
    contentLength_ = size;
    state_ = THttpParser::HTTP_PARSE_CONTENT;
  }
  return THttpParser::HTTP_PARSE_RESULT_CONTINUE;
}

THttpParser::HttpParseResult THttpParser::parseChunkFooter() {
  // End of data, read footer lines until a blank one appears
  while (true) {
    char* line = readLine();
    if (line == nullptr) {
      return THttpParser::HTTP_PARSE_RESULT_BLOCK;
    }
    if (strlen(line) == 0) {
      state_ = THttpParser::HTTP_PARSE_START;
      break;
    }
  }
  return THttpParser::HTTP_PARSE_RESULT_CONTINUE;
}

THttpParser::HttpParseResult THttpParser::parseContent() {
  assert(httpPos_ <= httpBufLen_);
  size_t avail = httpBufLen_ - httpPos_;
  if (avail > 0 && avail >= contentLength_) {
    //copy all of them to the data buf
    assert(dataBuf_ != nullptr);
    dataBuf_->write((uint8_t*)(httpBuf_ + httpPos_), contentLength_);
    httpPos_ += contentLength_;
    checkMessageSize(contentLength_, true);
    contentLength_ = 0;
    shift();
    if (chunked_) {
      state_ = THttpParser::HTTP_PARSE_TRAILING;
    } else {
      state_ = THttpParser::HTTP_PARSE_START;
    }

    return THttpParser::HTTP_PARSE_RESULT_CONTINUE;
  } else {
    return THttpParser::HTTP_PARSE_RESULT_BLOCK;
  }
}

THttpParser::HttpParseResult THttpParser::parseTrailing() {
  assert(chunked_);
  char* line = readLine();
  if (line == nullptr) {
    return THttpParser::HTTP_PARSE_RESULT_BLOCK;
  } else {
    state_ = THttpParser::HTTP_PARSE_CHUNK;
  }
  return THttpParser::HTTP_PARSE_RESULT_CONTINUE;
}

void THttpClientParser::parseHeaderLine(const char* header) {
  const char* colon = strchr(header, ':');
  if (colon == nullptr) {
    return;
  }
  const char* value = colon + 1;

  if (boost::istarts_with(header, "Transfer-Encoding")) {
    if (boost::iends_with(value, "chunked")) {
      chunked_ = true;
    }
  } else if (boost::istarts_with(header, "Content-Length")) {
    chunked_ = false;
    contentLength_ = atoi(value);
  } else if (boost::istarts_with(header, "Connection")) {
    if (boost::iends_with(header, "close")) {
      connectionClosedByServer_ = true;
    }
  }
}

bool THttpClientParser::parseStatusLine(const char* status) {
  const char* http = status;

  // Skip over the "HTTP/<version>" string.
  // TODO: we should probably check that the version is 1.0 or 1.1
  const char* code = strchr(http, ' ');
  if (code == nullptr) {
    throw TTransportException(string("Bad Status: ") + status);
  }

  // RFC 2616 requires exactly 1 space between the HTTP version and the status
  // code.  Skip over it.
  ++code;

  // Check the status code.  It must be followed by a space.
  if (strncmp(code, "200 ", 4) == 0) {
    // HTTP 200 = OK, we got the response
    return true;
  } else if (strncmp(code, "100 ", 4) == 0) {
    // HTTP 100 = continue, just keep reading
    return false;
  } else {
    throw TTransportException(string("Bad Status: ") + status);
  }
}

void THttpClientParser::resetConnectClosedByServer() {
  connectionClosedByServer_ = false;
}

bool THttpClientParser::isConnectClosedByServer() {
  return connectionClosedByServer_;
}

unique_ptr<IOBuf> THttpClientParser::constructHeader(unique_ptr<IOBuf> buf) {
  std::map<std::string, std::string> empty;
  return constructHeader(std::move(buf), empty, empty);
}

unique_ptr<IOBuf> THttpClientParser::constructHeader(
   unique_ptr<IOBuf> buf,
   const std::map<std::string, std::string>& persistentWriteHeaders,
   const std::map<std::string, std::string>& writeHeaders) {
  IOBufQueue queue;
  queue.append("POST ");
  queue.append(path_);
  queue.append(" HTTP/1.1");
  queue.append(CRLF);
  queue.append("Host: ");
  queue.append(host_);
  queue.append(CRLF);
  queue.append("Content-Type: application/x-thrift");
  queue.append(CRLF);
  queue.append("Accept: application/x-thrift");
  queue.append(CRLF);
  queue.append("User-Agent: ");
  queue.append(userAgent_);
  queue.append(CRLF);
  queue.append("Content-Length: ");
  string contentLen = std::to_string(buf->computeChainDataLength());
  queue.append(contentLen);
  queue.append(CRLF);
  // write persistent headers
  for (const auto& persistentWriteHeader : persistentWriteHeaders) {
    queue.append(persistentWriteHeader.first);

    queue.append(": ");
    queue.append(persistentWriteHeader.second);
    queue.append(CRLF);
  }
  // write non-persistent headers
  for (const auto& writeHeader : writeHeaders) {
    queue.append(writeHeader.first);
    queue.append(": ");
    queue.append(writeHeader.second);
    queue.append(CRLF);
  }
  queue.append(CRLF);

  auto res = queue.move();
  res->appendChain(std::move(buf));
  return std::move(res);
}

}}}
