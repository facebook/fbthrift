/*
 * Copyright 2018-present Facebook, Inc.
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

#include <algorithm>
#include <utility>

#include <gtest/gtest.h>

#include <folly/Optional.h>
#include <folly/Range.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/transport/rocket/RocketException.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/ErrorCode.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Serializer.h>

using namespace apache::thrift::rocket;

namespace {
constexpr StreamId kTestStreamId{178};
constexpr folly::StringPiece kMetadata{"metadata"};
constexpr folly::StringPiece kData{"data"};

folly::StringPiece getRange(const folly::IOBuf& buf) {
  EXPECT_FALSE(buf.isChained());
  return folly::StringPiece{reinterpret_cast<const char*>(buf.data()),
                            buf.length()};
}

template <class Frame>
Frame serializeAndDeserialize(Frame&& frame) {
  Serializer writer;
  std::forward<Frame>(frame).serialize(writer);
  auto serializedFrameData = std::move(writer).move();
  serializedFrameData->coalesce();
  // Skip past frame length
  serializedFrameData->trimStart(Serializer::kBytesForFrameOrMetadataLength);
  return Frame(std::move(serializedFrameData));
}

template <class Frame>
Frame serializeAndDeserializeFragmented(Frame&& frame) {
  Serializer writer;
  std::forward<Frame>(frame).serialize(writer);
  auto serializedFrameData = std::move(writer).move();
  serializedFrameData->coalesce();

  folly::Optional<Frame> returnFrame;
  folly::io::Cursor cursor(serializedFrameData.get());
  bool hitSplitLogic = false;
  while (!cursor.isAtEnd()) {
    const auto currentFrameSize = readFrameOrMetadataSize(cursor);
    auto frameBuf = folly::IOBuf::createCombined(currentFrameSize);
    cursor.clone(*frameBuf, currentFrameSize);
    if (!returnFrame) {
      returnFrame.emplace(Frame(std::move(frameBuf)));
    } else {
      hitSplitLogic = true;
      PayloadFrame pf(std::move(frameBuf));
      returnFrame->payload().append(std::move(pf.payload()));
      if (!pf.hasFollows()) {
        break;
      }
    }
  }
  EXPECT_TRUE(returnFrame);
  EXPECT_TRUE(hitSplitLogic);
  return std::move(*returnFrame);
}

folly::IOBuf makeLargeIOBuf() {
  // Ensure IOBuf will be serialized across multiple fragment frames
  constexpr size_t kLargeIOBufSize = 0x2ffffff;

  folly::IOBuf buf(folly::IOBuf::CreateOp(), kLargeIOBufSize);
  EXPECT_GT(buf.tailroom(), kLargeIOBufSize);
  auto* const p = buf.writableTail();
  size_t i = 0;
  // Fill buffer with some non-trivial data
  std::for_each(p, p + kLargeIOBufSize, [&i](auto& c) {
    constexpr size_t kAPrime = 251;
    c = i++ % kAPrime;
  });
  buf.append(kLargeIOBufSize);
  return buf;
}
} // namespace

namespace apache {
namespace thrift {
namespace rocket {

TEST(FrameSerialization, SetupSanity) {
  SetupFrame frame(Payload::makeFromMetadataAndData(kMetadata, kData));

  auto validate = [](const SetupFrame& f) {
    // Resumption and lease flags are not currently supported
    EXPECT_FALSE(f.hasResumeIdentificationToken());
    EXPECT_FALSE(f.hasLease());
    EXPECT_TRUE(f.payload().metadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_EQ(kData, getRange(*f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserialize(std::move(frame)));
}

TEST(FrameSerialization, RequestResponseSanity) {
  RequestResponseFrame frame(
      kTestStreamId, Payload::makeFromMetadataAndData(kMetadata, kData));

  auto validate = [](const RequestResponseFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_FALSE(f.hasFollows());
    EXPECT_TRUE(f.payload().metadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_EQ(kData, getRange(*f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserialize(std::move(frame)));
}

TEST(FrameSerialization, RequestFnfSanity) {
  RequestFnfFrame frame(
      kTestStreamId, Payload::makeFromMetadataAndData(kMetadata, kData));

  auto validate = [](const RequestFnfFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_FALSE(f.hasFollows());
    EXPECT_TRUE(f.payload().metadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_EQ(kData, getRange(*f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserialize(std::move(frame)));
}

TEST(FrameSerialization, RequestStreamSanity) {
  RequestStreamFrame frame(
      kTestStreamId,
      Payload::makeFromMetadataAndData(kMetadata, kData),
      123456789 /* initialRequestN */);

  auto validate = [](const RequestStreamFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_FALSE(f.hasFollows());
    EXPECT_EQ(123456789, f.initialRequestN());
    EXPECT_TRUE(f.payload().metadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_EQ(kData, getRange(*f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserialize(std::move(frame)));
}

TEST(FrameSerialization, RequestNSanity) {
  RequestNFrame frame(kTestStreamId, std::numeric_limits<int32_t>::max());

  auto validate = [](const RequestNFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_EQ(std::numeric_limits<int32_t>::max(), f.requestN());
  };

  validate(frame);
  validate(serializeAndDeserialize(std::move(frame)));
}

TEST(FrameSerialization, CancelSanity) {
  CancelFrame frame(kTestStreamId);

  auto validate = [](const CancelFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
  };

  validate(frame);
  validate(serializeAndDeserialize(std::move(frame)));
}

TEST(FrameSerialization, PayloadSanity) {
  PayloadFrame frame(
      kTestStreamId,
      Payload::makeFromMetadataAndData(kMetadata, kData),
      Flags::none().follows(true).complete(true).next(true));

  auto validate = [](const PayloadFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_TRUE(f.hasFollows());
    EXPECT_TRUE(f.hasComplete());
    EXPECT_TRUE(f.hasNext());
    EXPECT_TRUE(f.payload().metadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_EQ(kData, getRange(*f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserialize(std::move(frame)));
}

TEST(FrameSerialization, PayloadEmptyMetadataSanity) {
  auto validate = [](const PayloadFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_TRUE(f.hasFollows());
    EXPECT_TRUE(f.hasComplete());
    EXPECT_TRUE(f.hasNext());
    EXPECT_FALSE(f.payload().hasNonemptyMetadata());
    EXPECT_EQ(kData, getRange(*f.payload().data()));
  };

  // No metadata
  {
    PayloadFrame frame(
        kTestStreamId,
        Payload::makeFromData(kData),
        Flags::none().follows(true).complete(true).next(true));

    validate(frame);
    validate(serializeAndDeserialize(std::move(frame)));
  }

  // Empty metadata
  {
    PayloadFrame frame(
        kTestStreamId,
        Payload::makeFromMetadataAndData(
            folly::ByteRange{folly::StringPiece{""}}, kData),
        Flags::none().follows(true).complete(true).next(true));

    validate(frame);
    validate(serializeAndDeserialize(std::move(frame)));
  }
}

TEST(FrameSerialization, ErrorSanity) {
  constexpr folly::StringPiece kErrorMessage{"error_message"};

  ErrorFrame frame(
      kTestStreamId, ErrorCode::CANCELED, Payload::makeFromData(kErrorMessage));

  auto validate = [=](const ErrorFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_EQ(ErrorCode::CANCELED, f.errorCode());
    EXPECT_FALSE(f.payload().metadata());
    EXPECT_EQ(kErrorMessage, getRange(*f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserialize(std::move(frame)));
}

TEST(FrameSerialization, ErrorFromException) {
  constexpr folly::StringPiece kErrorMessage{"error_message"};

  RocketException rex(
      ErrorCode::CANCELED, folly::IOBuf::copyBuffer(kErrorMessage));
  ErrorFrame frame(kTestStreamId, std::move(rex));

  auto validate = [=](const ErrorFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_EQ(ErrorCode::CANCELED, f.errorCode());
    EXPECT_FALSE(f.payload().metadata());
    EXPECT_EQ(kErrorMessage, getRange(*f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserialize(std::move(frame)));
}

TEST(FrameSerialization, PayloadLargeMetadata) {
  const auto metadata = makeLargeIOBuf();
  constexpr folly::StringPiece kData{"data"};

  PayloadFrame frame(
      kTestStreamId,
      Payload::makeFromMetadataAndData(
          metadata.clone(), folly::IOBuf::copyBuffer(kData)),
      Flags::none().complete(true).next(true));

  auto validate = [=](const PayloadFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_TRUE(f.payload().hasNonemptyMetadata());
    EXPECT_TRUE(folly::IOBufEqualTo()(metadata, *f.payload().metadata()));
    EXPECT_EQ(kData, getRange(*f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserializeFragmented(std::move(frame)));
}

TEST(FrameSerialization, PayloadLargeData) {
  constexpr folly::StringPiece kMetadata{"metadata"};
  const auto data = makeLargeIOBuf();

  PayloadFrame frame(
      kTestStreamId,
      Payload::makeFromMetadataAndData(
          folly::IOBuf::copyBuffer(kMetadata), data.clone()),
      Flags::none().complete(true).next(true));

  auto validate = [=](const PayloadFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_TRUE(f.payload().hasNonemptyMetadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_TRUE(folly::IOBufEqualTo()(data, *f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserializeFragmented(std::move(frame)));
}

TEST(FrameSerialization, RequestResponseLargeMetadata) {
  constexpr folly::StringPiece kMetadata{"metadata"};
  const auto data = makeLargeIOBuf();

  RequestResponseFrame frame(
      kTestStreamId,
      Payload::makeFromMetadataAndData(
          folly::IOBuf::copyBuffer(kMetadata), data.clone()));

  auto validate = [=](const RequestResponseFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_TRUE(f.payload().hasNonemptyMetadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_TRUE(folly::IOBufEqualTo()(data, *f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserializeFragmented(std::move(frame)));
}

TEST(FrameSerialization, RequestResponseLargeData) {
  constexpr folly::StringPiece kMetadata{"metadata"};
  const auto data = makeLargeIOBuf();

  RequestResponseFrame frame(
      kTestStreamId,
      Payload::makeFromMetadataAndData(
          folly::IOBuf::copyBuffer(kMetadata), data.clone()));

  auto validate = [=](const RequestResponseFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_TRUE(f.payload().hasNonemptyMetadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_TRUE(folly::IOBufEqualTo()(data, *f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserializeFragmented(std::move(frame)));
}

TEST(FrameSerialization, RequestFnfLargeMetadata) {
  constexpr folly::StringPiece kMetadata{"metadata"};
  const auto data = makeLargeIOBuf();

  RequestFnfFrame frame(
      kTestStreamId,
      Payload::makeFromMetadataAndData(
          folly::IOBuf::copyBuffer(kMetadata), data.clone()));

  auto validate = [=](const RequestFnfFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_TRUE(f.payload().hasNonemptyMetadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_TRUE(folly::IOBufEqualTo()(data, *f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserializeFragmented(std::move(frame)));
}

TEST(FrameSerialization, RequestFnfLargeData) {
  constexpr folly::StringPiece kMetadata{"metadata"};
  const auto data = makeLargeIOBuf();

  RequestFnfFrame frame(
      kTestStreamId,
      Payload::makeFromMetadataAndData(
          folly::IOBuf::copyBuffer(kMetadata), data.clone()));

  auto validate = [=](const RequestFnfFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_TRUE(f.payload().hasNonemptyMetadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_TRUE(folly::IOBufEqualTo()(data, *f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserializeFragmented(std::move(frame)));
}

TEST(FrameSerialization, RequestStreamLargeMetadata) {
  constexpr folly::StringPiece kMetadata{"metadata"};
  const auto data = makeLargeIOBuf();

  RequestStreamFrame frame(
      kTestStreamId,
      Payload::makeFromMetadataAndData(
          folly::IOBuf::copyBuffer(kMetadata), data.clone()),
      123 /* initialRequestN */);

  auto validate = [=](const RequestStreamFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_TRUE(f.payload().hasNonemptyMetadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_TRUE(folly::IOBufEqualTo()(data, *f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserializeFragmented(std::move(frame)));
}

TEST(FrameSerialization, RequestStreamLargeData) {
  constexpr folly::StringPiece kMetadata{"metadata"};
  const auto data = makeLargeIOBuf();

  RequestStreamFrame frame(
      kTestStreamId,
      Payload::makeFromMetadataAndData(
          folly::IOBuf::copyBuffer(kMetadata), data.clone()),
      123 /* initialRequestN */);

  auto validate = [=](const RequestStreamFrame& f) {
    EXPECT_EQ(kTestStreamId, f.streamId());
    EXPECT_TRUE(f.payload().hasNonemptyMetadata());
    EXPECT_EQ(kMetadata, getRange(*f.payload().metadata()));
    EXPECT_TRUE(folly::IOBufEqualTo()(data, *f.payload().data()));
  };

  validate(frame);
  validate(serializeAndDeserializeFragmented(std::move(frame)));
}

} // namespace rocket
} // namespace thrift
} // namespace apache
