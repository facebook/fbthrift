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

// Every frame parser does the same job. You give it bytes, it gives back whole
// frames, in order, with the same content. These tests check that job. They are
// written once and run against every parser.
//
// The tests look only at what the caller sees: the frames in the sink and the
// returned Result. Buffer handling, internal state and error reporting differ
// in each parser, so they are tested next to that parser.
//
// The gtest machinery here is less common than plain TEST_F. The tests are
// written against a placeholder type. Each INSTANTIATE_TYPED_TEST_SUITE_P at
// the bottom makes a copy of all of them for one concrete type. TYPED_TEST_P
// declares a test, REGISTER_TYPED_TEST_SUITE_P lists the tests to copy, and all
// three must be in the same file. Inside a test the fixture is a dependent base
// class, so its members need `this->`.

#include <algorithm>
#include <cstdint>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <folly/Range.h>
#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/Common.h>
#include <thrift/lib/cpp2/fast_thrift/frame/FrameType.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/AlignedParser.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/FrameLengthParser.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/test/ParserContractFixture.h>
#include <thrift/lib/cpp2/fast_thrift/frame/read/test/WireFrames.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/FrameHeaders.h>
#include <thrift/lib/cpp2/fast_thrift/frame/write/FrameLength.h>

namespace apache::thrift::fast_thrift::frame::read {

using namespace testing;

using channel_pipeline::Result;

TYPED_TEST_SUITE_P(ParserContractTest);

TYPED_TEST_P(ParserContractTest, SingleCompleteFrame) {
  using Traits = typename TestFixture::Traits;

  EXPECT_EQ(this->feed(Traits::makeFrame(20, 'a')), Result::Success);
  ASSERT_EQ(this->frames_.size(), 1);
  EXPECT_EQ(
      this->frames_[0]->computeChainDataLength(), Traits::emittedSize(20));
  TestFixture::expectPayload(*this->frames_[0], 20, 'a');
}

TYPED_TEST_P(ParserContractTest, PartialHeaderEmitsNothing) {
  using Traits = typename TestFixture::Traits;

  // With a one-byte header there is nothing to feed here, and the test would
  // pass without checking anything.
  ASSERT_GT(Traits::headerSize(), 1);

  auto bytes = Traits::makeFrame(20, 'a');
  bytes.resize(Traits::headerSize() - 1);

  EXPECT_EQ(this->feed(bytes), Result::Success);
  EXPECT_EQ(this->frames_.size(), 0);
}

TYPED_TEST_P(ParserContractTest, HeaderThenBody) {
  using Traits = typename TestFixture::Traits;

  const auto bytes = Traits::makeFrame(20, 'a');
  const auto header = Traits::headerSize();

  EXPECT_EQ(
      this->feed(folly::ByteRange{bytes.data(), header}), Result::Success);
  EXPECT_EQ(this->frames_.size(), 0);

  EXPECT_EQ(
      this->feed(
          folly::ByteRange{bytes.data() + header, bytes.size() - header}),
      Result::Success);
  ASSERT_EQ(this->frames_.size(), 1);
  EXPECT_EQ(
      this->frames_[0]->computeChainDataLength(), Traits::emittedSize(20));
  TestFixture::expectPayload(*this->frames_[0], 20, 'a');
}

TYPED_TEST_P(ParserContractTest, MultipleFramesInOneStream) {
  using Traits = typename TestFixture::Traits;

  EXPECT_EQ(this->feed(TestFixture::concatFrames(3, 20)), Result::Success);
  ASSERT_EQ(this->frames_.size(), 3);
  for (size_t i = 0; i < this->frames_.size(); ++i) {
    EXPECT_EQ(
        this->frames_[i]->computeChainDataLength(), Traits::emittedSize(20));
    TestFixture::expectPayload(*this->frames_[i], 20, TestFixture::fillFor(i));
  }
}

TYPED_TEST_P(ParserContractTest, MultipleFramesSeparately) {
  using Traits = typename TestFixture::Traits;

  for (size_t i = 0; i < 3; ++i) {
    EXPECT_EQ(
        this->feed(Traits::makeFrame(20, TestFixture::fillFor(i))),
        Result::Success);
    ASSERT_EQ(this->frames_.size(), i + 1);
    EXPECT_EQ(
        this->frames_[i]->computeChainDataLength(), Traits::emittedSize(20));
    TestFixture::expectPayload(*this->frames_[i], 20, TestFixture::fillFor(i));
  }
}

TYPED_TEST_P(ParserContractTest, LargeFrame) {
  using Traits = typename TestFixture::Traits;
  constexpr size_t kPayload = 65536;

  EXPECT_EQ(this->feed(Traits::makeFrame(kPayload, 'a')), Result::Success);
  ASSERT_EQ(this->frames_.size(), 1);
  EXPECT_EQ(
      this->frames_[0]->computeChainDataLength(),
      Traits::emittedSize(kPayload));
  TestFixture::expectPayload(*this->frames_[0], kPayload, 'a');
}

TYPED_TEST_P(ParserContractTest, LargeFrameInChunks) {
  using Traits = typename TestFixture::Traits;
  constexpr size_t kPayload = 65536;
  constexpr size_t kChunk = 4096;

  const auto bytes = Traits::makeFrame(kPayload, 'a');
  for (size_t offset = 0; offset < bytes.size(); offset += kChunk) {
    const auto len = std::min(kChunk, bytes.size() - offset);
    EXPECT_EQ(
        this->feed(folly::ByteRange{bytes.data() + offset, len}),
        Result::Success);
  }

  ASSERT_EQ(this->frames_.size(), 1);
  EXPECT_EQ(
      this->frames_[0]->computeChainDataLength(),
      Traits::emittedSize(kPayload));
  TestFixture::expectPayload(*this->frames_[0], kPayload, 'a');
}

TYPED_TEST_P(ParserContractTest, BackpressureStopsAfterFirstFrame) {
  using Traits = typename TestFixture::Traits;
  this->sinkResult_ = Result::Backpressure;

  EXPECT_EQ(this->feed(TestFixture::concatFrames(3, 20)), Result::Backpressure);
  // Backpressure means "accepted, but slow down". The first frame arrived, and
  // its fill byte proves it is the first one and not a later one.
  ASSERT_EQ(this->frames_.size(), 1);
  EXPECT_EQ(
      this->frames_[0]->computeChainDataLength(), Traits::emittedSize(20));
  TestFixture::expectPayload(*this->frames_[0], 20, TestFixture::fillFor(0));
}

TYPED_TEST_P(ParserContractTest, ErrorStopsProcessing) {
  using Traits = typename TestFixture::Traits;
  this->sinkResult_ = Result::Error;

  // Three frames, so that ignoring the refusal shows up as a second frame.
  EXPECT_EQ(this->feed(TestFixture::concatFrames(3, 20)), Result::Error);
  // The frame was handed over before the sink refused it.
  ASSERT_EQ(this->frames_.size(), 1);
  EXPECT_EQ(
      this->frames_[0]->computeChainDataLength(), Traits::emittedSize(20));
  TestFixture::expectPayload(*this->frames_[0], 20, TestFixture::fillFor(0));
}

TYPED_TEST_P(ParserContractTest, ResetDropsPartialFrame) {
  using Traits = typename TestFixture::Traits;

  auto partial = Traits::makeFrame(20, 'a');
  partial.resize(Traits::headerSize());
  EXPECT_EQ(this->feed(partial), Result::Success);
  EXPECT_EQ(this->frames_.size(), 0);

  this->parser_.reset();

  EXPECT_EQ(this->feed(Traits::makeFrame(30, 'b')), Result::Success);
  ASSERT_EQ(this->frames_.size(), 1);
  EXPECT_EQ(
      this->frames_[0]->computeChainDataLength(), Traits::emittedSize(30));
  TestFixture::expectPayload(*this->frames_[0], 30, 'b');
}

REGISTER_TYPED_TEST_SUITE_P(
    ParserContractTest,
    SingleCompleteFrame,
    PartialHeaderEmitsNothing,
    HeaderThenBody,
    MultipleFramesInOneStream,
    MultipleFramesSeparately,
    LargeFrame,
    LargeFrameInChunks,
    BackpressureStopsAfterFirstFrame,
    ErrorStopsProcessing,
    ResetDropsPartialFrame);

namespace {

// FrameLengthParser's wire format: a three-byte big-endian length, then that
// many payload bytes. Templated on the driver so both entry points reuse it.
template <typename DriverT>
struct FrameLengthParserTraits {
  using Parser = FrameLengthParser;
  using Driver = DriverT;

