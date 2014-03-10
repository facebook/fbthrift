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

#ifndef THRIFT_ASYNC_TASYNCSOCKETFACTORY_H_
#define THRIFT_ASYNC_TASYNCSOCKETFACTORY_H_ 1

#include "thrift/lib/cpp/async/TAsyncSocket.h"

namespace apache { namespace thrift { namespace async {

class TEventBase;

/**
 * Factory class for producing TAsyncSocket instances.
 *
 * This implementation produces instances of TAsyncSocket, but one may want to
 * subclass this to produce MockTAsyncSocket, TAsyncSSLSocket, etc.
 */
class TAsyncSocketFactory {
 public:
  explicit TAsyncSocketFactory(TEventBase* eventBase);
  virtual ~TAsyncSocketFactory();

  /**
   * Construct a new, unconnected socket.
   */
  virtual TAsyncSocket::UniquePtr make() const;

  /**
   * Construct a new socket based on the given connected file descriptor.
   */
  virtual TAsyncSocket::UniquePtr make(int fd) const;

 protected:
  TEventBase* eventBase_;
};

}}}

#endif
