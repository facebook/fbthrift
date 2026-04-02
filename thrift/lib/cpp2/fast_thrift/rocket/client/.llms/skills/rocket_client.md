---
description: Rocket client handlers - setup, stream state management, and request/response handling
oncalls:
  - thrift
---

# Rocket Client Handlers

This skill covers the client-side RSocket handlers in `rocket/client/`.

## ⚠️ No Thrift Dependencies

**Rocket client code MUST NOT reference any Thrift code, constants, or types.** See `rocket.md` for details.

---

## Pipeline Architecture

```
Outbound: App → RocketClientStreamStateHandler → RocketClientRequestResponseFrameHandler → RocketClientFrameCodecHandler → Transport
Inbound:  App ← RocketClientSetupFrameHandler ← RocketClientStreamStateHandler ← RocketClientRequestResponseFrameHandler ← RocketClientErrorFrameHandler ← RocketClientFrameCodecHandler ← Transport
```

---

## Handlers Overview

| Handler | Type | Purpose |
|---------|------|---------|
| `RocketClientFrameCodecHandler` | Duplex | Parse IOBuf→ParsedFrame (read), extract serialized frame (write) |
| `RocketClientErrorFrameHandler` | Inbound | Intercept connection-level ERROR frames (streamId=0), convert to exceptions |
| `RocketClientSetupFrameHandler` | Inbound | Send SETUP frame on connect |
| `RocketClientStreamStateHandler` | Duplex | Assign stream IDs, track active streams, correlate responses |
| `RocketClientRequestResponseFrameHandler` | Duplex | Track REQUEST_RESPONSE streams, serialize requests, validate responses |

---

## RocketClientFrameCodecHandler

**Type:** Duplex handler

Bidirectional codec for Rocket frames.

### Read Path (Inbound)
- Receives `BytesPtr` (IOBuf) from framing layer
- Parses into `ParsedFrame` using `frame::read::parseFrame()`
- Validates frame, fires exception if invalid
- Wraps in `RocketResponseMessage` and fires upstream

### Write Path (Outbound)
- Receives `RocketRequestMessage` with serialized frame in variant
- Extracts the `std::unique_ptr<folly::IOBuf>` from the variant
- Forwards to transport

---

## RocketClientErrorFrameHandler

**Type:** Inbound handler

Intercepts connection-level ERROR frames (streamId == 0) and converts them to exceptions.

### Error Codes Handled

| Error Code | Exception Type | Description |
|------------|----------------|-------------|
| `CONNECTION_CLOSE` | `NOT_OPEN` | Server gracefully closing connection |
| `INVALID_SETUP` | `INVALID_STATE` | Connection setup failed: invalid setup |
| `UNSUPPORTED_SETUP` | `NOT_SUPPORTED` | Unsupported setup parameters |
| `REJECTED_SETUP` | `NOT_OPEN` | Setup rejected by server |
| `REJECTED_RESUME` | `NOT_OPEN` | Resume rejected by server |
| `CONNECTION_ERROR` | `END_OF_FILE` | Connection error from server |

Stream-level ERROR frames (streamId > 0) pass through unchanged.

---

## RocketClientSetupFrameHandler

**Type:** Inbound handler

Sends the RSocket SETUP frame immediately when connection is established.

### Protocol Constants

| Constant | Value | Description |
|----------|-------|-------------|
| `kRSocketMajorVersion` | 1 | RSocket protocol major version |
| `kRSocketMinorVersion` | 0 | RSocket protocol minor version |
| `kMaxKeepaliveTime` | 2³¹ - 1 | Max keepalive time (client keepalive not supported) |
| `kMaxLifetime` | 2³¹ - 1 | Max connection lifetime |

### Behavior
- On `onPipelineActivated`: creates SETUP frame via factory, serializes, fires write
- If write fails: closes the pipeline
- All reads pass through unchanged
- `setupSent_` flag prevents duplicate SETUP frames

### Usage

```cpp
RocketClientSetupFrameHandler handler(
    [&]() {
      auto metadata = createMetadata();  // Opaque IOBuf
      return std::make_pair(std::move(metadata), nullptr);
    });
```

---

## RocketClientStreamStateHandler

**Type:** Duplex handler

Manages stream ID generation and lifecycle for client-initiated streams.

### Key Responsibilities

1. **Stream ID Generation** — Odd-numbered IDs (1, 3, 5, ...) per RSocket spec
2. **Stream State Tracking** — Uses `DirectStreamMap<ClientStreamContext>` for O(1) lookup
3. **Response Correlation** — Matches inbound frames to originating requests via `requestHandle`
4. **Lifecycle Management** — Cleans up on terminal frames

