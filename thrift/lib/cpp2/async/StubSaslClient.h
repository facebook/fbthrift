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

#ifndef THRIFT_SASLCLIENTSTUB_H_
#define THRIFT_SASLCLIENTSTUB_H_ 1

#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp2/async/SaslClient.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp2/security/SecurityLogger.h>

namespace apache { namespace thrift {

// The GSSAPI version of this is going to need a captive thread to
// avoid blocking the main thread on Kerberos operations, so we'll
// use one here, so the stub is more representative, even though it
// could be written asynchronously.

class StubSaslClient : public SaslClient {
public:
  explicit StubSaslClient(apache::thrift::async::TEventBase*,
    const std::shared_ptr<SecurityLogger>& logger = nullptr);
  virtual void start(Callback *cb);
  virtual void consumeFromServer(
    Callback *cb, std::unique_ptr<folly::IOBuf>&& message);
  virtual std::unique_ptr<folly::IOBuf> wrap(std::unique_ptr<folly::IOBuf>&&);
  virtual std::unique_ptr<folly::IOBuf> unwrap(
    folly::IOBufQueue* q, size_t* remaining);
  virtual void setClientIdentity(const std::string& identity) {}
  virtual void setServiceIdentity(const std::string& identity) {}
  virtual std::string getClientIdentity() const;
  virtual std::string getServerIdentity() const;
  virtual std::unique_ptr<folly::IOBuf> encrypt(
    std::unique_ptr<folly::IOBuf>&& buf) {
    return std::move(buf);
  }
  virtual std::unique_ptr<folly::IOBuf> decrypt(
    std::unique_ptr<folly::IOBuf>&& buf) {
    return std::move(buf);
  }

  // This is for testing.
  void setForceFallback() { forceFallback_ = true; }
  void setForceTimeout() { forceTimeout_ = true; }

  const std::string* getErrorString() const {
    return nullptr;
  }

  void setErrorString(const std::string& str) {}

private:
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;
  int phase_;
  bool forceFallback_;
  bool forceTimeout_;
};

}} // apache::thrift

#endif // THRIFT_SASLCLIENTSTUB_H_
