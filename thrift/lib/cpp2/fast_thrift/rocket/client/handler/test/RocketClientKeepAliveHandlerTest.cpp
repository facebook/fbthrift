/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <folly/io/async/EventBase.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/HandlerTag.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/test/MockAdapters.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/FrameParser.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/FrameHeaders.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/FrameWriter.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/client/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/rocket/client/handler/RocketClientKeepAliveHandler.h>

namespace apache::thrift::fast_thrift::rocket::client::handler {

namespace cp = apache::thrift::fast_thrift::channel_pipeline;
namespace frame = apache::thrift::fast_thrift::frame;

namespace {

HANDLER_TAG(client_keepalive);

RocketResponseMessage makeKeepAliveReply() {
  auto bytes = frame::write::serialize(
      frame::write::KeepAliveHeader{
          .lastReceivedPosition = 0,
          .respond = false,
      },
      nullptr);
  RocketResponseMessage response;
  response.payload = frame::read::parseFrame(std::move(bytes));
  return response;
}

class ClientKeepAliveHandlerTest : public ::testing::Test {
 protected:
  using PipelineBuilder = cp::PipelineBuilder<
      cp::test::MockHeadHandler,
      cp::test::MockTailHandler,
      cp::test::TestAllocator>;

  folly::EventBase eventBase_;
  cp::test::MockHeadHandler transport_;
  cp::test::MockTailHandler app_;
  cp::test::TestAllocator allocator_;
};

TEST_F(ClientKeepAliveHandlerTest, SendsProbesAndTimesOutMissingReplies) {
  size_t probes = 0;
  transport_.setOnWriteCallback([&probes](cp::TypeErasedBox&& box) noexcept {
    auto request = box.take<RocketRequestMessage>();
    EXPECT_EQ(request.frame.frameType, frame::FrameType::KEEPALIVE);
    EXPECT_EQ(request.frame.streamId, frame::kConnectionStreamId);
    EXPECT_TRUE(request.frame.respond);
    ++probes;
    return cp::Result::Success;
  });

  auto pipeline = PipelineBuilder()
                      .setEventBase(&eventBase_)
                      .setHead(&transport_)
                      .setTail(&app_)
                      .setAllocator(&allocator_)
                      .addNextDuplex<RocketClientKeepAliveHandler>(
                          client_keepalive_tag,
                          RSocketKeepAliveConfig{
                              .intervalMs = 2,
                              .maxLifetimeMs = 12,
                          })
                      .build();

  pipeline->activate();
  eventBase_.loop();

  EXPECT_GE(probes, 1u);
  EXPECT_EQ(app_.exceptionCount(), 1);
}

TEST_F(ClientKeepAliveHandlerTest, RepliesKeepConnectionAlive) {
  size_t probes = 0;
  cp::PipelineImpl* pipelinePtr = nullptr;
  transport_.setOnWriteCallback([&](cp::TypeErasedBox&& box) noexcept {
    auto request = box.take<RocketRequestMessage>();
    EXPECT_EQ(request.frame.frameType, frame::FrameType::KEEPALIVE);
    EXPECT_TRUE(request.frame.respond);
    ++probes;
    EXPECT_EQ(
        pipelinePtr->fireRead(cp::erase_and_box(makeKeepAliveReply())),
        cp::Result::Success);
    if (probes == 3) {
      pipelinePtr->deactivate();
    }
    return cp::Result::Success;
  });

  auto pipeline = PipelineBuilder()
                      .setEventBase(&eventBase_)
                      .setHead(&transport_)
                      .setTail(&app_)
                      .setAllocator(&allocator_)
                      .addNextDuplex<RocketClientKeepAliveHandler>(
                          client_keepalive_tag,
                          RSocketKeepAliveConfig{
                              .intervalMs = 2,
                              .maxLifetimeMs = 8,
                          })
                      .build();
  pipelinePtr = pipeline.get();

  pipeline->activate();
  eventBase_.loop();

  EXPECT_EQ(probes, 3u);
  EXPECT_EQ(app_.exceptionCount(), 0);
  EXPECT_EQ(app_.readCount(), 0);
}

TEST_F(ClientKeepAliveHandlerTest, DeferredStartWaitsForFirstInboundFrame) {
  size_t probes = 0;
  cp::PipelineImpl* pipelinePtr = nullptr;
  transport_.setOnWriteCallback([&](cp::TypeErasedBox&& box) noexcept {
    auto request = box.take<RocketRequestMessage>();
    EXPECT_EQ(request.frame.frameType, frame::FrameType::KEEPALIVE);
    ++probes;
    pipelinePtr->deactivate();
    return cp::Result::Success;
  });

  auto pipeline =
      PipelineBuilder()
          .setEventBase(&eventBase_)
          .setHead(&transport_)
          .setTail(&app_)
          .setAllocator(&allocator_)
          .addNextDuplex<RocketClientKeepAliveHandler>(
              client_keepalive_tag,
              RSocketKeepAliveConfig{
                  .intervalMs = 2,
                  .maxLifetimeMs = 12,
              },
              RocketClientKeepAliveHandler::StartPolicy::FirstInboundFrame)
          .build();
  pipelinePtr = pipeline.get();

  pipeline->activate();
  eventBase_.loop();
  EXPECT_EQ(probes, 0u);

  EXPECT_EQ(
      pipeline->fireRead(cp::erase_and_box(makeKeepAliveReply())),
      cp::Result::Success);
  eventBase_.loop();
  EXPECT_EQ(probes, 1u);
  EXPECT_EQ(app_.exceptionCount(), 0);
}

} // namespace
} // namespace apache::thrift::fast_thrift::rocket::client::handler
