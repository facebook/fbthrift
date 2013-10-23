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

#ifndef THRIFT_SASLENDPOINT_H_
#define THRIFT_SASLENDPOINT_H_ 1

#include <memory>

namespace folly {
class IOBuf;
class IOBufQueue;
}

namespace apache { namespace thrift {

class SaslEndpoint {
public:
  SaslEndpoint() : channelCallbackUnavailable_(new bool(false)) {}
  virtual ~SaslEndpoint() {}

  // The following methods must not be called until after
  // saslComplete() has been invoked on the derived class's callback.

  // Given plaintext, returns data to be transmitted.  This data
  // includes a prepended length and protected data.
  virtual std::unique_ptr<folly::IOBuf> wrap(
      std::unique_ptr<folly::IOBuf>&&) = 0;

  // Given some protected data, this determines if a complete token is
  // present.  If it is, then it is removed from the input buffer, and
  // the plaintext is returned, and remaining is set to zero.
  // Otherwise, an empty buffer is returned and remaining is set to
  // the number of bytes required to construct a valid token.
  virtual std::unique_ptr<folly::IOBuf> unwrap(
      folly::IOBufQueue* q, size_t* remaining) = 0;

  // Returns the identity of the client.  TODO mhorowitz: the return
  // type may change.
  virtual std::string getClientIdentity() const = 0;

  // Returns the identity of the server.  TODO mhorowitz: the return
  // type may change.
  virtual std::string getServerIdentity() const = 0;

  virtual void markChannelCallbackUnavailable() {
    // Mark the object as destroyed, stop thread execution if it hasn't started
    *channelCallbackUnavailable_ = true;
  }

protected:
  std::shared_ptr<bool> channelCallbackUnavailable_;

};

}} // apache::thrift

#endif // THRIFT_SASLENDPOINT_H_
