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

// Unit tests for ThriftServerStreamMuxHandler. A real main pipeline
// (MockHead <- mux <- MockTail) stands in for the thrift server pipeline: an
// outbound ThriftServerStreamOpenPayload opens a stream, inbound
// RequestN/Cancel drive/terminate it, and everything reaching the wire
// (MockHead) is recorded. The per-stream producer is a synchronous sample
// FiniteProducer supplied via a ProducerPipeline::ConfigFunc, exactly as an
// application would.

#include <thrift/lib/cpp2/fast_thrift/thrift/server/handler/ThriftServerStreamMuxHandler.h>

#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <folly/ExceptionWrapper.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/HandlerTag.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/detail/ContextImpl.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/test/MockAdapters.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftControlPayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftRequestPayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftResponsePayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/Messages.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/StreamResponsePayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/ProducerPipeline.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>

namespace apache::thrift::fast_thrift::thrift {

namespace cp = channel_pipeline;

namespace {

using Context = cp::detail::ContextImpl;

// A synchronous sample producer: on each RequestN it emits up to `n` payloads
// (bounded by the total it was given) then a single Complete; consumes Cancel.
template <typename Ctx>
class FiniteProducer {
 public:
  explicit FiniteProducer(uint64_t total) noexcept : remaining_(total) {}

  void handlerAdded(Ctx& /*ctx*/) noexcept {}
  void handlerRemoved(Ctx& /*ctx*/) noexcept {}
  void onPipelineActive(Ctx& /*ctx*/) noexcept {}
  void onPipelineInactive(Ctx& /*ctx*/) noexcept {}
  void onReadReady(Ctx& /*ctx*/) noexcept {}
  void onWriteReady(Ctx& /*ctx*/) noexcept {}
  void onException(Ctx& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }
  cp::Result onWrite(Ctx& ctx, cp::TypeErasedBox&& msg) noexcept {
    return ctx.fireWrite(std::move(msg));
  }
  cp::Result onRead(Ctx& ctx, cp::TypeErasedBox&& msg) noexcept {
    auto& m = msg.get<stream::ThriftStreamMessage>();
    if (m.payload.is<stream::RequestN>()) {
      const uint64_t n = m.payload.get<stream::RequestN>().n;
      for (uint64_t i = 0; i < n && remaining_ > 0; ++i) {
        (void)ctx.fireWrite(
            cp::erase_and_box(
                stream::ThriftStreamMessage{.payload = stream::Payload{}}));
        --remaining_;
      }
      if (remaining_ == 0 && !completed_) {
        completed_ = true;
        (void)ctx.fireWrite(
            cp::erase_and_box(
                stream::ThriftStreamMessage{.payload = stream::Complete{}}));
      }
      return cp::Result::Success;
    }
    if (m.payload.is<stream::Cancel>()) {
      return cp::Result::Success; // consume; a real producer stops producing
    }
    return ctx.fireRead(std::move(msg));
  }

 private:
  uint64_t remaining_;
  bool completed_{false};
};

// A sample producer that fails the stream: on the first RequestN it emits a
// single stream::Error carrying the given exception.
template <typename Ctx>
class ErrorProducer {
 public:
  void handlerAdded(Ctx& /*ctx*/) noexcept {}
  void handlerRemoved(Ctx& /*ctx*/) noexcept {}
  void onPipelineActive(Ctx& /*ctx*/) noexcept {}
  void onPipelineInactive(Ctx& /*ctx*/) noexcept {}
  void onReadReady(Ctx& /*ctx*/) noexcept {}
  void onWriteReady(Ctx& /*ctx*/) noexcept {}
  void onException(Ctx& ctx, folly::exception_wrapper&& e) noexcept {
    ctx.fireException(std::move(e));
  }
  cp::Result onWrite(Ctx& ctx, cp::TypeErasedBox&& msg) noexcept {
    return ctx.fireWrite(std::move(msg));
  }
  cp::Result onRead(Ctx& ctx, cp::TypeErasedBox&& msg) noexcept {
    auto& m = msg.get<stream::ThriftStreamMessage>();
    if (m.payload.is<stream::RequestN>()) {
      if (!failed_) {
        failed_ = true;
        (void)ctx.fireWrite(
            cp::erase_and_box(
                stream::ThriftStreamMessage{
                    .payload = stream::Error{
                        .ex = folly::make_exception_wrapper<std::runtime_error>(
                            "boom")}}));
      }
      return cp::Result::Success;
    }
    if (m.payload.is<stream::Cancel>()) {
      return cp::Result::Success;
    }
    return ctx.fireRead(std::move(msg));
  }

