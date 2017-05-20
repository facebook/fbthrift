/*
 * Copyright 2004-present Facebook, Inc.
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
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp2/security/SecurityLogger.h>

namespace apache { namespace thrift {

// The GSSAPI version of this is going to need a captive thread to
// avoid blocking the main thread on Kerberos operations, so we'll
// use one here, so the stub is more representative, even though it
// could be written asynchronously.

class StubSaslClient : public SaslClient {
public:
  explicit StubSaslClient(folly::EventBase*,
    const std::shared_ptr<SecurityLogger>& logger = nullptr,
    int forceMsSpentPerRTT = 0);
  void start(Callback* cb) override;
  void consumeFromServer(Callback* cb,
                         std::unique_ptr<folly::IOBuf>&& message) override;
  virtual std::unique_ptr<folly::IOBuf> wrap(std::unique_ptr<folly::IOBuf>&&);
  virtual std::unique_ptr<folly::IOBuf> unwrap(
    folly::IOBufQueue* q, size_t* remaining);
  void setClientIdentity(const std::string& /* identity */) override {}
  void setServiceIdentity(const std::string& /* identity */) override {}
  std::string getClientIdentity() const override;
  std::string getServerIdentity() const override;
  std::unique_ptr<folly::IOBuf> encrypt(
      std::unique_ptr<folly::IOBuf>&& buf) override {
    return std::move(buf);
  }
  std::unique_ptr<folly::IOBuf> decrypt(
      std::unique_ptr<folly::IOBuf>&& buf) override {
    return std::move(buf);
  }

  // This is for testing.
  void setForceFallback() { forceFallback_ = true; }
  void setForceTimeout() { forceTimeout_ = true; }
  void setForceMsSpentPerRTT(int t) { forceMsSpentPerRTT_ = t; }

  const std::string* getErrorString() const override { return nullptr; }

  void setErrorString(const std::string& /* str */) override {}

 private:
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;
  int phase_;
  bool forceFallback_;
  bool forceTimeout_;
  int forceMsSpentPerRTT_;  // milliseconds
};

}} // apache::thrift

#endif // THRIFT_SASLCLIENTSTUB_H_
