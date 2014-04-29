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

#include "thrift/lib/cpp2/async/GssSaslClient.h"

#include "folly/io/Cursor.h"
#include "folly/io/IOBuf.h"
#include "folly/io/IOBufQueue.h"
#include "folly/Memory.h"
#include "folly/MoveWrapper.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/concurrency/FunctionRunner.h"
#include "thrift/lib/cpp2/protocol/MessageSerializer.h"
#include "thrift/lib/cpp2/gen-cpp2/Sasl_types.h"
#include "thrift/lib/cpp2/gen-cpp2/SaslAuthService.h"
#include "thrift/lib/cpp2/security/KerberosSASLHandshakeClient.h"
#include "thrift/lib/cpp2/security/KerberosSASLHandshakeUtils.h"
#include "thrift/lib/cpp2/security/KerberosSASLThreadManager.h"
#include "thrift/lib/cpp2/security/SecurityLogger.h"
#include "thrift/lib/cpp/concurrency/Exception.h"

#include <memory>

using folly::IOBuf;
using folly::IOBufQueue;
using folly::MoveWrapper;
using apache::thrift::concurrency::Guard;
using apache::thrift::concurrency::Mutex;
using apache::thrift::concurrency::FunctionRunner;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using apache::thrift::concurrency::TooManyPendingTasksException;

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

GssSaslClient::GssSaslClient(apache::thrift::async::TEventBase* evb,
      const std::shared_ptr<SecurityLogger>& logger)
    : SaslClient(logger)
    , evb_(evb)
    , clientHandshake_(new KerberosSASLHandshakeClient(logger))
    , mutex_(new Mutex)
    , saslThreadManager_(nullptr) {
}

void GssSaslClient::start(Callback *cb) {

  auto channelCallbackUnavailable = channelCallbackUnavailable_;
  auto clientHandshake = clientHandshake_;
  auto mutex = mutex_;
  auto logger = saslLogger_;

  logger->logStart("prepare_first_request");
  try {
    if (!saslThreadManager_) {
      throw TApplicationException(
        "saslThreadManager is not set in GssSaslClient");
    }

    logger->logStart("thread_manager_overhead");
    saslThreadManager_->get()->add(std::make_shared<FunctionRunner>([=] {
      logger->logEnd("thread_manager_overhead");
      MoveWrapper<unique_ptr<IOBuf>> iobuf;
      std::exception_ptr ex;

      // This is an optimization. If the channel is unavailable, we have no
      // need to attempt to communicate to the KDC. Note that this is just
      // checking a boolean, so thread safety is not an issue here.
      if (*channelCallbackUnavailable) {
        return;
      }

      try {
        clientHandshake->startClientHandshake();
        auto token = clientHandshake->getTokenToSend();

        SaslStart start;
        start.mechanism = MECH;
        if (token != nullptr) {
          start.request.response = *token;
          start.request.__isset.response = true;
        }
        start.__isset.request = true;
        SaslAuthService_authFirstRequest_pargs argsp;
        argsp.saslStart = &start;

        *iobuf = PargsPresultCompactSerialize(
          argsp, "authFirstRequest", T_CALL);
      } catch (const std::exception& e) {
        ex = std::current_exception();
      }

      Guard guard(*mutex);
      // Return if channel is unavailable. Ie. evb_ may not be good.
      if (*channelCallbackUnavailable) {
        return;
      }

      evb_->runInEventBaseThread([=]() mutable {
        if (*channelCallbackUnavailable) {
          return;
        }
        if (ex) {
          cb->saslError(std::exception_ptr(ex));
          return;
        } else {
          logger->logStart("first_rtt");
          cb->saslSendServer(std::move(*iobuf));
        }
      });
    }));
  } catch (const TooManyPendingTasksException& e) {
    logger->log("too_many_pending_tasks_in_start");
    cb->saslError(std::current_exception());
  }
}

