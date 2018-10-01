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

#include <thrift/lib/cpp2/transport/rocket/framing/Frames.h>

#include <chrono>
#include <type_traits>
#include <utility>

#include <glog/logging.h>

#include <folly/CPortability.h>
#include <folly/Format.h>
#include <folly/Likely.h>
#include <folly/Range.h>
#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/FrameType.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Serializer.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Util.h>

namespace apache {
namespace thrift {
namespace rocket {

namespace {
// All frame sizes (header size + payload size) are encoded in 3 bytes
constexpr size_t kMaxFragmentedPayloadSize = 0xffffff - 512;

template <class Frame>
void readPayloadCommon(
    Frame& frame,
    bool expectingMetadata,
    folly::io::Cursor& cursor) {
  size_t metadataSize = 0;

  if (expectingMetadata) {
    metadataSize = readFrameOrMetadataSize(cursor);
    if (metadataSize > 0) {
      auto& metadata = frame.payload().metadata();
      metadata = folly::IOBuf::createCombined(metadataSize);
      cursor.clone(*metadata, metadataSize);
    }
    metadataSize += Serializer::kBytesForFrameOrMetadataLength;
  }

  // Finally, fix up payload data.
  frame.payload().data()->trimStart(frame.frameHeaderSize() + metadataSize);
}

template <class Frame>
void serializeInFragmentsSlowCommon(
    Frame&& frame,
    Flags flags,
    Serializer& writer) {
  folly::IOBufQueue metadataQueue(folly::IOBufQueue::cacheChainLength());
  folly::IOBufQueue dataQueue(folly::IOBufQueue::cacheChainLength());

  // The nonempty check here is not strictly necessary, since writePayload()
  // will always check nonemptiness of metadata before serializing.
  if (frame.payload().hasNonemptyMetadata()) {
    metadataQueue.append(std::move(frame.payload()).metadata());
  }
  dataQueue.append(std::move(frame.payload()).data());

  bool isFirstFrame = true;
  bool finished = false;
  while (!finished) {
    size_t bytesLeft = kMaxFragmentedPayloadSize;
    auto md = metadataQueue.splitAtMost(bytesLeft);
    bytesLeft -= md->computeChainDataLength();
    auto d = dataQueue.splitAtMost(bytesLeft);

    finished = metadataQueue.empty() && dataQueue.empty();
    auto p = Payload::makeFromMetadataAndData(std::move(md), std::move(d));
    DCHECK_LE(p.metadataAndDataSize(), kMaxFragmentedPayloadSize);
    if (std::exchange(isFirstFrame, false)) {
      frame.payload() = std::move(p);
      frame.setHasFollows(!finished);
      std::move(frame).serialize(writer);
    } else {
      PayloadFrame pf(frame.streamId(), std::move(p), flags.follows(!finished));
      std::move(pf).serialize(writer);
    }
  }
}
} // namespace

void SetupFrame::serialize(Serializer& writer) const {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                         Stream ID = 0                         |
   * +-----------+-+-+-+-+-----------+-------------------------------+
   * |Frame Type |0|M|R|L|  Flags    |
   * +-----------+-+-+-+-+-----------+-------------------------------+
   * |         Major Version         |        Minor Version          |
   * +-------------------------------+-------------------------------+
   * |0|                 Time Between KEEPALIVE Frames               |
   * +---------------------------------------------------------------+
   * |0|                       Max Lifetime                          |
   * +---------------------------------------------------------------+
   * |         Token Length          | Resume Identification Token  ...
   * +---------------+-----------------------------------------------+
   * |  MIME Length  |   Metadata Encoding MIME Type                ...
   * +---------------+-----------------------------------------------+
   * |  MIME Length  |     Data Encoding MIME Type                  ...
   * +---------------+-----------------------------------------------+
   *                       Metadata & Setup Payload
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  // SETUP frames are only sent when connections are first established, so we
  // forgo potential batching optimizations.
  nwritten += writer.write(StreamId{0});

  // The metadata flag can get out of sync with whether the payload actually has
  // metadata. In order to work around this and any other potential
  // inconsistencies, we recompute all flags during serialization as opposed to
  // serializing flags_ directly. This comment applies to all frames.
  nwritten += writer.writeFrameTypeAndFlags(
      frameType(),
      Flags::none()
          .metadata(payload_.hasNonemptyMetadata())
          .resumeToken(hasResumeIdentificationToken()));

  // Major and minor version. Our rsocket implementation only handles rsocket
  // protocol version 1.0.
  nwritten += writer.writeBE<uint16_t>(1); // rsocket major version
  nwritten += writer.writeBE<uint16_t>(0); // rsocket minor version

  // Time between KEEPALVE frames. Note that KEEPALIVE frames are unsupported.
  // Note: keepalive time MUST be > 0 and at most 2^31 - 1.
  constexpr std::chrono::milliseconds kMaxKeepaliveTime{(1ull << 31) - 1};
  nwritten += writer.writeBE<uint32_t>(kMaxKeepaliveTime.count());

  // Max lifetime. Protocol specifies that max lifetime MUST be > 0, but not
  // used in this implementation.
  constexpr std::chrono::milliseconds kMaxLifetime{(1ull << 31) - 1};
  nwritten += writer.writeBE<uint32_t>(kMaxLifetime.count());

  // Resume identification token length and token are not present if 'R' flag is
  // not present.
  if (hasResumeIdentificationToken()) {
    // Resume identification token length and token
    nwritten += writer.writeBE<uint16_t>(resumeIdentificationToken_.size());
    nwritten += writer.write(resumeIdentificationToken_);
  }

  // Length of metadata MIME type
  nwritten += writer.writeBE<uint8_t>(static_cast<uint8_t>(kMimeType.size()));
  nwritten += writer.write(kMimeType);

  // Length of data MIME type
  nwritten += writer.writeBE<uint8_t>(static_cast<uint8_t>(kMimeType.size()));
  nwritten += writer.write(kMimeType);

  // Setup metadata and data
  nwritten += writer.writePayload(payload());

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

void RequestResponseFrame::serialize(Serializer& writer) && {
  if (UNLIKELY(payload().metadataAndDataSize() > kMaxFragmentedPayloadSize)) {
    return std::move(*this).serializeInFragmentsSlow(writer);
  }
  serializeIntoSingleFrame(writer);
}

void RequestResponseFrame::serializeIntoSingleFrame(Serializer& writer) const {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+-+-------------+-------------------------------+
   * |Frame Type |0|M|F|     Flags   |
   * +-------------------------------+
   *                      Metadata & Request Data
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(
      frameType(),
      Flags::none()
          .metadata(payload_.hasNonemptyMetadata())
          .follows(hasFollows()));
  nwritten += writer.writePayload(payload());

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

void RequestFnfFrame::serialize(Serializer& writer) && {
  if (UNLIKELY(payload().metadataAndDataSize() > kMaxFragmentedPayloadSize)) {
    return std::move(*this).serializeInFragmentsSlow(writer);
  }
  serializeIntoSingleFrame(writer);
}

void RequestFnfFrame::serializeIntoSingleFrame(Serializer& writer) const {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+-+-------------+-------------------------------+
   * |Frame Type |0|M|F|    Flags    |
   * +-------------------------------+
   *                      Metadata & Request Data
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(
      frameType(),
      Flags::none()
          .metadata(payload_.hasNonemptyMetadata())
          .follows(hasFollows()));
  nwritten += writer.writePayload(payload());

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

void RequestStreamFrame::serialize(Serializer& writer) && {
  if (UNLIKELY(payload().metadataAndDataSize() > kMaxFragmentedPayloadSize)) {
    return std::move(*this).serializeInFragmentsSlow(writer);
  }
  serializeIntoSingleFrame(writer);
}

void RequestStreamFrame::serializeIntoSingleFrame(Serializer& writer) const {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+-+-------------+-------------------------------+
   * |Frame Type |0|M|F|    Flags    |
   * +-------------------------------+-------------------------------+
   * |0|                    Initial Request N                        |
   * +---------------------------------------------------------------+
   *                       Metadata & Request Data
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(
      frameType(),
      Flags::none()
          .metadata(payload_.hasNonemptyMetadata())
          .follows(hasFollows()));
  nwritten += writer.writeBE<uint32_t>(initialRequestN());
  nwritten += writer.writePayload(payload());

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

void RequestNFrame::serialize(Serializer& writer) const {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+---------------+-------------------------------+
   * |Frame Type |0|0|     Flags     |
   * +-------------------------------+-------------------------------+
   * |0|                         Request N                           |
   * +---------------------------------------------------------------+
   */

  // Excludes room for frame length
  constexpr auto frameSize = frameHeaderSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(frameType(), Flags::none());
  nwritten += writer.writeBE<uint32_t>(requestN());

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

void CancelFrame::serialize(Serializer& writer) const {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+---------------+-------------------------------+
   * |Frame Type |0|0|    Flags      |
   * +-------------------------------+-------------------------------+
   */

  // Excludes room for frame length
  constexpr auto frameSize = frameHeaderSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(frameType(), Flags::none());

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

void PayloadFrame::serialize(Serializer& writer) && {
  if (UNLIKELY(payload().metadataAndDataSize() > kMaxFragmentedPayloadSize)) {
    return std::move(*this).serializeInFragmentsSlow(writer);
  }
  serializeIntoSingleFrame(writer);
}

void PayloadFrame::serializeIntoSingleFrame(Serializer& writer) const {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+-+-+-+---------+-------------------------------+
   * |Frame Type |0|M|F|C|N|  Flags  |
   * +-------------------------------+-------------------------------+
   *                    Metadata & Data
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());

  // Protocol states that complete or next (or both) MUST be set... but this
  // appears to not hold in the case of Payload fragment frames.
  // DCHECK(hasComplete() || hasNext());
  nwritten += writer.writeFrameTypeAndFlags(
      frameType(),
      Flags::none()
          .metadata(payload_.hasNonemptyMetadata())
          .follows(hasFollows())
          .complete(hasComplete())
          .next(hasNext()));

  nwritten += writer.writePayload(payload());

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

void ErrorFrame::serialize(Serializer& writer) const {
  /**
   *  0                   1                   2                   3
   *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   * |                           Stream ID                           |
   * +-----------+-+-+---------------+-------------------------------+
   * |Frame Type |0|0|      Flags    |
   * +-----------+-+-+---------------+-------------------------------+
   * |                          Error Code                           |
   * +---------------------------------------------------------------+
   *                            Error Payload
   */

  // Excludes room for frame length
  const auto frameSize = frameHeaderSize() + payload().serializedSize();
  auto nwritten = writer.writeFrameOrMetadataSize(frameSize);

  nwritten += writer.write(streamId());
  nwritten += writer.writeFrameTypeAndFlags(frameType(), Flags::none());

  using ErrorCodeInt = std::underlying_type_t<ErrorCode>;
  nwritten +=
      writer.writeBE<ErrorCodeInt>(static_cast<ErrorCodeInt>(errorCode()));

  nwritten += writer.writePayload(payload());

  DCHECK_EQ(Serializer::kBytesForFrameOrMetadataLength + frameSize, nwritten);
}

FOLLY_NOINLINE void RequestResponseFrame::serializeInFragmentsSlow(
    Serializer& writer) && {
  serializeInFragmentsSlowCommon(std::move(*this), Flags::none(), writer);
}

FOLLY_NOINLINE void RequestFnfFrame::serializeInFragmentsSlow(
    Serializer& writer) && {
  serializeInFragmentsSlowCommon(std::move(*this), Flags::none(), writer);
}

FOLLY_NOINLINE void RequestStreamFrame::serializeInFragmentsSlow(
    Serializer& writer) && {
  serializeInFragmentsSlowCommon(std::move(*this), Flags::none(), writer);
}

FOLLY_NOINLINE void PayloadFrame::serializeInFragmentsSlow(
    Serializer& writer) && {
  serializeInFragmentsSlowCommon(
      std::move(*this),
      Flags::none().complete(hasComplete()).next(hasNext()),
      writer);
}

SetupFrame::SetupFrame(std::unique_ptr<folly::IOBuf> _frame)
    : payload_(Payload::makeFromData(std::move(_frame))) {
  // Trick to avoid the default-constructed IOBuf. See expanded comment in
  // PayloadFrame constructor. Do this optimization in Setup frame for
  // consistency, not performance.
  auto* frame = payload_.data();
  DCHECK(!frame->isChained());

  folly::io::Cursor cursor(frame);
  const StreamId zero(readStreamId(cursor));
  DCHECK_EQ(StreamId{0}, zero);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);

  const auto majorVersion = cursor.readBE<uint16_t>();
  const auto minorVersion = cursor.readBE<uint16_t>();

  if (majorVersion != 1 || minorVersion != 0) {
    throw std::runtime_error(folly::sformat(
        "SETUP frame received with unsupported version {}.{}",
        majorVersion,
        minorVersion));
  }

  // Skip keep-alive interval (4 bytes) and max lifetime (4 bytes). These values
  // are not currently used in Thrift.
  cursor.skip(8);

  // Resumption is not currently supported, but we handle the resume
  // identification token properly in case remote end sends a token.
  if (hasResumeIdentificationToken()) {
    const auto tokenLength = cursor.readBE<uint16_t>();
    cursor.skip(tokenLength);
  }

  // MIME types are currently not used, but we still handle the bytes properly.
  const auto metadataMimeLength = cursor.read<uint8_t>();
  cursor.skip(metadataMimeLength);
  const auto dataMimeLength = cursor.read<uint8_t>();
  cursor.skip(dataMimeLength);

  readPayloadCommon(*this, flags_.metadata(), cursor);
}

RequestResponseFrame::RequestResponseFrame(std::unique_ptr<folly::IOBuf> _frame)
    : payload_(Payload::makeFromData(std::move(_frame))) {
  // Trick to avoid the default-constructed IOBuf. See expanded comment in
  // PayloadFrame constructor.
  auto* frame = payload_.data();
  folly::io::Cursor cursor(frame);
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);

  readPayloadCommon(*this, flags_.metadata(), cursor);
}

RequestFnfFrame::RequestFnfFrame(std::unique_ptr<folly::IOBuf> _frame)
    : payload_(Payload::makeFromData(std::move(_frame))) {
  // Trick to avoid the default-constructed IOBuf. See expanded comment in
  // PayloadFrame constructor.
  auto* frame = payload_.data();
  folly::io::Cursor cursor(frame);
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);

  readPayloadCommon(*this, flags_.metadata(), cursor);
}

RequestStreamFrame::RequestStreamFrame(std::unique_ptr<folly::IOBuf> _frame)
    : payload_(Payload::makeFromData(std::move(_frame))) {
  // Trick to avoid the default-constructed IOBuf. See expanded comment in
  // PayloadFrame constructor.
  auto* frame = payload_.data();
  folly::io::Cursor cursor(frame);
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);

  initialRequestN_ = cursor.readBE<int32_t>();

  readPayloadCommon(*this, flags_.metadata(), cursor);
}

RequestNFrame::RequestNFrame(std::unique_ptr<folly::IOBuf> frame) {
  folly::io::Cursor cursor(frame.get());
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  // RequestN frame has no flags, but we need to skip over the two bytes.
  FrameType type;
  Flags flags;
  std::tie(type, flags) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);
  DCHECK(Flags::none() == flags);

  requestN_ = cursor.readBE<int32_t>();
}

CancelFrame::CancelFrame(std::unique_ptr<folly::IOBuf> frame) {
  folly::io::Cursor cursor(frame.get());
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  // Cancel frame has no flags.
  FrameType type;
  Flags flags;
  std::tie(type, flags) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);
  DCHECK(Flags::none() == flags);
}

PayloadFrame::PayloadFrame(std::unique_ptr<folly::IOBuf> _frame)
    : payload_(Payload::makeFromData(std::move(_frame))) {
  // Trick to avoid the default-constructed IOBuf. Initially, we stash the
  // entire frame in payload_.data. Since this IOBuf covers more than the data
  // range, we need to fix up payload_.data later on.
  auto* frame = payload_.data();
  folly::io::Cursor cursor(frame);
  DCHECK(!frame->isChained());

  streamId_ = readStreamId(cursor);

  FrameType type;
  std::tie(type, flags_) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);

  readPayloadCommon(*this, flags_.metadata(), cursor);
}

ErrorFrame::ErrorFrame(std::unique_ptr<folly::IOBuf> _frame)
    : payload_(Payload::makeFromData(std::move(_frame))) {
  auto* frame = payload_.data();
  folly::io::Cursor cursor(frame);
  DCHECK(!frame->isChained());
  DCHECK_GE(frame->length(), frameHeaderSize());

  streamId_ = readStreamId(cursor);

  // Error frame has no flags, but we still need to skip the two bytes.
  FrameType type;
  Flags flags;
  std::tie(type, flags) = readFrameTypeAndFlags(cursor);
  DCHECK(frameType() == type);
  DCHECK(Flags::none() == flags);

  errorCode_ = static_cast<ErrorCode>(cursor.readBE<uint32_t>());

  // Finally, adjust error payload data() pointer as needed, as in PayloadFrame
  // constructor.
  frame->trimStart(frameHeaderSize());
}

// Static member definition
constexpr folly::StringPiece SetupFrame::kMimeType;

} // namespace rocket
} // namespace thrift
} // namespace apache
