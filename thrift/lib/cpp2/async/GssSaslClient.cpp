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

#include <thrift/lib/cpp2/async/GssSaslClient.h>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/Memory.h>
#include <folly/MoveWrapper.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/concurrency/FunctionRunner.h>
#include <thrift/lib/cpp2/protocol/MessageSerializer.h>
#include <thrift/lib/cpp2/gen-cpp2/Sasl_types.h>
#include <thrift/lib/cpp2/gen-cpp2/SaslAuthService.h>
#include <thrift/lib/cpp2/security/KerberosSASLHandshakeClient.h>
#include <thrift/lib/cpp2/security/KerberosSASLHandshakeUtils.h>
#include <thrift/lib/cpp2/security/KerberosSASLThreadManager.h>
#include <thrift/lib/cpp2/security/SecurityLogger.h>
#include <thrift/lib/cpp/concurrency/Exception.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>

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
using apache::thrift::transport::TTransportException;

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
    : SaslClient(evb, logger)
    , clientHandshake_(new KerberosSASLHandshakeClient(logger))
    , mutex_(new Mutex)
    , saslThreadManager_(nullptr)
    , seqId_(new int(0))
    , protocol_(0xFFFF)
    , inProgress_(std::make_shared<bool>(false)) {
}

void GssSaslClient::start(Callback *cb) {

  auto evb = evb_;
  auto clientHandshake = clientHandshake_;
  auto mutex = mutex_;
  auto logger = saslLogger_;
  auto proto = protocol_;
  auto seqId = seqId_;
  auto threadManager = saslThreadManager_;
  auto inProgress = inProgress_;

  logger->logStart("prepare_first_request");
  auto ew_tm = folly::try_and_catch<TooManyPendingTasksException>([&]() {
    if (!saslThreadManager_) {
      throw TApplicationException(
        "saslThreadManager is not set in GssSaslClient");
    }

    logger->logStart("thread_manager_overhead");
    *inProgress = true;
    threadManager->start(std::make_shared<FunctionRunner>([=] {
      logger->logEnd("thread_manager_overhead");
      MoveWrapper<unique_ptr<IOBuf>> iobuf;
      folly::exception_wrapper ex;

      {
        Guard guard(*mutex);
        if (!*evb) {
          return;
        }

        (*evb)->runInEventBaseThread([=] () mutable {
            if (!*evb) {
              return;
            }
            cb->saslStarted();
          });
      }

      ex = folly::try_and_catch<std::exception, TTransportException,
          TProtocolException, TApplicationException, TKerberosException>(
          [&]() {
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

        *iobuf = PargsPresultProtoSerialize(
          proto, argsp, "authFirstRequest", T_CALL, (*seqId)++);
      });

      Guard guard(*mutex);
      // Return if channel is unavailable. Ie. evb_ may not be good.
      if (!*evb) {
        return;
      }

      // Log the overhead around rescheduling the remainder of the
      // handshake at the back of the evb queue.
      logger->logStart("evb_overhead");
      (*evb)->runInEventBaseThread([=]() mutable {
        logger->logEnd("evb_overhead");
        if (!*evb) {
          return;
        }
        if (ex) {
          cb->saslError(std::move(ex));
          if (*inProgress) {
            threadManager->end();
            *inProgress = false;
          }
          return;
        } else {
          logger->logStart("first_rtt");
          cb->saslSendServer(std::move(*iobuf));
        }
      });
    }));
  });
  if (ew_tm) {
    logger->log("too_many_pending_tasks_in_start");
    cb->saslError(std::move(ew_tm));
    // no end() here.  If this happens, we never really started.
  }
}

void GssSaslClient::consumeFromServer(
  Callback *cb, std::unique_ptr<IOBuf>&& message) {
  std::shared_ptr<IOBuf> smessage(std::move(message));

  auto evb = evb_;
  auto clientHandshake = clientHandshake_;
  auto mutex = mutex_;
  auto logger = saslLogger_;
  auto proto = protocol_;
  auto seqId = seqId_;
  auto threadManager = saslThreadManager_;
  auto inProgress = inProgress_;

  auto ew_tm = folly::try_and_catch<TooManyPendingTasksException>([&]() {
    threadManager->get()->add(std::make_shared<FunctionRunner>([=] {
      std::string req_data;
      MoveWrapper<unique_ptr<IOBuf>> iobuf;
      folly::exception_wrapper ex;

      {
        Guard guard(*mutex);
        if (!*evb) {
          return;
        }

        (*evb)->runInEventBaseThread([=] () mutable {
            if (!*evb) {
              return;
            }
            cb->saslStarted();
          });
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
      ex = folly::try_and_catch<std::exception, TTransportException,
          TProtocolException, TApplicationException, TKerberosException>(
          [&]() {
        SaslReply reply;
        SaslAuthService_authFirstRequest_presult presult;
        presult.success = &reply;
        string methodName;
        try {
          methodName = PargsPresultProtoDeserialize(
              proto, presult, smessage.get(), T_REPLY).first;
        } catch (const TProtocolException& e) {
          if (proto == protocol::T_BINARY_PROTOCOL &&
              e.getType() == TProtocolException::BAD_VERSION) {
            // We used to use compact always in security messages, even when
            // the header said they should be binary. If we end up in this if,
            // we're talking to an old version remote end, so try compact too.
            methodName = PargsPresultProtoDeserialize(
                protocol::T_COMPACT_PROTOCOL,
                presult,
                smessage.get(),
                T_REPLY).first;
          } else {
            throw;
          }
        }
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
          *iobuf = PargsPresultProtoSerialize(proto, argsp, "authNextRequest",
                                                T_CALL, (*seqId)++);
        }
      });

      Guard guard(*mutex);
      // Return if channel is unavailable. Ie. evb_ may not be good.
      if (!*evb) {
        return;
      }

      auto phase = clientHandshake->getPhase();
      (*evb)->runInEventBaseThread([=]() mutable {
        if (!*evb) {
          return;
        }
        if (ex) {
          cb->saslError(std::move(ex));
          if (*inProgress) {
            threadManager->end();
            *inProgress = false;
          }
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
          if (*inProgress) {
            threadManager->end();
            *inProgress = false;
          }
        }
      });
    }));
  });
  if (ew_tm) {
    logger->log("too_many_pending_tasks_in_consume");
    cb->saslError(std::move(ew_tm));
    if (*inProgress) {
      threadManager->end();
      *inProgress = false;
    }
  }
}

std::unique_ptr<IOBuf> GssSaslClient::encrypt(
    std::unique_ptr<IOBuf>&& buf) {
  return clientHandshake_->wrapMessage(std::move(buf));
}

std::unique_ptr<IOBuf> GssSaslClient::decrypt(
    std::unique_ptr<IOBuf>&& buf) {
  return clientHandshake_->unwrapMessage(std::move(buf));
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

void GssSaslClient::detachEventBase() {
  apache::thrift::concurrency::Guard guard(*mutex_);
  if (*inProgress_) {
    saslThreadManager_->end();
    *inProgress_ = false;
  }
  *evb_ = nullptr;
}

void GssSaslClient::attachEventBase(apache::thrift::async::TEventBase* evb) {
  apache::thrift::concurrency::Guard guard(*mutex_);
  *evb_ = evb;
}

}}