  static size_t headerSize() { return kMetadataLengthSize; }

  // The length prefix is stripped, so only the payload reaches the sink.
  static size_t emittedSize(size_t payloadSize) { return payloadSize; }

  static std::vector<uint8_t> makeFrame(size_t payloadSize, uint8_t fill) {
    std::vector<uint8_t> bytes(kMetadataLengthSize + payloadSize, fill);
    write::writeFrameLength(bytes.data(), payloadSize);
    return bytes;
  }
};

// AlignedParser reads an RSocket frame header, then the payload. It has no
// consumeBuffer, so it runs with one driver only. The header type is a
// parameter so that the suite runs twice. One run sends a request, which gets
// buffers of its own. The other sends a frame type that does not.
template <typename HeaderT>
struct AlignedParserTraits {
  using Parser = AlignedParser;
  using Driver = ConsumeDriver;

  static size_t headerSize() { return kMetadataLengthSize + kBaseHeaderSize; }

  // The parser removes only the 3 length bytes. The rest of the header stays
  // in the frame it emits.
  static size_t emittedSize(size_t payloadSize) {
    return kBaseHeaderSize + payloadSize;
  }

  static std::vector<uint8_t> makeFrame(size_t payloadSize, uint8_t fill) {
    return serializeFrame(
        HeaderT{.streamId = 1},
        nullptr,
        folly::IOBuf::copyBuffer(std::string(payloadSize, fill)));
  }
};

// One entry per run of the suite. Types is gtest's type list. One element each
// is enough, because each instantiation below has its own name prefix.
using ViaConsume = Types<FrameLengthParserTraits<ConsumeDriver>>;
using ViaConsumeBuffer = Types<FrameLengthParserTraits<ConsumeBufferDriver>>;
using ViaConsumeBufferChain =
    Types<FrameLengthParserTraits<ConsumeBufferChainDriver>>;
using ViaAligned = Types<AlignedParserTraits<write::RequestResponseHeader>>;
using ViaAlignedPlain = Types<AlignedParserTraits<write::RequestFnfHeader>>;

} // namespace

// Copies the whole suite under the given prefix. The cases above then run as
// Consume/ParserContractTest/0.SingleCompleteFrame and once more per prefix
// below. FrameLengthParser works with every driver, so it gets all three.
// AlignedParser has only consume, so it gets one driver and two frame types.
INSTANTIATE_TYPED_TEST_SUITE_P(Consume, ParserContractTest, ViaConsume);
INSTANTIATE_TYPED_TEST_SUITE_P(
    ConsumeBuffer, ParserContractTest, ViaConsumeBuffer);
INSTANTIATE_TYPED_TEST_SUITE_P(
    ConsumeBufferChain, ParserContractTest, ViaConsumeBufferChain);
// The two Aligned prefixes carry DISABLED_ because the parser is still an
// empty skeleton. It has to sit on the prefix rather than on the tests,
// because the test bodies are shared with FrameLengthParser, which passes
// them. Drop it when the parser gets written.
INSTANTIATE_TYPED_TEST_SUITE_P(
    DISABLED_Aligned, ParserContractTest, ViaAligned);
INSTANTIATE_TYPED_TEST_SUITE_P(
    DISABLED_AlignedPlain, ParserContractTest, ViaAlignedPlain);

} // namespace apache::thrift::fast_thrift::frame::read
