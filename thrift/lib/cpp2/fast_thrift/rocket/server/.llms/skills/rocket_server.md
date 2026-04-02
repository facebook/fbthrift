---
description: Rocket server handlers - setup validation, stream state management, and request/response handling
oncalls:
  - thrift
---

# Rocket Server Handlers

This skill covers the server-side RSocket handlers in `rocket/server/`.

## ⚠️ No Thrift Dependencies

**Rocket server code MUST NOT reference any Thrift code, constants, or types.** See `rocket.md` for details.

---

## Pipeline Architecture

```
Inbound:  Transport → FrameLengthParser → RocketServerFrameCodecHandler → RocketServerSetupFrameHandler → RocketServerRequestResponseFrameHandler → RocketServerStreamStateHandler → App
Outbound: Transport ← FrameLengthEncoder ← RocketServerFrameCodecHandler ← RocketServerSetupFrameHandler ← RocketServerRequestResponseFrameHandler ← RocketServerStreamStateHandler ← App
```

---

## Handlers Overview

| Handler | Type | Purpose |
|---------|------|---------|
| `RocketServerFrameCodecHandler` | Duplex | Parse IOBuf→ParsedFrame (read); passthrough (write) |
| `RocketServerSetupFrameHandler` | Duplex | Validate/consume SETUP frame, reject invalid setups |
| `RocketServerRequestResponseFrameHandler` | Duplex | Track REQUEST_RESPONSE streams, serialize responses |
| `RocketServerStreamStateHandler` | Duplex | Register streams, route requests/responses, handle terminal frames |

---

## RocketServerFrameCodecHandler

**Type:** Duplex handler

### Read Path (Inbound)
- Receives raw `IOBuf` from framing layer
- Parses into `ParsedFrame` using `frame::read::parseFrame()`
- Validates frame
- Forwards `ParsedFrame` upstream

### Write Path (Outbound)
- Passthrough for serialized frames

---

## RocketServerSetupFrameHandler

**Type:** Duplex handler (two-phase design)

Validates the RSocket SETUP frame on connection establishment.

### Two-Phase Design

- **Phase 1 (awaiting setup):**
  - Validates first frame is SETUP
  - Validates major version is supported
  - Validates keepaliveTime > 0
  - Validates maxLifetime > 0
  - Stores parameters
  - **Consumes frame** (does not forward downstream)
  - Rejects invalid setups with ERROR frame, closes connection

- **Phase 2 (setup complete):**
  - Passthrough for all frames
  - Rejects duplicate SETUP frames

### Error Codes

| Error Code | Value | Description |
|------------|-------|-------------|
| `kInvalidSetup` | 0x00000001 | First frame not SETUP, duplicate SETUP, or invalid parameters |
| `kUnsupportedSetup` | 0x00000002 | Unsupported RSocket major version |

### Stored Parameters

After successful setup, available via `setupParameters()`:

| Parameter | Type | Description |
|-----------|------|-------------|
| `majorVersion` | `uint16_t` | RSocket protocol major version |
| `minorVersion` | `uint16_t` | RSocket protocol minor version |
| `keepaliveTime` | `uint32_t` | Keepalive interval (ms) |
| `maxLifetime` | `uint32_t` | Max connection lifetime (ms) |
| `hasLease` | `bool` | Whether client supports LEASE |

### Usage

```cpp
RocketServerSetupFrameHandler handler;

// After setup completes:
if (handler.isSetupComplete()) {
  auto& params = handler.setupParameters();
  configureKeepalive(params.keepaliveTime);
}
```

---

## RocketServerStreamStateHandler

**Type:** Duplex handler

Manages server-side RSocket stream state for client-initiated streams.

### Key Responsibilities

1. **Stream Registration** — Registers new streams on request-initiating frames (REQUEST_RESPONSE, REQUEST_FNF, REQUEST_STREAM, REQUEST_CHANNEL)
2. **Stream ID Tracking** — Uses `DirectStreamMap<ServerStreamContext>` for O(1) lookup
3. **Connection Frame Handling** — Drops connection-level frames (streamId=0) like KEEPALIVE
4. **Terminal Frame Handling** — Removes streams on CANCEL, ERROR, or complete PAYLOAD
5. **Transactional Rollback** — Re-adds streams if write fails after removal

### Read Path (Inbound)

```
ParsedFrame
    → Connection frame (streamId=0) → drop (not forwarded to app)
    → Request-initiating frame → register stream, wrap in ServerRequestMessage
    → Terminal frame → remove stream, wrap in ServerRequestMessage
    → Non-terminal frame (e.g., REQUEST_N) → pass through if stream active
    → Unknown streamId → log warning, drop
```

### Write Path (Outbound)