 private:
  bool failed_{false};
};

HANDLER_TAG(mux);
HANDLER_TAG(producer);

// Builds a ProducerPipeline::ConfigFunc that composes a FiniteProducer between
// the framework-owned endpoints.
std::unique_ptr<stream::ProducerPipeline::ConfigFunc> makeRecipe(
    uint64_t total) {
  return std::make_unique<stream::ProducerPipeline::ConfigFunc>(
      [total](stream::ProducerPipeline::Builder& builder) {
        builder.addNextDuplex<FiniteProducer<Context>>(
            producer_tag, std::make_unique<FiniteProducer<Context>>(total));
      });
}

std::unique_ptr<stream::ProducerPipeline::ConfigFunc> makeErrorRecipe() {
  return std::make_unique<stream::ProducerPipeline::ConfigFunc>(
      [](stream::ProducerPipeline::Builder& builder) {
        builder.addNextDuplex<ErrorProducer<Context>>(
            producer_tag, std::make_unique<ErrorProducer<Context>>());
      });
}

// Returns the chunk's exception metadata if it encodes a stream exception
// (appUnknownException), else nullptr.
const apache::thrift::PayloadExceptionMetadataBase* exceptionMetadata(
    const ThriftStreamPayload& sp) {
  if (sp.metadata == nullptr) {
    return nullptr;
  }
  const auto& payloadMetadataRef =
      std::as_const(*sp.metadata).payloadMetadata();
  if (!payloadMetadataRef ||
      payloadMetadataRef->getType() !=
          apache::thrift::PayloadMetadata::Type::exceptionMetadata) {
    return nullptr;
  }
  return &payloadMetadataRef->get_exceptionMetadata();
}

// A recorded outbound chunk that reached the wire (MockHead), reduced to the
// fields the tests assert on.
struct Recorded {
  enum class Kind { InitialResponse, Payload, Complete, Error, Other };
  Kind kind;
  uint32_t streamId;
  std::string exName{};
  std::string exWhat{};
};

ThriftServerResponseMessage makeOpen(uint32_t streamId, uint64_t total) {
  ThriftServerResponseMessage msg;
  msg.payload = ThriftServerStreamOpenPayload{
      .initialResponse =
          ThriftStreamInitialResponsePayload{.streamId = streamId},
      .configFunc = makeRecipe(total)};
  return msg;
}

ThriftServerResponseMessage makeErrorOpen(uint32_t streamId) {
  ThriftServerResponseMessage msg;
  msg.payload = ThriftServerStreamOpenPayload{
      .initialResponse =
          ThriftStreamInitialResponsePayload{.streamId = streamId},
      .configFunc = makeErrorRecipe()};
  return msg;
}

ThriftServerRequestMessage makeRequestN(uint32_t streamId, uint32_t n) {
  ThriftServerRequestMessage msg;
  msg.payload = ThriftRequestNPayload{.streamId = streamId, .requestN = n};
  msg.streamId = streamId;
  return msg;
}

ThriftServerRequestMessage makeCancel(uint32_t streamId) {
  ThriftServerRequestMessage msg;
  msg.payload = ThriftCancelPayload{.streamId = streamId};
  msg.streamId = streamId;
  return msg;
}

class ThriftServerStreamMuxHandlerTest : public ::testing::Test {
 protected:
  cp::PipelineImpl::Ptr build() {
    head_.setOnWriteCallback([this](cp::TypeErasedBox&& box) -> cp::Result {
      auto& resp = box.get<ThriftServerResponseMessage>();
      auto& p = resp.payload;
      if (p.is<ThriftStreamInitialResponsePayload>()) {
        recorded_.push_back(
            {Recorded::Kind::InitialResponse,
             p.get<ThriftStreamInitialResponsePayload>().streamId});
      } else if (p.is<ThriftStreamPayload>()) {
        auto& sp = p.get<ThriftStreamPayload>();
        // A stream exception rides a terminal PAYLOAD chunk carrying
        // appUnknownException metadata (see ThriftServerStreamMuxHandler).
        const auto* exBase = exceptionMetadata(sp);
        if (exBase != nullptr) {
          recorded_.push_back(
              {Recorded::Kind::Error,
               sp.streamId,
               exBase->name_utf8().value_or(""),
               exBase->what_utf8().value_or("")});
        } else {
          recorded_.push_back(
              {sp.complete ? Recorded::Kind::Complete : Recorded::Kind::Payload,
               sp.streamId});
        }
      } else if (p.is<ThriftErrorPayload>()) {
        recorded_.push_back(
            {Recorded::Kind::Error, p.get<ThriftErrorPayload>().streamId});
      } else {
        recorded_.push_back({Recorded::Kind::Other, 0});
      }
      return cp::Result::Success;
    });
    // Inbound exceptions the mux fires (e.g. for an unknown stream) propagate
    // to the tail; capture them so tests can assert on them.
    tail_.setOnExceptionCallback(
        [this](folly::exception_wrapper&& e) { exceptions_.push_back(e); });
    return cp::PipelineBuilder<
               cp::test::MockHeadHandler,
               cp::test::MockTailHandler,
               cp::test::TestAllocator>()
        .setEventBase(&evb_)
        .setHead(&head_)
        .setTail(&tail_)
        .setAllocator(&alloc_)
        .addNextDuplex<ThriftServerStreamMuxHandler<Context>>(mux_tag)
        .build();
  }

