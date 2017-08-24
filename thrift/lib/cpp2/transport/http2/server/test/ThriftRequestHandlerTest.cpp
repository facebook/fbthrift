/*
 * Copyright 2017-present Facebook, Inc.
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

#include <gtest/gtest.h>

#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventBaseManager.h>
#include <thrift/lib/cpp2/transport/http2/common/testutil/FakeProcessors.h>
#include <thrift/lib/cpp2/transport/http2/common/testutil/FakeResponseHandler.h>
#include <thrift/lib/cpp2/transport/http2/server/ThriftRequestHandler.h>
#include <memory>

namespace apache {
namespace thrift {

using folly::EventBase;
using folly::EventBaseManager;
using folly::IOBuf;
using proxygen::HTTPMessage;

class ThriftRequestHandlerTest : public testing::Test {
 public:
  // Sets up for a test.
  // TODO: Parameterize for different channel implementations.
  ThriftRequestHandlerTest() {
    eventBase_ = std::make_unique<EventBase>();
    EventBaseManager::get()->setEventBase(eventBase_.get(), true);
    responseHandler_ = std::make_unique<FakeResponseHandler>(eventBase_.get());
    processor_ = std::make_unique<EchoProcessor>(
        "extrakey", "extravalue", "<eom>", eventBase_.get());
    // requestHandler_ deletes itself.
    requestHandler_ = new ThriftRequestHandler(processor_.get());
    requestHandler_->setResponseHandler(responseHandler_.get());
  }

  // Tears down after the test.
  ~ThriftRequestHandlerTest() override {}

 protected:
  std::unique_ptr<folly::EventBase> eventBase_;
  std::unique_ptr<FakeResponseHandler> responseHandler_;
  std::unique_ptr<EchoProcessor> processor_;
  ThriftRequestHandler* requestHandler_;
};

// Tests the interaction between ThriftRequestHandler and
// SingleRpcChannel without any errors.
TEST_F(ThriftRequestHandlerTest, SingleRpcChannelNoErrors) {
  auto msg = std::make_unique<HTTPMessage>();
  auto& headers = msg->getHeaders();
  headers.rawSet("key1", "value1");
  headers.rawSet("key2", "value2");
  requestHandler_->onRequest(std::move(msg));
  auto iobuf = IOBuf::copyBuffer("payload");
  requestHandler_->onBody(std::move(iobuf));
  requestHandler_->onEOM();
  eventBase_->loopOnce();
  requestHandler_->requestComplete();
  auto outputHeaders = responseHandler_->getHeaders();
  auto outputPayload = responseHandler_->getBody();
  EXPECT_EQ(3, outputHeaders->size());
  EXPECT_EQ("value1", outputHeaders->at("key1"));
  EXPECT_EQ("value2", outputHeaders->at("key2"));
  EXPECT_EQ("extravalue", outputHeaders->at("extrakey"));
  EXPECT_EQ("payload<eom>", outputPayload);
}

// Tests the interaction between ThriftRequestHandler and
// SingleRpcChannel with an error after the entire RPC has been
// processed.
TEST_F(ThriftRequestHandlerTest, SingleRpcChannelErrorAtEnd) {
  auto msg = std::make_unique<HTTPMessage>();
  auto& headers = msg->getHeaders();
  headers.rawSet("key1", "value1");
  headers.rawSet("key2", "value2");
  requestHandler_->onRequest(std::move(msg));
  auto iobuf = IOBuf::copyBuffer("payload");
  requestHandler_->onBody(std::move(iobuf));
  requestHandler_->onEOM();
  eventBase_->loopOnce();
  requestHandler_->onError(proxygen::kErrorNone);
  auto outputHeaders = responseHandler_->getHeaders();
  auto outputPayload = responseHandler_->getBody();
  EXPECT_EQ(3, outputHeaders->size());
  EXPECT_EQ("value1", outputHeaders->at("key1"));
  EXPECT_EQ("value2", outputHeaders->at("key2"));
  EXPECT_EQ("extravalue", outputHeaders->at("extrakey"));
  EXPECT_EQ("payload<eom>", outputPayload);
}

// Tests the interaction between ThriftRequestHandler and
// SingleRpcChannel with an error after onEOM() - but before the
// callbacks take place.
TEST_F(ThriftRequestHandlerTest, SingleRpcChannelErrorBeforeCallbacks) {
  auto msg = std::make_unique<HTTPMessage>();
  auto& headers = msg->getHeaders();
  headers.rawSet("key1", "value1");
  headers.rawSet("key2", "value2");
  requestHandler_->onRequest(std::move(msg));
  auto iobuf = IOBuf::copyBuffer("payload");
  requestHandler_->onBody(std::move(iobuf));
  requestHandler_->onEOM();
  requestHandler_->onError(proxygen::kErrorNone);
  eventBase_->loopOnce();
  auto outputHeaders = responseHandler_->getHeaders();
  auto outputPayload = responseHandler_->getBody();
  EXPECT_EQ(0, outputHeaders->size());
  EXPECT_EQ("", outputPayload);
}

// Tests the interaction between ThriftRequestHandler and
// SingleRpcChannel with an error before onEOM().
TEST_F(ThriftRequestHandlerTest, SingleRpcChannelErrorBeforeEOM) {
  auto msg = std::make_unique<HTTPMessage>();
  auto& headers = msg->getHeaders();
  headers.rawSet("key1", "value1");
  headers.rawSet("key2", "value2");
  requestHandler_->onRequest(std::move(msg));
  auto iobuf = IOBuf::copyBuffer("payload");
  requestHandler_->onBody(std::move(iobuf));
  requestHandler_->onError(proxygen::kErrorNone);
  eventBase_->loopOnce();
  auto outputHeaders = responseHandler_->getHeaders();
  auto outputPayload = responseHandler_->getBody();
  EXPECT_EQ(0, outputHeaders->size());
  EXPECT_EQ("", outputPayload);
}

// Tests the interaction between ThriftRequestHandler and
// SingleRpcChannel with an error before onBody().
TEST_F(ThriftRequestHandlerTest, SingleRpcChannelErrorBeforeOnBody) {
  auto msg = std::make_unique<HTTPMessage>();
  auto& headers = msg->getHeaders();
  headers.rawSet("key1", "value1");
  headers.rawSet("key2", "value2");
  requestHandler_->onRequest(std::move(msg));
  requestHandler_->onError(proxygen::kErrorNone);
  eventBase_->loopOnce();
  auto outputHeaders = responseHandler_->getHeaders();
  auto outputPayload = responseHandler_->getBody();
  EXPECT_EQ(0, outputHeaders->size());
  EXPECT_EQ("", outputPayload);
}

// Tests the interaction between ThriftRequestHandler and
// SingleRpcChannel with an error right at the beginning.
TEST_F(ThriftRequestHandlerTest, SingleRpcChannelErrorAtBeginning) {
  requestHandler_->onError(proxygen::kErrorNone);
  eventBase_->loopOnce();
  auto outputHeaders = responseHandler_->getHeaders();
  auto outputPayload = responseHandler_->getBody();
  EXPECT_EQ(0, outputHeaders->size());
  EXPECT_EQ("", outputPayload);
}

} // namespace thrift
} // namespace apache
