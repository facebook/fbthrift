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

#ifndef THRIFT_GSSSASLSERVER_H_
#define THRIFT_GSSSASLSERVER_H_ 1

#include <thrift/lib/cpp2/async/SaslServer.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>
#include <thrift/lib/cpp2/security/KerberosSASLHandshakeServer.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/concurrency/Mutex.h>

namespace apache { namespace thrift {

/**
 * Server responsible for the GSS SASL handshake.
 */
class GssSaslServer : public SaslServer {
public:
  explicit GssSaslServer(
    apache::thrift::async::TEventBase*,
    std::shared_ptr<apache::thrift::concurrency::ThreadManager> thread_manager
  );
  void consumeFromClient(Callback* cb,
                         std::unique_ptr<folly::IOBuf>&& message) override;
  std::unique_ptr<folly::IOBuf> encrypt(
      std::unique_ptr<folly::IOBuf>&&) override;
  std::unique_ptr<folly::IOBuf> decrypt(
      std::unique_ptr<folly::IOBuf>&&) override;
  void setServiceIdentity(const std::string& identity) override {
    serverHandshake_->setRequiredServicePrincipal(identity);
  }
  std::string getClientIdentity() const override;
  std::string getServerIdentity() const override;
  void detachEventBase() override {
    apache::thrift::concurrency::Guard guard(*mutex_);
    *evb_ = nullptr;
  }
  void attachEventBase(apache::thrift::async::TEventBase* evb) override {
    apache::thrift::concurrency::Guard guard(*mutex_);
    *evb_ = evb;
  }
  void setProtocolId(uint16_t protocol) override {
    protocol_ = protocol;
  }

private:
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;
  std::shared_ptr<KerberosSASLHandshakeServer> serverHandshake_;
  std::shared_ptr<apache::thrift::concurrency::Mutex> mutex_;
  uint16_t protocol_;
};

}} // apache::thrift

#endif // THRIFT_GSSSASLSERVER_H_