```
ServerResponseMessage from App
    → Validate streamId is active
    → If complete=true, remove stream (save context for rollback)
    → Convert to StreamResponseMessage
    → On write error: re-add stream for retry
```

### ServerStreamContext

```cpp
struct ServerStreamContext {
  void* requestContext{nullptr};  // Extensible per-stream state
};
```

---

## RocketServerRequestResponseFrameHandler

**Type:** Duplex handler

Handles REQUEST_RESPONSE pattern specifically.

### Key Responsibilities

1. **Inbound Request Tracking** — Detects REQUEST_RESPONSE frames, tracks stream IDs
2. **Outbound Serialization** — Serializes `StreamResponseMessage` to PAYLOAD frames with `complete=true, next=true`
3. **Terminal Frame Cleanup** — Removes tracking on CANCEL/ERROR
4. **Transactional Rollback** — Re-adds tracking if write fails

### Read Path (Inbound)

```
ParsedFrame
    → REQUEST_RESPONSE → track stream ID, forward (rollback on downstream error)
    → CANCEL/ERROR for tracked stream → remove tracking
    → Other frames → pass through
```

### Write Path (Outbound)

```
StreamResponseMessage
    → Check if stream is tracked
    → If not tracked: forward unchanged
    → Remove from tracking
    → Serialize payload as PAYLOAD frame (complete=true, next=true)
    → On write error: re-add to tracking
```

---

## Message Types

**ServerRequestMessage** (inbound to app):
```cpp
#pragma pack(push, 1)
struct ServerRequestMessage {
  frame::read::ParsedFrame frame;     // 40B - Parsed request frame
  folly::exception_wrapper error; // 8B - Set on connection failure
  uint32_t streamId{0};           // 4B - Client-assigned stream ID
};
#pragma pack(pop)
```
Note: If `error` is set, the connection failed and `frame` may be empty. Check error first.

**ServerResponseMessage** (outbound from app):
```cpp
#pragma pack(push, 1)
struct ServerResponseMessage {
  std::unique_ptr<folly::IOBuf> payload;  // 8B
  std::unique_ptr<folly::IOBuf> metadata; // 8B
  uint32_t streamId{0};                   // 4B
  uint32_t errorCode{0};                  // 4B
  bool complete{true};                    // 1B - Terminal response removes stream
};
#pragma pack(pop)
```

**StreamResponseMessage** (wire-level, internal):
```cpp
#pragma pack(push, 1)
struct StreamResponseMessage {
  std::unique_ptr<folly::IOBuf> payload;  // 8B
  std::unique_ptr<folly::IOBuf> metadata; // 8B
  uint32_t streamId{0};                   // 4B
  uint32_t errorCode{0};                  // 4B
};
#pragma pack(pop)
```

---

## Error Handling

### Backpressure Semantics

`Result::Backpressure` is a **soft signal**, not an error:
- Operation **was accepted** and will be processed
- Signal to **slow down**
- Stream remains active and valid
- No rollback or retry needed

### Connection-Level Errors

On `onException`:
1. Iterate over all active streams
2. Fire `ServerRequestMessage` with error for each
3. Clear active streams map
4. Propagate exception

### Handler Lifecycle

- `handlerRemoved`: Fails all active streams with `std::runtime_error("Handler removed")`
- `onPipelineDeactivated`: Asserts `activeStreams_.empty()` (all should be failed via `onException`)

### Unknown Stream ID

Logs warning and drops frame. Can happen if:
- Stream already terminated
- Protocol error
- Malformed frame

---

## Testing

```bash
# Frame codec handler
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/rocket/server/test:rocket_server_frame_codec_handler_test

# Setup handler
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/rocket/server/test:server_setup_frame_handler_test

# Stream state handler
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/rocket/server/test:server_stream_state_handler_test

# Request/response handler
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/rocket/server/test:server_request_response_frame_handler_test
```

---

## Benchmarks

```bash
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/server/bench:rocket_server_frame_codec_handler_bench
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/server/bench:server_setup_frame_handler_bench
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/server/bench:server_stream_state_handler_bench
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/server/bench:server_request_response_frame_handler_bench
```

---

## Key Files

| File | Purpose |
|------|---------|
| `RocketServerFrameCodecHandler.h` | Frame parsing (IOBuf → ParsedFrame) |
| `RocketServerSetupFrameHandler.h` | SETUP validation and parameter storage |
| `RocketServerStreamStateHandler.h` | Stream ID management and routing |
| `RocketServerRequestResponseFrameHandler.h` | REQUEST_RESPONSE handling |
| `Common.h` | ServerRequestMessage, ServerResponseMessage, StreamResponseMessage, ServerStreamContext |
