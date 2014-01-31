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

#ifndef THRIFT_TRANSPORT_THTTPCLIENT_H_
#define THRIFT_TRANSPORT_THTTPCLIENT_H_ 1

#include "thrift/lib/cpp/transport/THttpTransport.h"

namespace apache { namespace thrift { namespace transport {

class THttpClient : public THttpTransport {

  typedef std::map<std::string, std::string> StringToStringMap;

 public:
  /*
   * Create for a given host and port.  The version that doesn't take
   * a transport constructs its own TSocket as a transport.
   *
   * The path must be non-empty and start with "/".
   */
  THttpClient(const std::shared_ptr<TTransport>& transport,
              const std::string& host,
              const std::string& path);
  THttpClient(const std::string& host, int port, const std::string& path);

  virtual ~THttpClient();

  void setUserAgent(const std::string&);

  void setHeader(const std::string& name, const std::string& value);

  StringToStringMap& getRequestHeaders() {
    return requestHeaders_;
  }

  const StringToStringMap& getResponseHeaders() const {
    return responseHeaders_;
  }

  virtual void flush();

  virtual void close() {
    connectionClosedByServer_ = false;
    THttpTransport::close();
  }

  virtual void init() {
    /*
     * HTTP requires that the `abs_path' component of a POST message start
     * with '/' (see rfc2616 and rfc2396).
     */
    assert(!path_.empty() && path_[0] == '/');

    THttpTransport::init();
  }

  const std::string& getHost() const {
    return host_;
  }

  void setHost(const std::string& host) {
    host_ = host;
  }

  const std::string& getPath() const {
    return path_;
  }

  const static std::string kAcceptHeader;
  const static std::string kConnectionHeader;
  const static std::string kContentLengthHeader;
  const static std::string kContentTypeHeader;
  const static std::string kHostHeader;
  const static std::string kTransferEncodingHeader;
  const static std::string kUserAgentHeader;

 protected:

  std::string host_;
  const std::string path_;
  bool connectionClosedByServer_;
  uint16_t statusCode_;
  StringToStringMap requestHeaders_;
  StringToStringMap responseHeaders_;

  virtual void beginParsingHeaders();
  virtual void parseHeader(char* header);
  virtual bool parseStatusLine(char* status);
  virtual void endParsingHeaders();
};

}}} // apache::thrift::transport

#endif // #ifndef THRIFT_TRANSPORT_THTTPCLIENT_H_
