---
description: Thrift client layer - bridge between Apache Thrift RequestChannel API and fast_thrift pipeline
oncalls:
  - thrift
---

# Thrift Client Layer

This skill covers the Thrift-layer client implementation that bridges Apache Thrift's `RequestChannel` API to the fast_thrift channel pipeline.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          Application Layer                                   │
│                    (Generated Thrift Client Code)                            │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                       ThriftClientChannel                                    │
│  - Implements apache::thrift::RequestChannel                                 │
│  - Manages pending callbacks keyed by requestId                              │
│  - Serializes RequestRpcMetadata via serializeRequestMetadata()              │
│  - Deserializes ResponseRpcMetadata via deserializeResponseMetadata()        │
│  - Processes ParsedFrame → ClientReceiveState via                            │
│    processRequestResponseFrame() (ThriftClientResponseProcessor.h)           │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │ ThriftRequestMessage          │
                    │ ThriftResponseMessage         │
                    └───────────────┬───────────────┘
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                ThriftClientRocketInterfaceHandler                             │
│  - Converts ThriftRequestMessage → RocketRequestMessage (outbound)           │
│  - Converts RocketResponseMessage → ThriftResponseMessage (inbound)          │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │ RocketRequestMessage          │
                    │ RocketResponseMessage         │
                    └───────────────┬───────────────┘
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Rocket Layer                                       │
│                    (../rocket/client/...)                                    │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Handlers Overview

| Handler | Type | Purpose |
|---------|------|---------|
| `ThriftClientChannel` | App Adapter | Implements `RequestChannel`, manages callbacks, serializes/deserializes metadata |
| `ThriftClientMetadataPushHandler` | Inbound | Handles metadata push (SETUP response, drain) |
| `ThriftClientRocketInterfaceHandler` | Duplex | Interface between Thrift and Rocket messages |

---

## ThriftClientChannel

The main entry point implementing `apache::thrift::RequestChannel`.

### Key Features
- Factory method: `ThriftClientChannel::newChannel(socket, protocolId)`
- Owns `TransportHandler` (created from socket in constructor)
- Request-response RPC via `sendRequestResponse()`
- Callback correlation using `pendingCallbacks_` map keyed by `requestId`
- Receives pipeline responses via `onMessage()` (ClientInboundAppAdapter concept)
- Serializes `RequestRpcMetadata` into IOBuf via `serializeRequestMetadata()` (RequestMetadata.h)
- Deserializes `ResponseRpcMetadata` from `ParsedFrame` via `deserializeResponseMetadata()` (ResponseMetadata.h)

### Usage

```cpp
// Create channel with socket - channel owns the TransportHandler
auto channel = ThriftClientChannel::newChannel(std::move(socket));

// Build pipeline using channel's transport handler
auto pipeline = PipelineBuilder<ThriftClientChannel, TransportHandler, Allocator>()
    .setEventBase(channel->getEventBase())
    .setHead(channel.get())
    .setTail(channel->transportHandler())
    .setAllocator(&allocator)
    // ... add handlers ...
    .build();

channel->setPipeline(std::move(pipeline));

// Use with generated Thrift client
auto client = MyService::newClient(std::move(channel));
co_await client->co_myMethod(request);
```

### Implementation Status

- ✅ Request-Response (`sendRequestResponse`)
- ❌ One-way (`sendRequestNoResponse`)
- ❌ Streaming (`sendRequestStream`)
- ❌ Sink (`sendRequestSink`)

---

## Metadata Free Functions

Metadata serialization/deserialization is handled by free functions in `RequestMetadata.h` and `ResponseMetadata.h`, called directly by `ThriftClientChannel`.

### RequestMetadata.h
- `serializeRequestMetadata(metadata, data)` — Serializes `RequestRpcMetadata` into an IOBuf with headroom for downstream frame headers.

### ResponseMetadata.h
- `deserializeResponseMetadata(frame)` — Deserializes `ResponseRpcMetadata` from a `ParsedFrame`. Returns `Expected<ResponseRpcMetadata, exception_wrapper>`.
- `processPayloadMetadata(metadata)` — Processes payload metadata, handling declared/undeclared exceptions. Returns empty `exception_wrapper` on success.

---

## ThriftClientResponseProcessor

Free function that processes request-response frames directly, called by `ThriftClientChannel::handleRequestResponse()`.

### Responsibilities
- Extracts data from PAYLOAD frames (zero-copy)
- Decodes ERROR frames using `decodeErrorFrame()`
- Returns `folly::Expected<unique_ptr<IOBuf>, exception_wrapper>`

---

## Message Types

**ThriftRequestPayload**:
```cpp
#pragma pack(push, 1)
struct ThriftRequestPayload {
  std::unique_ptr<folly::IOBuf> metadata;  // pre-serialized metadata
  std::unique_ptr<folly::IOBuf> data;
  uint32_t initialRequestN{0};
  frame::FrameType frameType{frame::FrameType::RESERVED};
  bool complete{false};
};
#pragma pack(pop)
```

**ThriftRequestMessage** (outbound):
```cpp
#pragma pack(push, 1)
struct ThriftRequestMessage {
  ThriftRequestPayload payload;
  uint32_t requestHandle{rocket::kNoRequestHandle};
};
#pragma pack(pop)
```

**ThriftResponseMessage** (inbound):
```cpp
#pragma pack(push, 1)
struct ThriftResponseMessage {
  frame::read::ParsedFrame payload;
  uint32_t requestHandle{rocket::kNoRequestHandle};
  frame::FrameType requestFrameType{frame::FrameType::RESERVED};
};
#pragma pack(pop)
```

---

## Key Concepts

### Request Handle

The `requestHandle` is a `uint32_t` opaque identifier for response correlation:
- Channel generates unique `requestHandle` for each request
- Rocket layer stores `requestHandle` by `streamId`
- Responses carry `requestHandle` back to channel for callback lookup

### Protocol ID

`protocolId` (T_COMPACT_PROTOCOL, T_BINARY_PROTOCOL) is per-connection and stored on the channel.

---

## Testing

```bash
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/thrift/client/test:thrift_client_channel_test
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/thrift/client/test:thrift_client_response_processor_test
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/thrift/client/test:thrift_client_integration_test
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/thrift/client/test:thrift_client_backwards_compatibility_e2e_test
```

---

## Benchmarks

```bash
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/thrift/client/bench:thrift_client_integration_bench
```

---

## Key Files

| File | Purpose |
|------|---------|
| `ThriftClientChannel.h/.cpp` | Main entry point, implements `RequestChannel` |
| `RequestMetadata.h` | Free function for request metadata serialization |
| `ResponseMetadata.h` | Free functions for response metadata deserialization |
| `ThriftClientResponseProcessor.h` | Free function for response processing |
| `ThriftClientMetadataPushHandler.h` | Metadata push handling |
| `ThriftClientRocketInterfaceHandler.h` | Thrift/Rocket message conversion |
| `ErrorDecoding.h` | Exception decoding utilities |
| `Messages.h` | ThriftRequestMessage, ThriftResponseMessage types |
