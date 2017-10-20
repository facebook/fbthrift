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

#include <thrift/lib/cpp2/transport/http2/common/testutil/ChannelTestFixture.h>

#include <folly/io/async/EventBaseManager.h>

namespace apache {
namespace thrift {

using std::string;
using std::unordered_map;
using folly::EventBase;
using folly::EventBaseManager;
using folly::IOBuf;
using proxygen::HTTPMessage;

ChannelTestFixture::ChannelTestFixture() {
  eventBase_ = std::make_unique<EventBase>();
  EventBaseManager::get()->setEventBase(eventBase_.get(), true);
  responseHandler_ = std::make_unique<FakeResponseHandler>(eventBase_.get());
}

void ChannelTestFixture::sendAndReceiveStream(
    std::shared_ptr<H2ChannelIf> channel,
    const unordered_map<string, string>& inputHeaders,
    const string& inputPayload,
    string::size_type chunkSize,
    unordered_map<string, string>*& outputHeaders,
    IOBuf*& outputPayload) {
  eventBase_->runInEventBaseThread([&]() {
    auto msg = std::make_unique<HTTPMessage>();
    auto& headers = msg->getHeaders();
    for (auto it = inputHeaders.begin(); it != inputHeaders.end(); ++it) {
      headers.rawSet(it->first, it->second);
    }
    channel->onH2StreamBegin(std::move(msg));
    const char* data = inputPayload.data();
    string::size_type len = inputPayload.length();
    string::size_type incr = (chunkSize == 0) ? len : chunkSize;
    for (string::size_type i = 0; i < inputPayload.length(); i += incr) {
      auto iobuf = IOBuf::copyBuffer(data + i, std::min(incr, len - i));
      channel->onH2BodyFrame(std::move(iobuf));
    }
    channel->onH2StreamEnd();
  });
  eventBase_->loop();
  // The loop exits when FakeResponseHandler::sendEOM() is called.
  outputHeaders = responseHandler_->getHeaders();
  outputPayload = responseHandler_->getBodyBuf();
}

string ChannelTestFixture::toString(IOBuf* buf) {
  // Clone so we do not destroy the IOBuf - just in case.
  return buf->clone()->moveToFbString().toStdString();
}

} // namespace thrift
} // namespace apache
