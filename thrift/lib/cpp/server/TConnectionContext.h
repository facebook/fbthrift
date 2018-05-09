/*
 * Copyright 2014-present Facebook, Inc.
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

#ifndef THRIFT_TCONNECTIONCONTEXT_H_
#define THRIFT_TCONNECTIONCONTEXT_H_ 1

#include <stddef.h>

#include <memory>
#include <folly/io/async/EventBaseManager.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <folly/SocketAddress.h>

namespace apache { namespace thrift {

namespace protocol {
class TProtocol;
}

namespace server {

class TConnectionContext {
 public:
  TConnectionContext()
    : userData_(nullptr)
    , destructor_(nullptr) {}

  virtual ~TConnectionContext() {
    cleanupUserData();
  }

  virtual const folly::SocketAddress* getPeerAddress() const {
    return &peerAddress_;
  }

  void reset() {
    peerAddress_.reset();
    localAddress_.reset();
    cleanupUserData();
  }

  virtual std::shared_ptr<protocol::TProtocol> getInputProtocol() const {
    return nullptr;
  }

  virtual std::shared_ptr<protocol::TProtocol> getOutputProtocol() const {
    return nullptr;
  }

  // Expose the THeader to read headers or other flags
  virtual transport::THeader* getHeader() const {
    if (getOutputProtocol()) {
      return dynamic_cast<apache::thrift::transport::THeader*>(
          getOutputProtocol()->getTransport().get());
    }
    return nullptr;
  }

  virtual bool setHeader(const std::string& key, const std::string& value) {
    auto header = getHeader();
    if (header) {
      header->setHeader(key, value);
      return true;
    } else {
      return false;
    }
  }

  virtual std::map<std::string, std::string> getHeaders() const {
    auto header = getHeader();
    if (header) {
      return header->getHeaders();
    } else {
      return std::map<std::string, std::string>();
    }
  }

  virtual const std::map<std::string, std::string>* getHeadersPtr() {
    auto header = getHeader();
    if (header) {
      return &header->getHeaders();
    } else {
      return nullptr;
    }
  }

  virtual folly::EventBaseManager* getEventBaseManager() {
    return nullptr;
  }

  /**
   * Get the user data field.
   */
  virtual void* getUserData() const {
    return userData_;
  }

  /**
   * Set the user data field.
   *
   * @param data         The new value for the user data field.
   * @param destructor   A function pointer to invoke when the connection
   *                     context is destroyed.  It will be invoked with the
   *                     contents of the user data field.
   *
   * @return Returns the old user data value.
   */
  virtual void* setUserData(void* data, void (*destructor)(void*) = nullptr) {
    void* oldData = userData_;
    userData_  = data;
    destructor_ = destructor;
    return oldData;
  }

 protected:
  folly::SocketAddress peerAddress_;
  folly::SocketAddress localAddress_;

  void cleanupUserData() {
    if (destructor_) {
      destructor_(userData_);
      destructor_ = nullptr;
    }
    userData_ = nullptr;
  }

 private:
  void* userData_;
  void (*destructor_)(void*);
};

}}} // apache::thrift::server

#endif // THRIFT_TCONNECTIONCONTEXT_H_
