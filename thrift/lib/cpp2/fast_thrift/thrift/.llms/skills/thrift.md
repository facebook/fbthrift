---
description: Thrift layer overview - bridges Apache Thrift API with fast_thrift pipeline
oncalls:
  - thrift
---

# Thrift Layer

The Thrift layer bridges the Apache Thrift RPC API with the fast_thrift channel pipeline. It handles RPC metadata serialization, request/response correlation, and integration with generated Thrift client/server code.

---

## Overview

The Thrift layer sits at the top of the fast_thrift stack, directly interfacing with application code:

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          Application Layer                                   │
│                    (Generated Thrift Client/Server Code)                     │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                          Thrift Layer  ← YOU ARE HERE                        │
│  - ThriftClientChannel / ThriftServerChannel                                 │
│  - Metadata serialization/deserialization                                    │
│  - Request/Response correlation                                              │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Rocket Layer                                       │
│                    (RSocket protocol handling)                               │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## Key Responsibilities

1. **API Integration** — Implements Apache Thrift's `RequestChannel` (client) and server interfaces
2. **Metadata Handling** — Serializes/deserializes `RequestRpcMetadata` and `ResponseRpcMetadata`
3. **Request Correlation** — Uses `requestHandle` to correlate responses with pending callbacks
4. **Protocol Negotiation** — Manages `protocolId` (Compact, Binary) for serialization
5. **Error Handling** — Decodes declared/undeclared exceptions from responses

---

## Detailed Skills

For client or server specific information, load the appropriate skill:

| Topic | Skill Location | When to Load |
|-------|----------------|--------------|
| **Thrift Client** | `../../client/.llms/skills/thrift_client.md` | Client channel, metadata handlers, request/response |
| **Thrift Server** | `../../server/.llms/skills/thrift_server.md` | Server channel, metadata handling |

---

## Client vs Server

### Client Side

- `ThriftClientChannel` implements `apache::thrift::RequestChannel`
- Handlers: `ThriftClientMetadataHandler`, `ThriftClientRequestResponseHandler`
- Messages: `ThriftRequestMessage` (outbound), `ThriftResponseMessage` (inbound)

### Server Side

- `ThriftServerChannel` handles incoming requests and metadata serialization
- Messages: `RocketRequestMessage` (inbound), `RocketResponseMessage` (outbound)

---

## Key Concepts

### Request Handle

A `uint32_t` opaque identifier for response correlation:
- Client generates unique handle per request
- Flows through Rocket layer (stored by streamId)
- Returns with response for callback lookup

### Protocol ID

Per-connection setting (`T_COMPACT_PROTOCOL`, `T_BINARY_PROTOCOL`):
- Set by generated client code after channel construction
- Handlers use getter function to fetch current value
- Determines serialization format for payloads

### Metadata Types

| Type | Direction | Purpose |
|------|-----------|---------|
| `RequestRpcMetadata` | Outbound | Method name, timeouts, headers, compression |
| `ResponseRpcMetadata` | Inbound | Stream metadata, exceptions, proxied payloads |

---

## Key Directories

| Directory | Purpose |
|-----------|---------|
| `thrift/client/` | Client-side channel and handlers |
| `thrift/server/` | Server-side channel and handlers |
| `thrift/test/` | End-to-end tests |

---

## Testing

```bash
# Client tests
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/thrift/client/test:

# Server tests
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/thrift/server/test:

# E2E tests
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/thrift/test:fast_thrift_e2e_test
```

---

## Benchmarks

```bash
# Client benchmarks
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/thrift/client/bench:thrift_client_integration_bench

# Server benchmarks
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/thrift/server/bench:headroom_pipeline_bench
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/thrift/server/bench:request_deserialization_bench
```
