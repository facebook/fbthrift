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

#ifndef THRIFT_SASLENDPOINT_H_
#define THRIFT_SASLENDPOINT_H_ 1

#include <memory>

#include <thrift/lib/cpp/async/TEventBase.h>

namespace folly {
class IOBuf;
class IOBufQueue;
}

namespace apache { namespace thrift {

class SaslEndpoint {
public:
  explicit SaslEndpoint(apache::thrift::async::TEventBase* evb = nullptr) :
    evb_(std::make_shared<apache::thrift::async::TEventBase*>(evb)) {}
  virtual ~SaslEndpoint() {}

  // The following methods must not be called until after
  // saslComplete() has been invoked on the derived class's callback.

  // Given plaintext, returns data to be transmitted.  This data
  // includes a prepended length and protected data.
  std::unique_ptr<folly::IOBuf> wrap(std::unique_ptr<folly::IOBuf>&&);

  // Given some protected data, this determines if a complete token is
  // present.  If it is, then it is removed from the input buffer, and
  // the plaintext is returned, and remaining is set to zero.
  // Otherwise, an empty buffer is returned and remaining is set to
  // the number of bytes required to construct a valid token.
  std::unique_ptr<folly::IOBuf> unwrap(
    folly::IOBufQueue* q, size_t* remaining);

  // Returns the identity of the client.  TODO mhorowitz: the return
  // type may change.
  virtual std::string getClientIdentity() const = 0;

  // Returns the identity of the server.  TODO mhorowitz: the return
  // type may change.
  virtual std::string getServerIdentity() const = 0;

  virtual void detachEventBase() {
    // Mark the object as destroyed, stop thread execution if it hasn't started
    *evb_ = nullptr;
  }

  virtual void attachEventBase(apache::thrift::async::TEventBase* evb) {
    *evb_ = evb;
  }

  // Sets the protocol (e.g. binary/compact)
  // to use for serialization/deserialization of auth messages
  virtual void setProtocolId(uint16_t /*protocol*/) {}

protected:
  virtual std::unique_ptr<folly::IOBuf> encrypt(
    std::unique_ptr<folly::IOBuf>&&) = 0;

  virtual std::unique_ptr<folly::IOBuf> decrypt(
    std::unique_ptr<folly::IOBuf>&&) = 0;

  std::shared_ptr<apache::thrift::async::TEventBase*> evb_;
};

}} // apache::thrift

#endif // THRIFT_SASLENDPOINT_H_
