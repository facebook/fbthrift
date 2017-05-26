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

#include <thrift/lib/cpp2/async/StubSaslClient.h>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/concurrency/FunctionRunner.h>
#include <thrift/lib/cpp/concurrency/PosixThreadFactory.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/gen-cpp2/Sasl_types.h>

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

StubSaslClient::StubSaslClient(folly::EventBase* evb,
      const std::shared_ptr<SecurityLogger>& logger,
      int forceMsSpentPerRTT)
    : SaslClient(evb, logger)
    , threadManager_(ThreadManager::newSimpleThreadManager(1 /* count */))
    , phase_(0)
    , forceFallback_(false)
    , forceTimeout_(false)
    , forceMsSpentPerRTT_(forceMsSpentPerRTT) {
  threadManager_->threadFactory(std::make_shared<PosixThreadFactory>());
}

void StubSaslClient::start(Callback *cb) {
  CHECK(phase_ == 0);
  threadManager_->start();

  // Double-dispatch, as the more complex implementation will.
  threadManager_->add(std::make_shared<FunctionRunner>([=] {
        (*evb_)->runInEventBaseThread([=] () mutable {
            cb->saslStarted();
          });

        SaslStart start;
        start.mechanism = MECH;
        start.request.response = RESPONSE0;
        start.__isset.request = true;
        start.request.__isset.response = true;

        IOBufQueue q;
        Serializer<CompactProtocolReader, CompactProtocolWriter> serializer;
        serializer.serialize(start, &q);
        phase_ = 1;

        (*evb_)->runInEventBaseThread([ cb, q = std::move(q) ]() mutable {
          cb->saslSendServer(q.move());
        });}));
}

void StubSaslClient::consumeFromServer(
  Callback *cb, std::unique_ptr<IOBuf>&& message) {
  std::shared_ptr<IOBuf> smessage(std::move(message));

  auto forceTimeout = forceTimeout_;
  auto forceMsSpentPerRTT = forceMsSpentPerRTT_;
  cb->saslStarted();
  threadManager_->add(std::make_shared<FunctionRunner>([=] {
        if (forceTimeout) {
          // sleep override
          sleep(1);
          return;
        }

        if (forceMsSpentPerRTT) {
          // sleep override
          std::this_thread::sleep_for(std::chrono::milliseconds(
            forceMsSpentPerRTT));
        }

        IOBufQueue req_data;
        folly::exception_wrapper ex;
        bool complete = false;

        if (phase_ == 0) {
          ex = folly::make_exception_wrapper<
            thrift::protocol::TProtocolException>("unexpected phase 0");
        } else if (phase_ == 1) {
          if (forceFallback_) {
            ex = folly::make_exception_wrapper<
              thrift::protocol::TProtocolException>(
                "expected challenge 1 force failed");
          } else {
            SaslReply reply;
            Serializer<CompactProtocolReader, CompactProtocolWriter>
              deserializer;
            ex = folly::try_and_catch<std::exception, TProtocolException>(
              [&]() {
                deserializer.deserialize(smessage.get(), reply);
              });
            if (!ex) {
              if (reply.__isset.outcome && !reply.outcome.success) {
                ex = folly::make_exception_wrapper<
                  thrift::protocol::TProtocolException>(
                    "server reports failure in phase 1");
              } else if (reply.__isset.challenge &&
                         reply.challenge == CHALLENGE1) {
                Serializer<CompactProtocolReader, CompactProtocolWriter>
                  serializer;
                SaslRequest req;
                req.response = RESPONSE1;
                req.__isset.response = true;
                serializer.serialize(req, &req_data);
                phase_ = 2;
              } else {
                ex = folly::make_exception_wrapper<
                  thrift::protocol::TProtocolException>("expected challenge 1");
              }
            }
          }
        } else if (phase_ == 2) {
          Serializer<CompactProtocolReader, CompactProtocolWriter> deserializer;
          SaslReply reply;
          ex = folly::try_and_catch<std::exception, TProtocolException>([&]() {
            deserializer.deserialize(smessage.get(), reply);
          });
          if (!ex) {
            if (reply.__isset.outcome &&
                !reply.outcome.success) {
              ex = folly::make_exception_wrapper<
                thrift::protocol::TProtocolException>(
                "server reports failure in phase 2");
            } else if (reply.__isset.outcome &&
                       reply.outcome.success &&
                       reply.__isset.challenge &&
                       reply.challenge == CHALLENGE2) {
              complete = true;
              phase_ = -1;
            } else {
              ex = folly::make_exception_wrapper<
                thrift::protocol::TProtocolException>("expected challenge 2");
            }
          }
        } else if (phase_ == -1) {
          ex = folly::make_exception_wrapper<
            thrift::protocol::TProtocolException>(
              "unexpected message after complete");
        } else {
          ex = folly::make_exception_wrapper<
            thrift::protocol::TProtocolException>(
              "unexpected message after complete");
        }

        if (complete) {
          CHECK(req_data.empty());
          CHECK(!ex);
        } else {
          CHECK(req_data.empty() == !!ex);
        }

        if (ex) {
          phase_ = -2;
        }

        (*evb_)->runInEventBaseThread(
          [=, req_data = std::move(req_data)] () mutable {
            if (!req_data.empty()) {
              cb->saslSendServer(req_data.move());
            }
            if (ex) {
              threadManager_->stop();
              cb->saslError(std::move(ex));
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
  for (uint64_t i = 0; i < buf->length(); i++) {
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
  for (uint32_t i = 0; i < outlen; i++) {
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
