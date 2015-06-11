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

#include <thrift/lib/cpp2/async/GssSaslServer.h>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/Conv.h>
#include <folly/MoveWrapper.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/concurrency/FunctionRunner.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp2/protocol/MessageSerializer.h>
#include <thrift/lib/cpp2/gen-cpp2/Sasl_types.h>
#include <thrift/lib/cpp2/gen-cpp2/SaslAuthService.tcc>
#include <thrift/lib/cpp2/security/KerberosSASLHandshakeServer.h>
#include <thrift/lib/cpp2/security/KerberosSASLHandshakeUtils.h>

#include <memory>

using folly::IOBuf;
using folly::IOBufQueue;
using folly::MoveWrapper;
using apache::thrift::concurrency::FunctionRunner;
using apache::thrift::concurrency::Guard;
using apache::thrift::concurrency::Mutex;
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

static const char KRB5_SASL[] = "krb5";
static const char KRB5_GSS[] = "gss";
static const char KRB5_GSS_NO_MUTUAL[] = "gssnm";

GssSaslServer::GssSaslServer(
    apache::thrift::async::TEventBase* evb,
    std::shared_ptr<apache::thrift::concurrency::ThreadManager> thread_manager)
    : SaslServer(evb)
    , threadManager_(thread_manager)
    , serverHandshake_(new KerberosSASLHandshakeServer)
    , mutex_(new Mutex)
    , protocol_(0xFFFF) {
}

