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

// Tests the runtime stream-open glue: makeStreamOpenMessage binds a
// hand-written StreamElementEncoder<int> (standing in for the codegen-supplied
// one) to an app-composed StreamFactory<int>, and the resulting
// ThriftServerStreamOpenPayload carries a ConfigFunc that, run through a
// ProducerPipeline, emits the encoded elements. A static_assert pins
// detail::writeStreamOpen to the FastHandlerCallback ResultFn shape codegen
// will target.

#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/StreamResponse.h>

#include <cstdint>
#include <cstring>
#include <memory>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/HandlerTag.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/detail/ContextImpl.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/util/FastHandlerCallback.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/ProducerPipeline.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/StreamElementEncoder.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/StreamFactory.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>

namespace apache::thrift::fast_thrift::thrift {

namespace cp = channel_pipeline;

namespace {

using Context = cp::detail::ContextImpl;

// Hand-written encoder: an int is serialized as its 4 raw bytes.
stream::Payload encodeIntValue(int32_t&& value) noexcept {
  auto buf = folly::IOBuf::create(sizeof(int32_t));
  std::memcpy(buf->writableData(), &value, sizeof(int32_t));
  buf->append(sizeof(int32_t));
  return stream::Payload{.data = std::move(buf)};
}

stream::Error encodeIntError(folly::exception_wrapper&& ex) noexcept {
  return stream::Error{.ex = std::move(ex)};
}

int32_t decodeInt(const folly::IOBuf& buf) {
  int32_t value = 0;
  std::memcpy(&value, buf.data(), sizeof(int32_t));
  return value;
}

HANDLER_TAG(producer);

// Application producer written in terms of int: on RequestN it emits up to `n`
// encoded payloads (bounded by its backlog) then Complete, using the
// framework-supplied encoder.
template <typename Ctx>
class IntProducer {
 public:
  IntProducer(
      stream::StreamElementEncoder<int32_t> encoder,
      std::vector<int32_t> values)
      : encoder_(encoder), values_(std::move(values)) {}

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
      for (uint64_t i = 0; i < n && next_ < values_.size(); ++i, ++next_) {
        (void)ctx.fireWrite(
            cp::erase_and_box(
                stream::ThriftStreamMessage{
                    .payload = encoder_.encodeValue(int32_t{values_[next_]})}));
      }
      if (next_ == values_.size() && !completed_) {
        completed_ = true;
        (void)ctx.fireWrite(
            cp::erase_and_box(
                stream::ThriftStreamMessage{.payload = stream::Complete{}}));
      }
      return cp::Result::Success;
    }
    if (m.payload.is<stream::Cancel>()) {
      return cp::Result::Success;
    }
    return ctx.fireRead(std::move(msg));
  }

 private:
  stream::StreamElementEncoder<int32_t> encoder_;
  std::vector<int32_t> values_;
  size_t next_{0};
  bool completed_{false};
};

stream::StreamFactory<int32_t> makeIntStreamFactory(
    std::vector<int32_t> values) {
  return stream::StreamFactory<int32_t>(
      [values = std::move(values)](
          stream::ProducerPipeline::Builder& builder,
          const stream::StreamElementEncoder<int32_t>& encoder) mutable {
        builder.addNextDuplex<IntProducer<Context>>(
            producer_tag,
            std::make_unique<IntProducer<Context>>(encoder, std::move(values)));
      });
}

// detail::writeStreamOpen must match FastHandlerCallback's ResultFn shape so
// codegen can hand it to makeFastHandlerCallback.
static_assert(std::is_same_v<
              FastHandlerCallback<stream::StreamFactory<int32_t>>::ResultFn,
              decltype(&detail::writeStreamOpen<
                       int32_t,
                       &encodeIntValue,
                       &encodeIntError>)>);

} // namespace

TEST(StreamResponseTest, MakeStreamOpenMessageBindsFactoryAndEncoder) {
  auto message = makeStreamOpenMessage<int32_t>(
      /*streamId=*/7,
      makeIntStreamFactory({10, 20, 30}),
      stream::StreamElementEncoder<int32_t>{&encodeIntValue, &encodeIntError});

  ASSERT_TRUE(message.payload.is<ThriftServerStreamOpenPayload>());
  auto& open = message.payload.get<ThriftServerStreamOpenPayload>();
  EXPECT_EQ(open.initialResponse.streamId, 7u);
  ASSERT_NE(open.configFunc, nullptr);

  // The bound ConfigFunc composes the producing-end sub-pipeline; injected
  // demand drives the producer to emit the encoded elements then Complete.
  folly::EventBase evb;
  std::vector<int32_t> got;
  bool completed = false;
  stream::ProducerPipeline::Builder builder(
      &evb,
      stream::StreamSink{
          [&](stream::ThriftStreamMessage&& m) noexcept -> cp::Result {
            if (m.payload.is<stream::Payload>()) {
              got.push_back(decodeInt(*m.payload.get<stream::Payload>().data));
            } else if (m.payload.is<stream::Complete>()) {
              completed = true;
            }
            return cp::Result::Success;
          }});
  (*open.configFunc)(builder);
  auto pipeline = builder.build();

  (void)pipeline.fireRead(
      cp::erase_and_box(
          stream::ThriftStreamMessage{.payload = stream::RequestN{.n = 10}}));

  EXPECT_EQ(got, (std::vector<int32_t>{10, 20, 30}));
  EXPECT_TRUE(completed);
}

} // namespace apache::thrift::fast_thrift::thrift
