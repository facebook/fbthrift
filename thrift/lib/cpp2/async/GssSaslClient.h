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

#ifndef THRIFT_GSSSASLCLIENT_H_
#define THRIFT_GSSSASLCLIENT_H_ 1

#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp2/async/SaslClient.h"
#include "thrift/lib/cpp2/security/KerberosSASLHandshakeClient.h"
#include "thrift/lib/cpp2/security/KerberosSASLThreadManager.h"
#include "thrift/lib/cpp/concurrency/Mutex.h"
#include "thrift/lib/cpp/util/kerberos/Krb5CredentialsCacheManager.h"
#include "folly/Memory.h"

namespace apache { namespace thrift {

/**
 * Client responsible for the GSS SASL handshake.
 */
class GssSaslClient : public SaslClient {
public:
  explicit GssSaslClient(apache::thrift::async::TEventBase*);
  virtual void start(Callback *cb);
  virtual void consumeFromServer(
    Callback *cb, std::unique_ptr<folly::IOBuf>&& message);
  virtual std::unique_ptr<folly::IOBuf> wrap(std::unique_ptr<folly::IOBuf>&&);
  virtual std::unique_ptr<folly::IOBuf> unwrap(
    folly::IOBufQueue* q, size_t* remaining);
  void setClientIdentity(const std::string& identity) {
    clientHandshake_->setRequiredClientPrincipal(identity);
  }
  void setServiceIdentity(const std::string& identity) {
    clientHandshake_->setRequiredServicePrincipal(identity);
  }
  virtual std::string getClientIdentity() const;
  virtual std::string getServerIdentity() const;
  virtual void markChannelCallbackUnavailable() {
    apache::thrift::concurrency::Guard guard(*mutex_);
    *channelCallbackUnavailable_ = true;
  }

  virtual const std::string* getErrorString() const {
    return errorString_.get();
  }

  // Set error string, prepend phase at which this error happened.
  virtual void setErrorString(const std::string& str) {
    std::string err =
      std::string("Phase: ") +
      std::to_string((int)clientHandshake_->getPhase()) +
      " " + str;
    errorString_ = folly::make_unique<std::string>(err);
  }

  virtual void setSaslThreadManager(
      const std::shared_ptr<SaslThreadManager>& thread_manager) {
    saslThreadManager_ = thread_manager;
    clientHandshake_->setSaslThreadManager(thread_manager);
  }

  virtual void setCredentialsCacheManager(
      const std::shared_ptr<krb5::Krb5CredentialsCacheManager>& cc_manager) {
    clientHandshake_->setCredentialsCacheManager(cc_manager);
  }

private:
  apache::thrift::async::TEventBase* evb_;
  std::shared_ptr<KerberosSASLHandshakeClient> clientHandshake_;
  std::unique_ptr<std::string> errorString_;
  std::shared_ptr<apache::thrift::concurrency::Mutex> mutex_;
  std::shared_ptr<SaslThreadManager> saslThreadManager_;
};

}} // apache::thrift

#endif // THRIFT_GSSSASLCLIENT_H_