void GssSaslClient::consumeFromServer(
  Callback *cb, std::unique_ptr<IOBuf>&& message) {
  std::shared_ptr<IOBuf> smessage(std::move(message));

  auto channelCallbackUnavailable = channelCallbackUnavailable_;
  auto clientHandshake = clientHandshake_;
  auto mutex = mutex_;
  auto logger = saslLogger_;

  try {
    saslThreadManager_->get()->add(std::make_shared<FunctionRunner>([=] {
      std::string req_data;
      MoveWrapper<unique_ptr<IOBuf>> iobuf;
      std::exception_ptr ex;

      // This is an optimization. If the channel is unavailable, we have no
      // need to attempt to communicate to the KDC. Note that this is just
      // checking a boolean, so thread safety is not an issue here.
      if (*channelCallbackUnavailable) {
        return;
      }

      // Get the input string or outcome status
      std::string input = "";
      bool finished = false;
      // SaslAuthService_authFirstRequest_presult should be structurally
      // identical to SaslAuthService_authNextRequest_presult
      static_assert(sizeof(SaslAuthService_authFirstRequest_presult) ==
                    sizeof(SaslAuthService_authNextRequest_presult),
                    "Types should be structurally identical");
      static_assert(std::is_same<
          decltype(SaslAuthService_authFirstRequest_presult::success),
          decltype(SaslAuthService_authNextRequest_presult::success)>::value,
        "Types should be structurally identical");
      try {
        SaslReply reply;
        SaslAuthService_authFirstRequest_presult presult;
        presult.success = &reply;
        string methodName =
          PargsPresultCompactDeserialize(presult, smessage.get(), T_REPLY);
        if (methodName != "authFirstRequest" &&
            methodName != "authNextRequest") {
          throw TApplicationException("Bad return method name: " + methodName);
        }
        if (reply.__isset.challenge) {
          input = reply.challenge;
        }
        if (reply.__isset.outcome) {
          finished = reply.outcome.success;
        }

        clientHandshake->handleResponse(input);
        auto token = clientHandshake->getTokenToSend();
        if (clientHandshake->getPhase() == COMPLETE) {
          assert(token == nullptr);
          if (finished != true) {
            throw TKerberosException("Outcome of false returned from server");
          }
        }
        if (token != nullptr) {
          SaslRequest req;
          req.response = *token;
          req.__isset.response = true;
          SaslAuthService_authNextRequest_pargs argsp;
          argsp.saslRequest = &req;
          *iobuf = PargsPresultCompactSerialize(argsp, "authNextRequest",
                                                T_CALL);
        }
      } catch (const std::exception& e) {
        ex = std::current_exception();
      }

      Guard guard(*mutex);
      // Return if channel is unavailable. Ie. evb_ may not be good.
      if (*channelCallbackUnavailable) {
        return;
      }

      auto phase = clientHandshake->getPhase();
      evb_->runInEventBaseThread([=]() mutable {
        if (*channelCallbackUnavailable) {
          return;
        }
        if (ex) {
          cb->saslError(std::exception_ptr(ex));
          return;
        }
        if (*iobuf && !(*iobuf)->empty()) {
          if (phase == SELECT_SECURITY_LAYER) {
            logger->logStart("third_rtt");
          } else {
            logger->logStart("second_rtt");
          }
          cb->saslSendServer(std::move(*iobuf));
        }
        if (clientHandshake_->isContextEstablished()) {
          cb->saslComplete();
        }
      });
    }));
  } catch (const TooManyPendingTasksException& e) {
    logger->log("too_many_pending_tasks_in_consume");
    cb->saslError(std::current_exception());
  }
}

std::unique_ptr<IOBuf> GssSaslClient::wrap(std::unique_ptr<IOBuf>&& buf) {
  buf->coalesce();

  std::unique_ptr<IOBuf> wrapped = clientHandshake_->wrapMessage(
    std::move(buf));
  uint32_t wraplen = wrapped->length();

  std::unique_ptr<IOBuf> framing = IOBuf::create(sizeof(wraplen));
  framing->append(sizeof(wraplen));
  framing->appendChain(std::move(wrapped));

  folly::io::RWPrivateCursor c(framing.get());
  c.writeBE<uint32_t>(wraplen);
  return framing;
}

std::unique_ptr<IOBuf> GssSaslClient::unwrap(
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
  std::unique_ptr<IOBuf> output = clientHandshake_->unwrapMessage(
    std::move(input));
  *remaining = 0;
  return output;
}

std::string GssSaslClient::getClientIdentity() const {
  if (clientHandshake_->isContextEstablished()) {
    return clientHandshake_->getEstablishedClientPrincipal();
  } else {
    return "";
  }
}

std::string GssSaslClient::getServerIdentity() const {
  if (clientHandshake_->isContextEstablished()) {
    return clientHandshake_->getEstablishedServicePrincipal();
  } else {
    return "";
  }
}

}}
