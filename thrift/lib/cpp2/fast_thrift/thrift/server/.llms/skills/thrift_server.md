---
description: Thrift server layer - server-side RPC handling for fast_thrift
oncalls:
  - thrift
---

# Thrift Server Layer

This skill covers the Thrift-layer server implementation that handles incoming RPC requests in the fast_thrift pipeline.

---

## Architecture Overview

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Rocket Layer                                       │
│                    (../rocket/server/...)                                    │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │ RocketRequestMessage          │
                    │ RocketResponseMessage         │
                    └───────────────┬───────────────┘
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                       ThriftServerChannel                                    │
│  - Deserializes RequestRpcMetadata (inbound)                                 │
│  - Serializes ResponseRpcMetadata (outbound)                                 │
│  - Dispatches to application logic                                           │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          Application Layer                                   │
│                    (Service Implementation)                                  │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Components

| Component | Type | Purpose |
|-----------|------|---------|
| `ThriftServerChannel` | App Adapter | Server-side request dispatching and metadata handling |

---

## ThriftServerChannel

Server-side channel that handles incoming requests and dispatches to the application.

### Responsibilities
- Receives `RocketRequestMessage` from pipeline
- Deserializes `RequestRpcMetadata` from frame metadata
- Dispatches to registered service handlers via `AsyncProcessor`
- Pre-serializes `ResponseRpcMetadata` and sends `RocketResponseMessage` back through pipeline

### Inbound (onMessage)
1. Receives `RocketRequestMessage` from Rocket layer
2. Deserializes `RequestRpcMetadata` from frame metadata (on the stack, no heap alloc)
3. Creates `PipelineResponseChannelRequest` and dispatches to `AsyncProcessor`

### Outbound (sendReply/sendErrorWrapped)
1. Pre-serializes `ResponseRpcMetadata` using Binary protocol (with headroom for frame header)
2. Produces `RocketResponseMessage` for Rocket layer

---

## Testing

```bash
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/thrift/server/test:thrift_server_channel_test
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/thrift/server/test:thrift_server_backwards_compatibility_e2e_test
```

---

## Benchmarks

```bash
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/thrift/server/bench:headroom_pipeline_bench
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/thrift/server/bench:request_deserialization_bench
```

---

## Key Files

| File | Purpose |
|------|---------|
| `ThriftServerChannel.h/.cpp` | Server-side request handling and metadata serialization |
