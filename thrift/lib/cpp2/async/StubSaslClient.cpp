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

#include "thrift/lib/cpp2/async/StubSaslClient.h"

#include "folly/io/Cursor.h"
#include "folly/MoveWrapper.h"
#include "folly/io/IOBuf.h"
#include "folly/io/IOBufQueue.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/concurrency/FunctionRunner.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp2/protocol/Serializer.h"
#include "thrift/lib/cpp2/gen-cpp2/Sasl_types.h"

#include <memory>

using folly::IOBuf;
using folly::IOBufQueue;
using apache::thrift::concurrency::FunctionRunner;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using apache::thrift::sasl::SaslRequest;
using apache::thrift::sasl::SaslReply;
using apache::thrift::sasl::SaslStart;

namespace apache { namespace thrift {

static const char MECH[] = "stub-v1";
static const char RESPONSE0[] = "response0";
static const char CHALLENGE1[] = "challenge1";
static const char RESPONSE1[] = "response1";
static const char CHALLENGE2[] = "challenge2";

StubSaslClient::StubSaslClient(apache::thrift::async::TEventBase* evb)
    : threadManager_(ThreadManager::newSimpleThreadManager(1 /* count */))
    , evb_(evb)
    , phase_(0)
    , forceFallback_(false) {
  threadManager_->threadFactory(std::make_shared<PosixThreadFactory>());
}

void StubSaslClient::start(Callback *cb) {
  CHECK(phase_ == 0);
  threadManager_->start();

  // Double-dispatch, as the more complex implementation will.
  threadManager_->add(std::make_shared<FunctionRunner>([=] {
        SaslStart start;
        start.mechanism = MECH;
        start.request.response = RESPONSE0;
        start.__isset.request = true;
        start.request.__isset.response = true;

        folly::MoveWrapper<IOBufQueue> q;
        Serializer<CompactProtocolReader, CompactProtocolWriter> serializer;
        serializer.serialize(start, &*q);
        phase_ = 1;

        evb_->runInEventBaseThread([=] () mutable {
            cb->saslSendServer(q->move());
          });}));
}

void StubSaslClient::consumeFromServer(
  Callback *cb, std::unique_ptr<IOBuf>&& message) {
  std::shared_ptr<IOBuf> smessage(std::move(message));

  threadManager_->add(std::make_shared<FunctionRunner>([=] {
        folly::MoveWrapper<IOBufQueue> req_data;
        std::exception_ptr ex;
        bool complete = false;

        if (phase_ == 0) {
          ex = std::make_exception_ptr(
            thrift::protocol::TProtocolException("unexpected phase 0"));
        } else if (phase_ == 1) {
          if (forceFallback_) {
            ex = std::make_exception_ptr(
              thrift::protocol::TProtocolException(
                "expected challenge 1 force failed"));
          } else {
            SaslReply reply;
            Serializer<CompactProtocolReader, CompactProtocolWriter>
              deserializer;
            try {
              deserializer.deserialize(smessage.get(), reply);
            } catch (...) {
              ex = std::current_exception();
            }
            if (!ex) {
              if (reply.__isset.outcome && !reply.outcome.success) {
                ex = std::make_exception_ptr(
                  thrift::protocol::TProtocolException(
                    "server reports failure in phase 1"));
              } else if (reply.__isset.challenge &&
                         reply.challenge == CHALLENGE1) {
                Serializer<CompactProtocolReader, CompactProtocolWriter>
                  serializer;
                SaslRequest req;
                req.response = RESPONSE1;
                req.__isset.response = true;
                serializer.serialize(req, &*req_data);
                phase_ = 2;
              } else {
                ex = std::make_exception_ptr(
                  thrift::protocol::TProtocolException("expected challenge 1"));
              }
            }
          }
        } else if (phase_ == 2) {
          Serializer<CompactProtocolReader, CompactProtocolWriter> deserializer;
          SaslReply reply;
          try {
            deserializer.deserialize(smessage.get(), reply);
          } catch (...) {
            ex = std::current_exception();
          }
          if (!ex) {
            if (reply.__isset.outcome &&
                !reply.outcome.success) {
              ex = std::make_exception_ptr(
                thrift::protocol::TProtocolException(
                "server reports failure in phase 2"));
            } else if (reply.__isset.outcome &&
                       reply.outcome.success &&
                       reply.__isset.challenge &&
                       reply.challenge == CHALLENGE2) {
              complete = true;
              phase_ = -1;
            } else {
              ex = std::make_exception_ptr(
                thrift::protocol::TProtocolException("expected challenge 2"));
            }
          }
        } else if (phase_ == -1) {
          ex = std::make_exception_ptr(
            thrift::protocol::TProtocolException(
              "unexpected message after complete"));
        } else {
          ex = std::make_exception_ptr(
            thrift::protocol::TProtocolException(
              "unexpected message after complete"));
        }

        if (complete) {
          CHECK(req_data->empty());
          CHECK(!ex);
        } else {
          CHECK(req_data->empty() == !!ex);
        }

        if (ex) {
          phase_ = -2;
        }

        evb_->runInEventBaseThread([=] () mutable {
            if (!req_data->empty()) {
              cb->saslSendServer(req_data->move());
            }
            if (ex) {
              threadManager_->stop();
              cb->saslError(std::exception_ptr(ex));
            }
            if (complete) {
              threadManager_->stop();
              cb->saslComplete();
            }
          });
      }));
}

std::unique_ptr<IOBuf> StubSaslClient::wrap(std::unique_ptr<IOBuf>&& buf) {
  buf->coalesce();
  uint32_t inlen = buf->length();
  std::unique_ptr<IOBuf> output = IOBuf::create(sizeof(inlen) + inlen);
  folly::io::Appender c(output.get(), 0);
  c.writeBE(inlen);
  // "encrypt" the data
  c.ensure(buf->length());
  for (int i = 0; i < buf->length(); i++) {
    c.writableData()[i] = buf->data()[i] ^ i ^ 0x5a;
  }
  c.append(buf->length());
  return output;
}

std::unique_ptr<IOBuf> StubSaslClient::unwrap(IOBufQueue* q,
                                              size_t* remaining) {
  folly::io::Cursor c(q->front());
  size_t chainSize = q->front()->computeChainDataLength();
  uint32_t outlen = 0;

  if (chainSize < sizeof(outlen)) {
    *remaining = sizeof(outlen) - chainSize;
    return nullptr;
  }

  outlen = c.readBE<uint32_t>();

  if (chainSize < sizeof(outlen) + outlen) {
    *remaining = sizeof(outlen) + outlen - chainSize;
    return nullptr;
  }

  // "decrypt" the data
  q->trimStart(sizeof(outlen));
  std::unique_ptr<IOBuf> input = q->split(outlen);
  input->coalesce();
  std::unique_ptr<IOBuf> output = IOBuf::create(outlen);
  for (int i = 0; i < outlen; i++) {
    output->writableData()[i] = input->data()[i] ^ i ^ 0xa5;
  }
  output->append(outlen);
  *remaining = 0;
  return output;
}

std::string StubSaslClient::getClientIdentity() const {
  if (phase_ == -1) {
    return "stub_client_name(local)";
  } else {
    return "";
  }
}

std::string StubSaslClient::getServerIdentity() const {
  if (phase_ == -1) {
    return "stub_server_name(remote)";
  } else {
    return "";
  }
}

}}
