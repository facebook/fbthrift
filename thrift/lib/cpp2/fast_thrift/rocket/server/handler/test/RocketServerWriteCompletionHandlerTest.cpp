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

#include <thrift/lib/cpp2/fast_thrift/rocket/server/handler/RocketServerWriteCompletionHandler.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/transport/WriteCompletion.h>

#include <gtest/gtest.h>

#include <cstdint>
#include <utility>
#include <vector>

namespace apache::thrift::fast_thrift::rocket::server::handler {
namespace {

namespace cp = apache::thrift::fast_thrift::channel_pipeline;
namespace transport = apache::thrift::fast_thrift::transport;

class CapturingContext {
 public:
  template <cp::PipelineEvent E>
  void fireEvent(const typename E::Payload& event) noexcept {
    static_assert(std::same_as<E, RocketWriteCompleteEvent>);
    events_.push_back(event);
  }

  const std::vector<RocketWriteCompleteEvent>& events() const noexcept {
    return events_;
  }

 private:
  std::vector<RocketWriteCompleteEvent> events_;
};

FrameWriteCompleteEvent frameWriteComplete(
    uint32_t streamId,
    transport::WriteCompletionStatus status,
    bool quiesced = false) noexcept {
  return {.streamId = streamId, .status = status, .quiesced = quiesced};
}

} // namespace

TEST(
    RocketServerWriteCompletionHandlerTest, FrameCompletionBecomesRocketEvent) {
  RocketServerWriteCompletionHandler handler;
  CapturingContext ctx;
  handler.on<FrameWriteCompleteEvent>(
      ctx, frameWriteComplete(7, transport::WriteCompletionStatus::Success));

  ASSERT_EQ(ctx.events().size(), 1u);
  EXPECT_EQ(ctx.events()[0].streamId, 7u);
  EXPECT_EQ(ctx.events()[0].status, transport::WriteCompletionStatus::Success);
}

TEST(RocketServerWriteCompletionHandlerTest, ErrorStatusIsForwarded) {
  RocketServerWriteCompletionHandler handler;
  CapturingContext ctx;

  handler.on<FrameWriteCompleteEvent>(
      ctx, frameWriteComplete(9, transport::WriteCompletionStatus::Error));

  ASSERT_EQ(ctx.events().size(), 1u);
  EXPECT_EQ(ctx.events()[0].streamId, 9u);
  EXPECT_EQ(ctx.events()[0].status, transport::WriteCompletionStatus::Error);
}

TEST(RocketServerWriteCompletionHandlerTest, EachFrameFiresItsOwnEvent) {
  RocketServerWriteCompletionHandler handler;
  CapturingContext ctx;

  // A batch that carried several frames arrives as several FrameWriteCompletes
  // (FragmentCompletionTracker already split it); the handler must not collapse
  // them.
  handler.on<FrameWriteCompleteEvent>(
      ctx, frameWriteComplete(1, transport::WriteCompletionStatus::Success));
  handler.on<FrameWriteCompleteEvent>(
      ctx, frameWriteComplete(3, transport::WriteCompletionStatus::Success));

  ASSERT_EQ(ctx.events().size(), 2u);
  EXPECT_EQ(ctx.events()[0].streamId, 1u);
  EXPECT_EQ(ctx.events()[1].streamId, 3u);
}

TEST(RocketServerWriteCompletionHandlerTest, QuiescenceIsRelayed) {
  RocketServerWriteCompletionHandler handler;
  CapturingContext ctx;

  handler.on<FrameWriteCompleteEvent>(
      ctx,
      frameWriteComplete(
          5, transport::WriteCompletionStatus::Success, /*quiesced=*/true));

  ASSERT_EQ(ctx.events().size(), 1u);
  EXPECT_TRUE(ctx.events()[0].quiesced);
}

} // namespace apache::thrift::fast_thrift::rocket::server::handler
