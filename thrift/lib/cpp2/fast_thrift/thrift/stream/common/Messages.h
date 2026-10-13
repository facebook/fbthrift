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

#pragma once

#include <cstdint>
#include <memory>

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>
#include <thrift/lib/cpp2/fast_thrift/common/CompactVariant.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache::thrift::fast_thrift::thrift::stream {

// Messages for an established stream / sink / bidi exchange.
//
// The five frame kinds partition into two *sets* by which way they travel
// between the two stream roles:
//
//   - Control = {RequestN, Cancel} always flows consumer -> producer.
//   - Data    = {Payload, Complete, Error} always flows producer -> consumer.
//
// A role fixes each set to a direction; the side of the connection only selects
// which roles are instantiated. A Producer therefore *reads* Control and
// *writes* Data; a Consumer is the exact mirror (reads Data, writes Control).
// This holds on either side of the wire: a server stream and a client sink both
// have a producer that reads Control and writes Data. Bidi hosts one of each
// role, sharing the streamId.

// A demand grant: "send N more items." Not tied to the wire's 31-bit REQUEST_N.
struct RequestN {
  uint64_t n{0};
};

// A stream data item. Holds the item bytes plus optional per-chunk metadata
// (compression, checksum, and the payload-kind variant that carries a
// mid-stream exception); serialization to a rocket PAYLOAD frame is a
// downstream concern. `metadata` is null for a plain data passthrough.
struct Payload {
  std::unique_ptr<folly::IOBuf> data{nullptr};
  std::unique_ptr<apache::thrift::StreamPayloadMetadata> metadata{nullptr};
};

// End-of-stream: the producer has no more items. Carries no data and is ordered
// after all payloads on the stream.
struct Complete {};

// Stream teardown requested by the consumer. Inbound terminal: the peer no
// longer wants items, so a producer should stop and any buffered-but-unsent
// items may be discarded. Carries no data.
struct Cancel {};

// Stream teardown caused by a producer failure. Outbound terminal: ordered
// after all payloads, it ends the stream in error rather than success. Carries
// the failure so a downstream handler can serialize it to the wire.
struct Error {
  folly::exception_wrapper ex;
};

// CompactVariant keeps each set's discriminator to a single byte so the message
// stays inline in TypeErasedBox.
//
// The consumer -> producer set: demand grants and consumer-initiated teardown.
// A producer reads these; a consumer writes them.
using ControlVariant = CompactVariant<RequestN, Cancel>;

struct ControlMessage {
  ControlVariant payload;
};

static_assert(channel_pipeline::TypeErasedBox::fits_inline<ControlMessage>());

// The producer -> consumer set: data items and producer-initiated terminals
// (success or failure). A producer writes these; a consumer reads them.
using DataVariant = CompactVariant<Payload, Complete, Error>;

struct DataMessage {
  DataVariant payload;
};

static_assert(channel_pipeline::TypeErasedBox::fits_inline<DataMessage>());

} // namespace apache::thrift::fast_thrift::thrift::stream
