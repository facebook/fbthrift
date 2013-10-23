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

#include "thrift/lib/cpp2/async/GssSaslServer.h"

#include "folly/io/Cursor.h"
#include "folly/io/IOBuf.h"
#include "folly/io/IOBufQueue.h"
#include "folly/Conv.h"
#include "folly/MoveWrapper.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/concurrency/FunctionRunner.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp2/protocol/MessageSerializer.h"
#include "thrift/lib/cpp2/gen-cpp2/Sasl_types.h"
#include "thrift/lib/cpp2/gen-cpp2/SaslAuthService.h"
#include "thrift/lib/cpp2/security/KerberosSASLHandshakeServer.h"
#include "thrift/lib/cpp2/security/KerberosSASLHandshakeUtils.h"

#include <memory>

using folly::IOBuf;
using folly::IOBufQueue;
using folly::MoveWrapper;
using apache::thrift::concurrency::FunctionRunner;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using namespace std;
using apache::thrift::sasl::SaslStart;
using apache::thrift::sasl::SaslRequest;
using apache::thrift::sasl::SaslReply;
using apache::thrift::sasl::SaslAuthService_authFirstRequest_pargs;
using apache::thrift::sasl::SaslAuthService_authFirstRequest_presult;
using apache::thrift::sasl::SaslAuthService_authNextRequest_pargs;
using apache::thrift::sasl::SaslAuthService_authNextRequest_presult;

namespace apache { namespace thrift {

static const char MECH[] = "krb5";

GssSaslServer::GssSaslServer(apache::thrift::async::TEventBase* evb)
    : threadManager_(ThreadManager::newSimpleThreadManager(
        1, /* count */
        0, /* pendingTaskCountMax */
        false, /* enableTaskStats */
        1<<6 /* maxQueueLen */))
    , evb_(evb)
    , serverHandshake_(new KerberosSASLHandshakeServer) {
  threadManager_->threadFactory(std::make_shared<PosixThreadFactory>());
}

void GssSaslServer::consumeFromClient(
  Callback *cb, std::unique_ptr<IOBuf>&& message) {
  std::shared_ptr<IOBuf> smessage(std::move(message));

  auto channelCallbackUnavailable = channelCallbackUnavailable_;
  auto serverHandshake = serverHandshake_;
  auto threadManager = threadManager_;
  threadManager_->start();
  threadManager_->add(std::make_shared<FunctionRunner>([=] {
    std::string reply_data;
    std::exception_ptr ex;

    // Get the input string. We deserialize differently depending on the
    // current state.
    std::string input;
    bool isFirstRequest;
    if (serverHandshake->getPhase() == INIT) {
      isFirstRequest = true;

      SaslStart start;
      SaslAuthService_authFirstRequest_pargs pargs;
      pargs.saslStart = &start;
      try {
        string methodName =
          PargsPresultCompactDeserialize(pargs, smessage.get(), T_CALL);

        if (methodName != "authFirstRequest") {
          throw TKerberosException("Bad Thrift first call: " + methodName);
        }
        if (start.mechanism != MECH) {
          throw TKerberosException("Unknown mechanism: " + start.mechanism);
        }

        input = start.request.response;
      } catch (...) {
        ex = std::current_exception();
      }
    } else {
      isFirstRequest = false;

      SaslRequest req;
      SaslAuthService_authNextRequest_pargs pargs;
      pargs.saslRequest = &req;
      try {
        string methodName =
          PargsPresultCompactDeserialize(pargs, smessage.get(), T_CALL);

        if (methodName != "authNextRequest") {
          throw TKerberosException("Bad Thrift next call: " + methodName);
        }

        input = req.response;
      } catch (...) {
        ex = std::current_exception();
      }
    }

    MoveWrapper<unique_ptr<IOBuf>> outbuf;
    if (!ex) {
      // If there were no exceptions, send a reply. If we're finished, then
      // send a success indicator reply, otherwise send a generic token.
      try {
        serverHandshake->handleResponse(input);
        auto token = serverHandshake->getTokenToSend();
        if (token != nullptr) {
          SaslReply reply;
          if (serverHandshake->getPhase() != COMPLETE) {
            reply.challenge = *token;
            reply.__isset.challenge = true;
          } else {
            reply.outcome.success = true;
            reply.__isset.outcome = true;
          }
          if (isFirstRequest) {
            SaslAuthService_authFirstRequest_presult resultp;
            resultp.success = &reply;
            resultp.__isset.success = true;
            *outbuf = PargsPresultCompactSerialize(resultp, "authFirstRequest",
                                                   T_REPLY);
          } else {
            SaslAuthService_authNextRequest_presult resultp;
            resultp.success = &reply;
            resultp.__isset.success = true;
            *outbuf = PargsPresultCompactSerialize(resultp, "authNextRequest",
                                                   T_REPLY);
          }
        }
      } catch (...) {
        ex = std::current_exception();
      }
    }

    evb_->runInEventBaseThread([=]() mutable {
        // If the callback has already been destroyed, the request must
        // have terminated, so we don't need to do anything.
        if (*channelCallbackUnavailable) {
          return;
        }
        if (ex) {
          threadManager->stop();
          cb->saslError(std::exception_ptr(ex));
          return;
        }
        if (*outbuf && !(*outbuf)->empty()) {
          cb->saslSendClient(std::move(*outbuf));
        }
        if (serverHandshake->isContextEstablished()) {
          threadManager->stop();
          cb->saslComplete();
        }
      });
  }));
}

std::unique_ptr<IOBuf> GssSaslServer::wrap(std::unique_ptr<IOBuf>&& buf) {
  buf->coalesce();

  std::unique_ptr<IOBuf> wrapped = serverHandshake_->wrapMessage(
    std::move(buf));
  uint32_t wraplen = wrapped->length();

  std::unique_ptr<IOBuf> framing = IOBuf::create(sizeof(wraplen));
  framing->append(sizeof(wraplen));
  framing->appendChain(std::move(wrapped));

  folly::io::RWPrivateCursor c(framing.get());
  c.writeBE<uint32_t>(wraplen);
  return framing;
}

std::unique_ptr<IOBuf> GssSaslServer::unwrap(
  IOBufQueue* q,
  size_t* remaining) {

  folly::io::Cursor c(q->front());
  size_t chainSize = q->front()->computeChainDataLength();
  uint32_t wraplen = 0;

  if (chainSize < sizeof(wraplen)) {
    *remaining = sizeof(wraplen) - chainSize;
    return nullptr;
  }

  wraplen = c.readBE<uint32_t>();

  if (chainSize < sizeof(wraplen) + wraplen) {
    *remaining = sizeof(wraplen) + wraplen - chainSize;
    return nullptr;
  }

  // unwrap the data
  q->trimStart(sizeof(wraplen));
  std::unique_ptr<IOBuf> input = q->split(wraplen);
  input->coalesce();
  std::unique_ptr<IOBuf> output = serverHandshake_->unwrapMessage(
    std::move(input));
  *remaining = 0;
  return output;
}

std::string GssSaslServer::getClientIdentity() const {
  if (serverHandshake_->isContextEstablished()) {
    return serverHandshake_->getEstablishedClientPrincipal();
  } else {
    return "";
  }
}

std::string GssSaslServer::getServerIdentity() const {
  if (serverHandshake_->isContextEstablished()) {
    return serverHandshake_->getEstablishedServicePrincipal();
  } else {
    return "";
  }
}

}}
