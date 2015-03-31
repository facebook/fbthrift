/*
 * Copyright 2015 Facebook, Inc.
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
#include <thrift/lib/cpp/transport/TSocketOverHttpTunnel.h>
#include <folly/String.h>

using namespace std;

using folly::StringPiece;

const int64_t kMaxProxyResponseSize = 4096;

namespace apache { namespace thrift { namespace transport {

void TSocketOverHttpTunnel::open() {
  auto target = address_.describe();
  auto wbuf = folly::to<string>("CONNECT ", target,
                                " HTTP/1.1\r\nHost: ", target, "\r\n\r\n");

  TSocket::open();
  TSocket::write((uint8_t*)wbuf.c_str(), wbuf.length());

  // We'll be looking for those guys in the proxy's response.
  string suffix = "\r\n\r\n";

  string response;
  do {
    char rbuf[kMaxProxyResponseSize + 1];
    auto len = TSocket::read((uint8_t *)rbuf, kMaxProxyResponseSize);

    if (len == 0) {
      auto msg = "Timed out reading from proxy server";
      throw TTransportException(msg);
    }

    rbuf[len] = '\0';
    response += folly::to<string>(rbuf);

    if (response.size() > kMaxProxyResponseSize) {
      auto msg = "Proxy response exceeded size limit";
      throw TTransportException(msg);
    }

    if (StringPiece(response).contains(suffix)) {
      if (!StringPiece(response).endsWith(suffix)) {
        auto msg = "Extra bytes in proxy response";
        throw TTransportException(msg);
      }
      break;
    }
  } while (1);

  auto pos = response.find_first_of(' ');
  if ((pos == std::string::npos) || \
      (response.substr(0, 5) != "HTTP/") || \
      (response.substr(pos + 1, 4) != "200 ")) {
    auto msg = "Error response from proxy server: " + response;
    throw TTransportException(msg);
  }
}

}}} // namespace apache::thrift::transport