  folly::EventBase evb_;
  cp::test::TestAllocator alloc_;
  cp::test::MockHeadHandler head_;
  cp::test::MockTailHandler tail_;
  std::vector<Recorded> recorded_;
  std::vector<folly::exception_wrapper> exceptions_;
};

} // namespace

TEST_F(ThriftServerStreamMuxHandlerTest, OpenForwardsInitialResponse) {
  auto pipeline = build();
  (void)pipeline->fireWrite(cp::erase_and_box(makeOpen(/*streamId=*/2, 3)));

  ASSERT_EQ(recorded_.size(), 1u);
  EXPECT_EQ(recorded_[0].kind, Recorded::Kind::InitialResponse);
  EXPECT_EQ(recorded_[0].streamId, 2u);
}

#ifndef NDEBUG
// The wire protocol is expected to reject a duplicate stream-open before it
// reaches the mux; a collision here is a framework bug. Verify the debug
// backstop fires rather than silently dropping the second sub-pipeline.
TEST_F(ThriftServerStreamMuxHandlerTest, DuplicateStreamOpenDchecks) {
  auto pipeline = build();
  (void)pipeline->fireWrite(cp::erase_and_box(makeOpen(/*streamId=*/2, 3)));
  EXPECT_DEATH(
      (void)pipeline->fireWrite(cp::erase_and_box(makeOpen(/*streamId=*/2, 3))),
      "duplicate stream id");
}
#endif

TEST_F(ThriftServerStreamMuxHandlerTest, RequestNDrivesProducerOutputToWire) {
  auto pipeline = build();
  (void)pipeline->fireWrite(cp::erase_and_box(makeOpen(/*streamId=*/2, 3)));
  (void)pipeline->fireRead(cp::erase_and_box(makeRequestN(2, /*n=*/10)));

  // Initial response, then 3 payloads, then a completion, all on streamId 2.
  ASSERT_EQ(recorded_.size(), 5u);
  EXPECT_EQ(recorded_[0].kind, Recorded::Kind::InitialResponse);
  EXPECT_EQ(recorded_[1].kind, Recorded::Kind::Payload);
  EXPECT_EQ(recorded_[2].kind, Recorded::Kind::Payload);
  EXPECT_EQ(recorded_[3].kind, Recorded::Kind::Payload);
  EXPECT_EQ(recorded_[4].kind, Recorded::Kind::Complete);
  for (const auto& r : recorded_) {
    EXPECT_EQ(r.streamId, 2u);
  }
}

TEST_F(ThriftServerStreamMuxHandlerTest, CompletedStreamIsTornDown) {
  auto pipeline = build();
  (void)pipeline->fireWrite(cp::erase_and_box(makeOpen(/*streamId=*/2, 1)));
  (void)pipeline->fireRead(cp::erase_and_box(makeRequestN(2, /*n=*/10)));
  const size_t afterComplete = recorded_.size();

  // The stream is gone; a late RequestN produces nothing on the wire and
  // surfaces an exception rather than being dropped silently.
  (void)pipeline->fireRead(cp::erase_and_box(makeRequestN(2, /*n=*/10)));
  EXPECT_EQ(recorded_.size(), afterComplete);
  EXPECT_EQ(exceptions_.size(), 1u);
}

TEST_F(ThriftServerStreamMuxHandlerTest, ConcurrentStreamsRoutedIndependently) {
  auto pipeline = build();
  (void)pipeline->fireWrite(cp::erase_and_box(makeOpen(/*streamId=*/2, 1)));
  (void)pipeline->fireWrite(cp::erase_and_box(makeOpen(/*streamId=*/4, 1)));
  recorded_.clear();

  (void)pipeline->fireRead(cp::erase_and_box(makeRequestN(4, /*n=*/10)));
  // Only stream 4 produced: one payload + completion, all tagged streamId 4.
  ASSERT_EQ(recorded_.size(), 2u);
  EXPECT_EQ(recorded_[0].streamId, 4u);
  EXPECT_EQ(recorded_[1].streamId, 4u);
  EXPECT_EQ(recorded_[1].kind, Recorded::Kind::Complete);
}

TEST_F(ThriftServerStreamMuxHandlerTest, CancelStopsAndTearsDownStream) {
  auto pipeline = build();
  (void)pipeline->fireWrite(cp::erase_and_box(makeOpen(/*streamId=*/2, 100)));
  (void)pipeline->fireRead(cp::erase_and_box(makeRequestN(2, /*n=*/2)));
  const size_t afterGrant = recorded_.size(); // 1 initial + 2 payloads

  (void)pipeline->fireRead(cp::erase_and_box(makeCancel(2)));
  // After cancel the stream is torn down; further demand produces nothing and
  // surfaces an exception (the stream is no longer known).
  (void)pipeline->fireRead(cp::erase_and_box(makeRequestN(2, /*n=*/10)));
  EXPECT_EQ(recorded_.size(), afterGrant);
  EXPECT_EQ(exceptions_.size(), 1u);
}

TEST_F(
    ThriftServerStreamMuxHandlerTest, ErrorSurfacesAppExceptionAndTearsDown) {
  auto pipeline = build();
  (void)pipeline->fireWrite(cp::erase_and_box(makeErrorOpen(/*streamId=*/2)));
  (void)pipeline->fireRead(cp::erase_and_box(makeRequestN(2, /*n=*/1)));

  // Initial response, then a terminal error chunk carrying the exception's
  // name/what as appUnknownException metadata.
  ASSERT_EQ(recorded_.size(), 2u);
  EXPECT_EQ(recorded_[0].kind, Recorded::Kind::InitialResponse);
  EXPECT_EQ(recorded_[1].kind, Recorded::Kind::Error);
  EXPECT_EQ(recorded_[1].streamId, 2u);
  EXPECT_NE(recorded_[1].exName.find("runtime_error"), std::string::npos);
  EXPECT_NE(recorded_[1].exWhat.find("boom"), std::string::npos);

  // The terminal tears the stream down; further demand produces nothing and
  // surfaces an exception (the stream is no longer known).
  const size_t afterError = recorded_.size();
  (void)pipeline->fireRead(cp::erase_and_box(makeRequestN(2, /*n=*/10)));
  EXPECT_EQ(recorded_.size(), afterError);
  EXPECT_EQ(exceptions_.size(), 1u);
}

TEST_F(ThriftServerStreamMuxHandlerTest, UnknownStreamFiresException) {
  auto pipeline = build();

  // No stream was ever opened for this id. A RequestN for it is surfaced as an
  // exception (so the connection sees the protocol violation) rather than
  // silently dropped, and produces nothing on the wire.
  (void)pipeline->fireRead(cp::erase_and_box(makeRequestN(/*streamId=*/9, 3)));
  EXPECT_TRUE(recorded_.empty());
  ASSERT_EQ(exceptions_.size(), 1u);
  EXPECT_NE(
      exceptions_[0].what().toStdString().find("unknown stream"),
      std::string::npos);

  // A CANCEL for an unknown stream is surfaced the same way.
  (void)pipeline->fireRead(cp::erase_and_box(makeCancel(/*streamId=*/9)));
  EXPECT_TRUE(recorded_.empty());
  EXPECT_EQ(exceptions_.size(), 2u);
}

TEST_F(ThriftServerStreamMuxHandlerTest, UnaryTrafficPassesThrough) {
  auto pipeline = build();

  // Unary request reaches the app (tail); the mux does not intercept it.
  ThriftServerRequestMessage req;
  req.payload = ThriftRequestResponsePayload{};
  (void)pipeline->fireRead(cp::erase_and_box(std::move(req)));
  EXPECT_EQ(tail_.readCount(), 1);

  // Unary response passes through the mux to the wire.
  ThriftServerResponseMessage resp;
  resp.payload = ThriftInitialResponsePayload{.streamId = 2};
  (void)pipeline->fireWrite(cp::erase_and_box(std::move(resp)));
  ASSERT_EQ(recorded_.size(), 1u);
  EXPECT_EQ(recorded_[0].kind, Recorded::Kind::Other);
}

} // namespace apache::thrift::fast_thrift::thrift
