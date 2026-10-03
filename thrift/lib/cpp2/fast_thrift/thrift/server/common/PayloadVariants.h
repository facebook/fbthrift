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

#include <memory>

#include <thrift/lib/cpp2/fast_thrift/common/allocator/EvbAllocator.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftControlPayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftPayloadVariant.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftRequestPayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftResponsePayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/ConnectionPayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/StreamResponsePayloads.h>

namespace apache::thrift::fast_thrift::thrift {

// Per-direction payload variants for the server side. Each lists only the
// alternatives that are actively wired end-to-end today; new alternatives
// join as their handlers come online (no shape change at consumer sites —
// they continue to take the same wrapper variant type).

// Server-side inbound — what the server pipeline reads from the wire.
// Unary REQUEST_RESPONSE, the stream-opening REQUEST_STREAM, connection setup,
// the established-stream REQUEST_N flow-control frame the stream mux routes by
// streamId, and the header-only cancellation (CANCEL) that the transport
// adapter turns into a ThriftServerRequestCancellationEvent — routed by
// streamId to the mux (stream cancel) or the request-lifecycle handler (unary
// cancel).
using ThriftServerRequestResponsePayload = BasicThriftRequestResponsePayload<
    std::unique_ptr<apache::thrift::RequestRpcMetadata>>;

inline std::unique_ptr<apache::thrift::RequestRpcMetadata>
makeServerRequestMetadata(folly::EventBase&) {
  return std::make_unique<apache::thrift::RequestRpcMetadata>();
}

inline std::unique_ptr<apache::thrift::RequestRpcMetadata>
makeServerRequestMetadata(
    folly::EventBase&, apache::thrift::RequestRpcMetadata metadata) {
  return std::make_unique<apache::thrift::RequestRpcMetadata>(
      std::move(metadata));
}

using ThriftServerInboundPayloadVariant = ThriftPayloadVariant<
    ThriftServerRequestResponsePayload,
    ThriftRequestStreamPayload,
    ThriftConnectionSetupPayload,
    ThriftRequestNPayload,
    ThriftRequestCancellationPayload>;

// Server-side outbound — what the server pipeline writes to the wire.
// Unary initial response + error, plus the established-stream response chunks
// (first chunk + continuing chunks) the stream mux emits for a producing
// stream.
using ThriftServerOutboundPayloadVariant = ThriftPayloadVariant<
    ThriftInitialResponsePayload,
    ThriftServerStreamOpenPayload,
    ThriftStreamInitialResponsePayload,
    ThriftStreamPayload,
    ThriftErrorPayload,
    ThriftSetupResponsePayload,
    ThriftSetupRejectionPayload>;

// The variant is inline storage sized by its largest alternative, and the
// inbound one sits on every request message. REQUEST_STREAM's payload sets that
// upper bound (it adds `initialRequestN` on top of the RR payload's shape); the
// setup and stream-control payloads stay no larger than the RR payload, so a
// field added by value to one of those fails here rather than silently widening
// the request path further.
static_assert(
    sizeof(ThriftConnectionSetupPayload) <=
        sizeof(ThriftServerRequestResponsePayload),
    "connection-lifecycle payloads must not grow the per-request inbound "
    "message; put new state in ConnectionSetupData instead");

static_assert(
    sizeof(ThriftRequestNPayload) <= sizeof(ThriftServerRequestResponsePayload),
    "stream flow-control payloads must not grow the per-request inbound "
    "message");

// Pins the sizing claim in the comment above: REQUEST_STREAM's payload is the
// largest inbound alternative, so it alone sets the per-request message size.
// If another alternative grows past it, revisit that comment (and whether the
// growth is intended) rather than letting the bound move silently.
static_assert(
    sizeof(ThriftRequestResponsePayload) <=
            sizeof(ThriftRequestStreamPayload) &&
        sizeof(ThriftConnectionSetupPayload) <=
            sizeof(ThriftRequestStreamPayload) &&
        sizeof(ThriftRequestNPayload) <= sizeof(ThriftRequestStreamPayload),
    "ThriftRequestStreamPayload is expected to be the largest inbound "
    "alternative (it adds initialRequestN on top of the RR payload shape)");

} // namespace apache::thrift::fast_thrift::thrift