### ClientStreamContext

Stored per active stream:
```cpp
struct ClientStreamContext {
  frame::FrameType requestFrameType;
  uint32_t requestHandle;
};
```

### Write Path (Outbound)
1. Validate streamId is `kInvalidStreamId` (not yet assigned)
2. Generate new stream ID via `generateStreamId()`
3. Store `ClientStreamContext` in `activeStreams_`
4. Fire write downstream

### Read Path (Inbound)
1. Connection frames (streamId=0) pass through
2. Look up stream in `activeStreams_`
3. Unknown streamId: log error, drop frame
4. Non-terminal frames: pass through
5. Terminal frames: extract context, erase from map, attach to response

### Handler Lifecycle
- `handlerRemoved`: Fails all active streams with `TTransportException::INVALID_STATE`

---

## RocketClientRequestResponseFrameHandler

**Type:** Duplex handler

Handles REQUEST_RESPONSE pattern specifically.

### Key Responsibilities

1. **Stream Tracking** — Uses `F14FastSet<uint32_t>` for active request-response stream IDs
2. **Outbound Serialization** — Serializes `RocketFramePayload` to wire format
3. **Response Validation** — Ensures proper flags on response payloads
4. **Transactional Rollback** — Removes tracking if write fails

### Write Path (Outbound)
1. Check if `frameType == REQUEST_RESPONSE`
2. Track stream ID in `requestResponseStreams_`
3. Serialize payload using `frame::write::serialize()` with `RequestResponseHeader`
4. Replace payload variant with serialized IOBuf
5. On write failure: remove from tracking

### Read Path (Inbound)
1. Check if stream is tracked
2. If not tracked: pass through
3. Erase from tracking
4. Validate response (PAYLOAD must have next/complete flag, ERROR always valid)
5. Invalid response: return Error

---

## Message Types

**RocketFramePayload** (unserialized frame data):
```cpp
#pragma pack(push, 1)
struct RocketFramePayload {
  std::unique_ptr<folly::IOBuf> metadata;  // Opaque metadata
  std::unique_ptr<folly::IOBuf> data;      // Opaque payload
  uint32_t streamId{kInvalidStreamId};
  uint32_t initialRequestN{0};
  bool follows{false};
  bool complete{false};
  bool next{false};
};
#pragma pack(pop)
```

**RocketRequestMessage** (outbound request):
```cpp
#pragma pack(push, 1)
struct RocketRequestMessage {
  /// Frame data - either unserialized payload or serialized buffer
  CompactVariant<RocketFramePayload, std::unique_ptr<folly::IOBuf>> frame;

  uint32_t requestHandle{kNoRequestHandle};  // For response correlation
  frame::FrameType frameType{frame::FrameType::RESERVED};
};
#pragma pack(pop)
```

**RocketResponseMessage** (inbound response):
```cpp
#pragma pack(push, 1)
struct RocketResponseMessage {
  frame::read::ParsedFrame frame;              // Parsed response frame
  uint32_t requestHandle{kNoRequestHandle}; // For response correlation
  frame::FrameType requestFrameType;      // Original request's frame type
};
#pragma pack(pop)
```

---

## Testing

```bash
# Setup handler
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/test:client_setup_frame_handler_test

# Stream state handler
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/test:client_stream_state_handler_test

# Request/response handler
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/test:client_request_response_frame_handler_test

# Frame codec handler
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/test:rocket_client_frame_codec_handler_test

# Error handler
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/test:rocket_client_error_frame_handler_test

# Integration test
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/test:rocket_client_integration_test
```

---

## Benchmarks

```bash
# Handler microbenchmarks
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/handler/bench:rocket_client_setup_frame_handler_bench
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/handler/bench:rocket_client_stream_state_handler_bench
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/handler/bench:rocket_client_request_response_frame_handler_bench
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/handler/bench:rocket_client_frame_codec_handler_bench
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/handler/bench:rocket_client_error_frame_handler_bench

# Integration benchmark
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/bench:rocket_client_integration_bench
```

---

## Key Files

| File | Purpose |
|------|---------|
| `handler/RocketClientFrameCodecHandler.h` | Frame parsing/extraction |
| `handler/RocketClientErrorFrameHandler.h` | Connection-level ERROR handling |
| `handler/RocketClientSetupFrameHandler.h` | SETUP frame on connect |
| `handler/RocketClientStreamStateHandler.h` | Stream ID management |
| `handler/RocketClientRequestResponseFrameHandler.h` | REQUEST_RESPONSE handling |
| `Messages.h` | RocketFramePayload, RocketRequestMessage, RocketResponseMessage |
