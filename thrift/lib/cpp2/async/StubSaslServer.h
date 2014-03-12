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

#ifndef THRIFT_SASLSERVERSTUB_H_
#define THRIFT_SASLSERVERSTUB_H_ 1

#include "thrift/lib/cpp/concurrency/ThreadManager.h"
#include "thrift/lib/cpp2/async/SaslServer.h"
#include "thrift/lib/cpp/async/TEventBase.h"

namespace apache { namespace thrift {

// The GSSAPI version of this is going to need a captive thread to
// avoid blocking the main thread on Kerberos operations, so we'll
// use one here, so the stub is more representative, even though it
// could be written asynchronously.

class StubSaslServer : public SaslServer {
public:
  explicit StubSaslServer(apache::thrift::async::TEventBase*);
  virtual void consumeFromClient(
    Callback *cb, std::unique_ptr<folly::IOBuf>&& message);
  virtual std::unique_ptr<folly::IOBuf> wrap(std::unique_ptr<folly::IOBuf>&&);
  virtual std::unique_ptr<folly::IOBuf> unwrap(
      folly::IOBufQueue* q, size_t* remaining);
  virtual void setServiceIdentity(const std::string& identity) {}
  virtual std::string getClientIdentity() const;
  virtual std::string getServerIdentity() const;

  // This is for testing.
  void setForceFallback() { forceFallback_ = true; }

private:
  std::shared_ptr<apache::thrift::concurrency::ThreadManager> threadManager_;
  apache::thrift::async::TEventBase* evb_;
  int phase_;
  bool forceFallback_;
};

}} // apache::thrift

#endif // THRIFT_SASLSERVERSTUB_H_
