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

#pragma once

#include <gtest/gtest.h>

#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp2/transport/http2/common/H2Channel.h>
#include <thrift/lib/cpp2/transport/http2/common/testutil/FakeResponseHandler.h>
#include <memory>
#include <string>
#include <unordered_map>

namespace apache {
namespace thrift {

/**
 * For testing of a single HTTP/2 stream of activity.  Mimics how a
 * proxygen::RequestHandler object interacts with an H2Channel
 * object.
 */
class ChannelTestFixture : public testing::Test {
 public:
  // Sets up for a test.
  ChannelTestFixture();

  // Tears down after the test.
  ~ChannelTestFixture() override {}

  // Sends a HTTP/2 stream described by (inputHeaders, inputPayload) to
  // the channel and waits for the response stream and returns it as
  // (outputHeaders, outputPayload).  The input payload is provided to
  // the channel in chunks as specified by the chunkSize.  If
  // chunkSize is 0, the entire payload is sent as a single chunk.
  void sendAndReceiveStream(
      std::shared_ptr<H2Channel> channel,
      const std::unordered_map<std::string, std::string>& inputHeaders,
      const std::string& inputPayload,
      std::string::size_type chunkSize,
      std::unordered_map<std::string, std::string>*& outputHeaders,
      folly::IOBuf*& outputPayload);

 protected:
  std::unique_ptr<folly::EventBase> eventBase_;
  std::unique_ptr<FakeResponseHandler> responseHandler_;

  std::string toString(folly::IOBuf* buf);
};

} // namespace thrift
} // namespace apache
