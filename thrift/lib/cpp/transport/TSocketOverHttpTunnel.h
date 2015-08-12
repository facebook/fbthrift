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
#ifndef _THRIFT_TRANSPORT_TSOCKETOVERHTTPTUNNEL_H_
#define _THRIFT_TRANSPORT_TSOCKETOVERHTTPTUNNEL_H_ 1

#include <thrift/lib/cpp/transport/TSocket.h>
#include <folly/SocketAddress.h>

namespace apache { namespace thrift { namespace transport {

class TSocketOverHttpTunnel : public TSocket {

private:
  folly::SocketAddress address_;
  folly::SocketAddress proxyAddress_;

public:
  TSocketOverHttpTunnel(folly::SocketAddress& address,
                        folly::SocketAddress& proxyAddress) :
          TSocket(&proxyAddress), address_(address),
          proxyAddress_(proxyAddress) { }

  void open();
};

}}} // namespace apache::thrift::transport

#endif // #ifndef _THRIFT_TRANSPORT_TSOCKETOVERHTTPTUNNEL_H_