void GssSaslServer::consumeFromClient(
  Callback *cb, std::unique_ptr<IOBuf>&& message) {
  std::shared_ptr<IOBuf> smessage(std::move(message));

  auto evb = evb_;
  auto serverHandshake = serverHandshake_;
  auto mutex = mutex_;
  auto proto = protocol_;

  auto exw = folly::try_and_catch<std::exception>([&]() {
    threadManager_->add(std::make_shared<FunctionRunner>([=] {
      std::string reply_data;
      folly::exception_wrapper ex;

      uint16_t replyWithProto = proto;

      // Get the input string. We deserialize differently depending on the
      // current state.
      std::string input;

      int requestSeqId;

      bool isFirstRequest;
      string selectedMech;
      if (serverHandshake->getPhase() == INIT) {
        isFirstRequest = true;

        SaslStart start;
        SaslAuthService_authFirstRequest_pargs pargs;
        pargs.get<0>().value = &start;
        ex = folly::try_and_catch<std::exception>([&]() {
          string methodName;
          try {
            std::tie(methodName, requestSeqId) = PargsPresultProtoDeserialize(
                proto,
                pargs,
                smessage.get(),
                T_CALL);
          } catch (const TProtocolException& e) {
            if (proto == protocol::T_BINARY_PROTOCOL &&
                e.getType() == TProtocolException::BAD_VERSION) {
              // We used to use compact always in security messages, even when
              // the header said they should be binary. If we end up in this if,
              // we're talking to an old version remote end, so try compact too.
              std::tie(methodName, requestSeqId) = PargsPresultProtoDeserialize(
                  protocol::T_COMPACT_PROTOCOL,
                  pargs,
                  smessage.get(),
                  T_CALL);
              replyWithProto = protocol::T_COMPACT_PROTOCOL;
            } else {
              throw;
            }
          }
          if (methodName != "authFirstRequest") {
            throw TKerberosException("Bad Thrift first call: " + methodName);
          }
          vector<string> mechs;
          if (start.__isset.mechanisms) {
            mechs = start.mechanisms;
          }
          mechs.push_back(start.mechanism);

          for (const auto& mech : mechs) {
            if (mech == KRB5_SASL) {
              selectedMech = KRB5_SASL;
              serverHandshake->setSecurityMech(SecurityMech::KRB5_SASL);
              break;
            } else if (mech == KRB5_GSS) {
              selectedMech = KRB5_GSS;
              serverHandshake->setSecurityMech(SecurityMech::KRB5_GSS);
              break;
            } else if (mech == KRB5_GSS_NO_MUTUAL) {
              selectedMech = KRB5_GSS_NO_MUTUAL;
              serverHandshake->setSecurityMech(
                SecurityMech::KRB5_GSS_NO_MUTUAL);
              break;
            }
          }

          // Fall back to SASL if no known mechanisms were passed.
          if (selectedMech.empty()) {
            selectedMech = KRB5_SASL;
            serverHandshake->setSecurityMech(SecurityMech::KRB5_SASL);
          }

          input = start.request.response;
        });
      } else {
        isFirstRequest = false;

        SaslRequest req;
        SaslAuthService_authNextRequest_pargs pargs;
        pargs.get<0>().value = &req;
        ex = folly::try_and_catch<std::exception>([&]() {
          string methodName;
          try {
            std::tie(methodName, requestSeqId) = PargsPresultProtoDeserialize(
                    proto,
                    pargs,
                    smessage.get(),
                    T_CALL);
          } catch (const TProtocolException& e) {
            if (proto == protocol::T_BINARY_PROTOCOL &&
                e.getType() == TProtocolException::BAD_VERSION) {
              // We used to use compact always in security messages, even when
              // the header said they should be binary. If we end up in this if,
              // we're talking to an old version remote end, so try compact too.
              std::tie(methodName, requestSeqId) = PargsPresultProtoDeserialize(
                  protocol::T_COMPACT_PROTOCOL,
                  pargs,
                  smessage.get(),
                  T_CALL);
              replyWithProto = protocol::T_COMPACT_PROTOCOL;
            } else {
              throw;
            }
          }
          catch (std::exception& e) {
            LOG(ERROR) << e.what();
            throw;
          }

          if (methodName != "authNextRequest") {
            throw TKerberosException("Bad Thrift next call: " + methodName);
          }

          input = req.response;
        });
      }

      MoveWrapper<unique_ptr<IOBuf>> outbuf;
      if (!ex) {
        // If there were no exceptions, send a reply. If we're finished, then
        // send a success indicator reply, otherwise send a generic token.
        ex = folly::try_and_catch<std::exception>([&]() {
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
              // We still need to send the token even when completed.
              if (serverHandshake->getSecurityMech() ==
                  SecurityMech::KRB5_GSS) {
                reply.challenge = *token;
                reply.__isset.challenge = true;
              }
            }
            if (!selectedMech.empty()) {
              reply.mechanism = selectedMech;
              reply.__isset.mechanism = true;
            }
            if (isFirstRequest) {
              SaslAuthService_authFirstRequest_presult resultp;
              resultp.get<0>().value = &reply;
              resultp.setIsSet(0);
              *outbuf = PargsPresultProtoSerialize(replyWithProto,
                                                   resultp,
                                                   "authFirstRequest",
                                                   T_REPLY,
                                                   requestSeqId);
            } else {
              SaslAuthService_authNextRequest_presult resultp;
              resultp.get<0>().value = &reply;
              resultp.setIsSet(0);
              *outbuf = PargsPresultProtoSerialize(replyWithProto,
                                                   resultp,
                                                   "authNextRequest",
                                                   T_REPLY,
                                                   requestSeqId);
            }
          }
        });
      }

      Guard guard(*mutex);
      // Return if channel is unavailable. Ie. evb_ may not be good.
      if (!*evb) {
        return;
      }

      (*evb)->runInEventBaseThread([=]() mutable {
          // If the callback has already been destroyed, the request must
          // have terminated, so we don't need to do anything.
          if (!*evb) {
            return;
          }
          if (ex) {
            cb->saslError(std::move(ex));
            return;
          }
          if (*outbuf && !(*outbuf)->empty()) {
            cb->saslSendClient(std::move(*outbuf));
          }
          if (serverHandshake->isContextEstablished()) {
            cb->saslComplete();
          }
        });
    }));
  });
  if (exw) {
    // If we fail to schedule.
    cb->saslError(std::move(exw));
  }
}

std::unique_ptr<IOBuf> GssSaslServer::encrypt(
    std::unique_ptr<IOBuf>&& buf) {
  return serverHandshake_->wrapMessage(std::move(buf));
}

std::unique_ptr<IOBuf> GssSaslServer::decrypt(
    std::unique_ptr<IOBuf>&& buf) {
  return serverHandshake_->unwrapMessage(std::move(buf));
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
