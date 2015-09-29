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

#include <thrift/lib/cpp/async/TAsyncSSLSocketFactory.h>

#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>

using folly::SSLContext;
using folly::SSLContextPtr;

namespace apache { namespace thrift { namespace async {

TAsyncSSLSocketFactory::TAsyncSSLSocketFactory(folly::EventBase* eventBase) :
  TAsyncSocketFactory(eventBase),
  eventBase_(eventBase),
  context_(),
  serverMode_(false) {
}

TAsyncSSLSocketFactory::~TAsyncSSLSocketFactory() {
}

void TAsyncSSLSocketFactory::setSSLContext(SSLContextPtr& ctx) {
  context_ = ctx;
}

void TAsyncSSLSocketFactory::setServerMode(bool serverMode) {
  serverMode_ = serverMode;
}

TAsyncSocket::UniquePtr TAsyncSSLSocketFactory::make() const {
  if (serverMode_) {
    throw std::logic_error("cannot create unconnected server socket");
  }

  return TAsyncSocket::UniquePtr(new TAsyncSSLSocket(context_, eventBase_));
}

TAsyncSocket::UniquePtr TAsyncSSLSocketFactory::make(int fd) const {
  return TAsyncSocket::UniquePtr(new TAsyncSSLSocket(context_, eventBase_, fd, serverMode_));
}

}}}
