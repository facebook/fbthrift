/*
 * Copyright 2015-present Facebook, Inc.
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

#include <thrift/lib/cpp2/async/HeaderChannel.h>

namespace apache {
namespace thrift {

/**
 * Interface for Thrift Client channels
 */
class ClientChannel : public RequestChannel, public HeaderChannel {
 public:
  ClientChannel() {}
  ~ClientChannel() override {}

  typedef
    std::unique_ptr<ClientChannel,
                    folly::DelayedDestruction::Destructor>
    Ptr;

  virtual apache::thrift::async::TAsyncTransport* getTransport() = 0;

  virtual bool good() = 0;

  virtual void attachEventBase(folly::EventBase*) = 0;
  virtual void detachEventBase() = 0;
  virtual bool isDetachable() = 0;

  virtual bool isSecurityActive() = 0;
  virtual uint32_t getTimeout() = 0;
  virtual void setTimeout(uint32_t ms) = 0;

  virtual void closeNow() = 0;
  virtual CLIENT_TYPE getClientType() = 0;
};
}
} // apache::thrift
