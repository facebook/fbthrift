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

#pragma once

#include <thrift/lib/cpp/async/TAsyncSocketFactory.h>

#include <thrift/lib/cpp/transport/TSSLSocket.h>

namespace apache { namespace thrift { namespace async {

/**
 * Factory class for producing TAsyncSSLSocket instances.
 */
class TAsyncSSLSocketFactory :
 public TAsyncSocketFactory {
 public:
  explicit TAsyncSSLSocketFactory(TEventBase* eventBase);
  virtual ~TAsyncSSLSocketFactory();

  /**
   * Set the SSLContext to use when constructing sockets.
   */
  void setSSLContext(transport::SSLContextPtr& context);

  /**
   * Set whether or not we're constructing client or server sockets.
   *
   * By default, client sockets are created.
   */
  void setServerMode(bool serverMode);

  // TAsyncSocketFactory
  virtual TAsyncSocket::UniquePtr make() const override;
  virtual TAsyncSocket::UniquePtr make(int fd) const override;

 protected:
  TEventBase* eventBase_;
  transport::SSLContextPtr context_;
  bool serverMode_;
};

}}}
