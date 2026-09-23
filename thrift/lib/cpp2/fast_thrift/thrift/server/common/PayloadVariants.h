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

#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftControlPayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftPayloadVariant.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftRequestPayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/common/ThriftResponsePayloads.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/ConnectionPayloads.h>

namespace apache::thrift::fast_thrift::thrift {

// Per-direction payload variants for the server side. Each lists only the
// alternatives that are actively wired end-to-end today; new alternatives
// join as their handlers come online (no shape change at consumer sites —
// they continue to take the same wrapper variant type).

// Server-side inbound — what the server pipeline reads from the wire.
// Unary REQUEST_RESPONSE, connection setup, and the established-stream
// flow-control frames (REQUEST_N / CANCEL) the stream mux routes by streamId.
using ThriftServerInboundPayloadVariant = ThriftPayloadVariant<
    ThriftRequestResponsePayload,
    ThriftConnectionSetupPayload,
    ThriftRequestNPayload,
    ThriftCancelPayload>;

// Server-side outbound — what the server pipeline writes to the wire.
// Unary initial response + error, plus the established-stream response chunks
// (first chunk + continuing chunks) the stream mux emits for a producing
// stream.
using ThriftServerOutboundPayloadVariant = ThriftPayloadVariant<
    ThriftInitialResponsePayload,
    ThriftStreamInitialResponsePayload,
    ThriftStreamPayload,
    ThriftErrorPayload,
    ThriftSetupResponsePayload,
    ThriftSetupRejectionPayload>;

// The variant is inline storage sized by its largest alternative, and the
// inbound one sits on every request message. The setup and stream-control
// payloads therefore stay no larger than the RR payload; this pins that, so a
// field added by value fails here rather than silently widening the request
// path.
static_assert(
    sizeof(ThriftConnectionSetupPayload) <=
        sizeof(ThriftRequestResponsePayload),
    "connection-lifecycle payloads must not grow the per-request inbound "
    "message; put new state in ConnectionSetupData instead");

static_assert(
    sizeof(ThriftRequestNPayload) <= sizeof(ThriftRequestResponsePayload) &&
        sizeof(ThriftCancelPayload) <= sizeof(ThriftRequestResponsePayload),
    "stream flow-control payloads must not grow the per-request inbound "
    "message");

} // namespace apache::thrift::fast_thrift::thrift
