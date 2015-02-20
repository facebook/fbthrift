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

#ifndef THRIFT_TRANSPORT_THTTPPARSER_H_
#define THRIFT_TRANSPORT_THTTPPARSER_H_ 1

#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>

namespace apache { namespace thrift { namespace util {

class THttpParser {
 protected:
  enum HttpParseState {
    HTTP_PARSE_START = 0,
    HTTP_PARSE_HEADER,
    HTTP_PARSE_CHUNK,
    HTTP_PARSE_CONTENT,
    HTTP_PARSE_CHUNKFOOTER,
    HTTP_PARSE_TRAILING
  };

  enum HttpParseResult {
    HTTP_PARSE_RESULT_CONTINUE,
    HTTP_PARSE_RESULT_BLOCK
  };

 public:
  THttpParser();
  virtual ~THttpParser();

  void getReadBuffer(void** bufReturn, size_t* lenReturn);
  bool readDataAvailable(size_t len);
  void setDataBuffer(apache::thrift::transport::TMemoryBuffer* buffer) {
    dataBuf_ = buffer;
  }
  void unsetDataBuffer() {
    dataBuf_ = nullptr;
  }
  void setMaxSize(uint32_t size) {
    maxSize_ = size;
  }
  uint32_t getMaxSize() {
    return maxSize_;
  }
  bool hasReadAheadData() {
    return (state_ == HTTP_PARSE_START) && (httpBufLen_ > httpPos_);
  }
  bool hasPartialMessage() {
    return partialMessageSize_ > 0;
  }
  const std::map<std::string, std::string>& getReadHeaders() {
    return readHeaders_;
  }
  virtual std::unique_ptr<folly::IOBuf> constructHeader(
    std::unique_ptr<folly::IOBuf> buf) = 0;
  virtual std::unique_ptr<folly::IOBuf> constructHeader(
    std::unique_ptr<folly::IOBuf> buf,
    const std::map<std::string, std::string>& persistentWriteHeaders,
    const std::map<std::string, std::string>& writeHeaders) = 0;

 protected:
  HttpParseResult parseStart();
  HttpParseResult parseHeader();
  HttpParseResult parseContent();
  HttpParseResult parseChunk();
  HttpParseResult parseChunkFooter();
  HttpParseResult parseTrailing();

  virtual bool parseStatusLine(const char* status) = 0;
  virtual void parseHeaderLine(const char* header) = 0;

  void shift();
  char* readLine();
  void checkMessageSize(uint32_t more, bool added);

  char* httpBuf_;
  uint32_t httpPos_;
  uint32_t httpBufLen_;
  uint32_t httpBufSize_;

  HttpParseState state_;

  // for read headers
  bool statusLine_;
  bool finished_;
  bool chunked_;
  std::map<std::string, std::string> readHeaders_;

  size_t contentLength_;

  // max http message size
  uint32_t maxSize_;
  uint32_t partialMessageSize_;

  apache::thrift::transport::TMemoryBuffer* dataBuf_;

  static const int CRLF_LEN;
};

class THttpClientParser : public THttpParser {
 public:
  THttpClientParser() {}
  THttpClientParser(std::string host, std::string path) {
    host_ = host;
    path_ = path;
    userAgent_ = "C++/THttpClient";
  }

  void setHost(const std::string& host) { host_ = host; }
  void setPath(const std::string& path) { path_ = path; }
  void resetConnectClosedByServer();
  bool isConnectClosedByServer();
  void setUserAgent(std::string userAgent) {
    userAgent_ = userAgent;
  }
  virtual std::unique_ptr<folly::IOBuf> constructHeader(
    std::unique_ptr<folly::IOBuf> buf) override;
  virtual std::unique_ptr<folly::IOBuf> constructHeader(
    std::unique_ptr<folly::IOBuf> buf,
    const std::map<std::string, std::string>& persistentWriteHeaders,
    const std::map<std::string, std::string>& writeHeaders) override;

 protected:
  virtual void parseHeaderLine(const char* header) override;
  virtual bool parseStatusLine(const char* status) override;

 private:
  bool connectionClosedByServer_;
  std::string host_;
  std::string path_;
  std::string userAgent_;
};


}}} // apache::thrift::util

#endif // #ifndef THRIFT_TRANSPORT_THTTPPARSER_H_
