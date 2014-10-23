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

#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <folly/SocketAddress.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

using namespace std;
using folly::SocketAddress;

namespace apache { namespace thrift {

ScopedServerInterfaceThread::ScopedServerInterfaceThread(
    shared_ptr<ServerInterface> si,
    const string& host,
    uint16_t port) {
  auto ts = make_shared<ThriftServer>();
  ts->setAddress(host, port);
  ts->setInterface(move(si));
  ts->setNWorkerThreads(1);
  ts_ = move(ts);
  sst_.start(ts_);
}

ScopedServerInterfaceThread::ScopedServerInterfaceThread(
    shared_ptr<ThriftServer> ts) {
  ts_ = move(ts);
  sst_.start(ts_);
}

ThriftServer& ScopedServerInterfaceThread::getThriftServer() const {
  return *ts_;
}

const folly::SocketAddress& ScopedServerInterfaceThread::getAddress() const {
  return *sst_.getAddress();
}

uint16_t ScopedServerInterfaceThread::getPort() const {
  return getAddress().getPort();
}

}}
