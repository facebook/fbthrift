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

#include "thrift/lib/cpp2/async/SaslServer.h"
#include "thrift/lib/cpp/concurrency/ThreadManager.h"
#include "thrift/lib/cpp2/security/KerberosSASLHandshakeServer.h"
#include "thrift/lib/cpp/async/TEventBase.h"


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
  virtual void consumeFromClient(
    Callback *cb, std::unique_ptr<folly::IOBuf>&& message);
  virtual std::unique_ptr<folly::IOBuf> wrap(std::unique_ptr<folly::IOBuf>&&);
  virtual std::unique_ptr<folly::IOBuf> unwrap(
      folly::IOBufQueue* q, size_t* remaining);
  void setServiceIdentity(const std::string& identity) {
    serverHandshake_->setRequiredServicePrincipal(identity);
  }
  virtual std::string getClientIdentity() const;
  virtual std::string getServerIdentity() const;
  virtual void markChannelCallbackUnavailable() {
    *channelCallbackUnavailable_ = true;
  }

private:
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;
  apache::thrift::async::TEventBase* evb_;
  std::shared_ptr<KerberosSASLHandshakeServer> serverHandshake_;
};

}} // apache::thrift

#endif // THRIFT_GSSSASLSERVER_H_
