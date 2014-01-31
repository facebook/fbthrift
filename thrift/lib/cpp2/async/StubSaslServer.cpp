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

#include "thrift/lib/cpp2/async/StubSaslServer.h"

#include "folly/io/Cursor.h"
#include "folly/io/IOBuf.h"
#include "folly/io/IOBufQueue.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/concurrency/FunctionRunner.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp/util/ThriftSerializer.h"
#include "thrift/lib/cpp2/gen-cpp2/Sasl_types.h"

#include <memory>

using folly::IOBuf;
using folly::IOBufQueue;
using apache::thrift::concurrency::FunctionRunner;
using apache::thrift::concurrency::PosixThreadFactory;
using apache::thrift::concurrency::ThreadManager;
using apache::thrift::util::ThriftSerializerCompact;
using apache::thrift::sasl::SaslRequest;
using apache::thrift::sasl::SaslReply;
using apache::thrift::sasl::SaslStart;

namespace apache { namespace thrift {

static const char MECH[] = "stub-v1";
static const char RESPONSE0[] = "response0";
static const char CHALLENGE1[] = "challenge1";
static const char RESPONSE1[] = "response1";
static const char CHALLENGE2[] = "challenge2";

StubSaslServer::StubSaslServer(apache::thrift::async::TEventBase* evb)
    : threadManager_(ThreadManager::newSimpleThreadManager(1 /* count */))
    , evb_(evb)
    , phase_(0)
    , forceFallback_(false) {
  threadManager_->threadFactory(std::make_shared<PosixThreadFactory>());
}

void StubSaslServer::consumeFromClient(
  Callback *cb, std::unique_ptr<IOBuf>&& message) {
  std::shared_ptr<IOBuf> smessage(std::move(message));
  threadManager_->start();

  // Double-dispatch, as the more complex implementation will.
  threadManager_->add(std::make_shared<FunctionRunner>([=] {
        std::string reply_data;
        std::exception_ptr ex;
        bool complete = false;

        if (phase_ == 0) {
          SaslStart start;
          ThriftSerializerCompact<SaslStart> deserializer;
          try {
            deserializer.deserialize(smessage->data(), smessage->length(),
                                     &start);
          } catch (...) {
            ex = std::current_exception();
          }
          if (!ex) {
            if (start.mechanism == MECH &&
                start.__isset.request &&
                start.request.__isset.response &&
                start.request.response == RESPONSE0) {
              ThriftSerializerCompact<SaslReply> serializer;
              SaslReply reply;
              reply.challenge = CHALLENGE1;
              reply.__isset.challenge = true;
              serializer.serialize(reply, &reply_data);
              phase_ = 1;
            } else {
              ex = std::make_exception_ptr(
                thrift::protocol::TProtocolException(
                  "expected response 0"));
            }
          }
        } else if (phase_ == 1) {
          if (forceFallback_) {
            ex = std::make_exception_ptr(
              thrift::protocol::TProtocolException(
                "expected response 1 force failed"));
          } else {
            SaslRequest req;
            ThriftSerializerCompact<SaslRequest> deserializer;
            try {
              deserializer.deserialize(smessage->data(), smessage->length(),
                                       &req);
            } catch (...) {
              ex = std::current_exception();
            }
            if (!ex) {
              if (req.__isset.response &&
                  req.response == RESPONSE1) {
                ThriftSerializerCompact<SaslReply> serializer;
                SaslReply reply;
                reply.challenge = CHALLENGE2;
                reply.__isset.challenge = true;
                reply.outcome.success = true;
                reply.__isset.outcome = true;
                reply.outcome.__isset.success = true;
                serializer.serialize(reply, &reply_data);
                complete = true;
                phase_ = -1;
              } else {
                ex = std::make_exception_ptr(
                  thrift::protocol::TProtocolException(
                    "expected response 1"));
              }
            }
          }
        } else if (phase_ == -1) {
          ex = std::make_exception_ptr(
            thrift::protocol::TProtocolException(
              "unexpected message after complete"));
        } else {
          ex = std::make_exception_ptr(
            thrift::protocol::TProtocolException(
              "unexpected message after error"));
        }

        if (!ex && !complete && reply_data.empty()) {
          ThriftSerializerCompact<SaslReply> serializer;
          SaslReply reply;
          reply.outcome.success = false;
          reply.__isset.outcome = true;
          reply.outcome.__isset.success = true;
          serializer.serialize(reply, &reply_data);
        }

        CHECK(reply_data.empty() == !!ex);
        CHECK(!(complete && !!ex));

        if (ex) {
          phase_ = -2;
        }

        evb_->runInEventBaseThread([=] {
            if (!reply_data.empty()) {
              cb->saslSendClient(
                IOBuf::copyBuffer(reply_data.data(), reply_data.length()));
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

std::unique_ptr<IOBuf> StubSaslServer::wrap(std::unique_ptr<IOBuf>&& buf) {
  buf->coalesce();
  uint32_t inlen = buf->length();
  std::unique_ptr<IOBuf> output = IOBuf::create(sizeof(inlen) + inlen);
  folly::io::Appender c(output.get(), 0);
  c.writeBE(inlen);
  // "encrypt" the data
  c.ensure(inlen);
  for (int i = 0; i < inlen; i++) {
    c.writableData()[i] = buf->data()[i] ^ i ^ 0xa5;
  }
  c.append(inlen);
  return output;
}

std::unique_ptr<IOBuf> StubSaslServer::unwrap(IOBufQueue* q,
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
    output->writableData()[i] = input->data()[i] ^ i ^ 0x5a;
  }
  output->append(outlen);
  *remaining = 0;
  return output;
}

std::string StubSaslServer::getClientIdentity() const {
  if (phase_ == -1) {
    return "stub_client_name(remote)";
  } else {
    return "";
  }
}

std::string StubSaslServer::getServerIdentity() const {
  if (phase_ == -1) {
    return "stub_server_name(local)";
  } else {
    return "";
  }
}

}}
