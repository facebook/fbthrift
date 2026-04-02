---
description: Rocket protocol layer overview - RSocket-based messaging for fast_thrift
oncalls:
  - thrift
---

# Rocket Protocol Layer

The Rocket layer implements the RSocket protocol, handling stream management, request/response correlation, and frame encoding/decoding.

## ⚠️ CRITICAL: No Thrift Dependencies

**Rocket code MUST NOT reference any Thrift code, constants, or types.**

The Rocket layer is a pure RSocket implementation. It should be completely agnostic to Thrift semantics:

- ❌ **DO NOT** import from `thrift/` directories
- ❌ **DO NOT** use Thrift-specific types (e.g., `TApplicationException`, `RpcKind`)
- ❌ **DO NOT** reference Thrift constants or enums
- ❌ **DO NOT** parse Thrift metadata in Rocket handlers
- ✅ **DO** treat metadata/data as opaque `IOBuf` payloads
- ✅ **DO** let the layer above handle application-specific semantics

This separation ensures Rocket can be reused for non-Thrift RSocket applications.

---

## Overview

Rocket sits between the application layer and the Framing layer:

```
Application Layer (layer above)
        ↓ ↑
    Rocket Layer  ← YOU ARE HERE
        ↓ ↑
Framing Layer (ParsedFrame / IOBuf)
```

---

## Key Responsibilities

1. **Stream Management** — Assign and track stream IDs for concurrent requests
2. **Request/Response Correlation** — Match responses to their originating requests
3. **Setup Handshake** — Exchange capabilities on connection establishment
4. **Frame Encoding** — Convert Rocket messages to wire format
5. **Error Handling** — Process ERROR frames and propagate exceptions

---

## Detailed Skills

For client or server specific information, load the appropriate skill:

| Topic | Skill Location | When to Load |
|-------|----------------|--------------|
| **Rocket Client** | `../../client/.llms/skills/rocket_client.md` | Client-side handlers, setup, stream state |
| **Rocket Server** | `../../server/.llms/skills/rocket_server.md` | Server-side handlers, setup validation |

---

## Stream ID Assignment

- **Client** uses **odd** stream IDs (1, 3, 5, ...)
- **Server** uses **even** stream IDs (0, 2, 4, ...)
- Stream 0 is reserved for connection-level frames

---

## Key Directories

| Directory | Purpose |
|-----------|---------|
| `rocket/client/` | Client-side handlers |
| `rocket/server/` | Server-side handlers |
| `rocket/common/` | Shared utilities (DirectStreamMap, etc.) |

---

## Terminal Frame Detection

A frame is terminal (ends the stream) if:
- **ERROR frame** — Stream failed
- **CANCEL frame** — Stream cancelled
- **PAYLOAD with complete flag** — Normal completion

---

## Error Handling Principles

### Backpressure Semantics

`Result::Backpressure` is a **soft signal**, not an error:
- Operation **was accepted** and will be processed
- Signal to **slow down**
- Stream remains active and valid
- No rollback or retry needed

### Connection-Level Errors

On `onException`:
1. Iterate over all active streams
2. Notify each stream of the error
3. Clear active streams map
4. Propagate exception
