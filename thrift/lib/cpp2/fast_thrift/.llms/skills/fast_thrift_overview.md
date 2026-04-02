---
description: High-level overview of fast_thrift - architecture, design philosophy, and navigation to detailed skills
oncalls:
  - thrift
---

# fast_thrift Overview

> The most performant and efficient transport ever designed.

## Design Philosophy

**Every cycle counts. Every allocation is suspect. Every virtual call is a failure.**

### Core Principles

1. **Zero virtualization** — No virtual functions in the hot path. Templates and concepts only.
2. **Composable by design** — Users plug in handlers as needed. The framework imposes no constraints.
3. **Parse once, access many** — Headers cached on first parse. Lazy field access for the rest.
4. **Inline everything** — Messages ≤120 bytes fit in `TypeErasedBox`. No heap allocation.
5. **Move, never copy** — Zero-copy semantics throughout the entire pipeline.

### Performance Mindset

| Bad | Good |
|-----|------|
| `virtual` dispatch | Templates + concepts |
| `std::function` | Function pointers, lambdas with captures by value |
| `std::shared_ptr` | `std::unique_ptr`, raw pointers with clear ownership |
| `std::map` | `F14FastMap`, flat arrays, `DirectStreamMap` |
| Heap allocation | Inline storage, stack allocation |
| Parsing twice | Cached fields, lazy views |
| Copying buffers | Move semantics, `IOBuf` chains |

---

## Architecture

fast_thrift is a ground-up rewrite of Thrift's transport layer, designed for maximum performance through:
- **Channel Pipeline architecture**: Composable handler chains with zero overhead
- **Template-based dispatch**: No virtual calls in hot paths
- **Parse-once semantics**: Headers cached on first parse, lazy field access
- **Zero-copy operations**: Move semantics throughout, no cloning
- **Inline storage**: Messages ≤120 bytes fit in `TypeErasedBox` (128-byte struct, 2 cache lines)

### Core Layers

```
┌─────────────────────────────────────────────────────────────────┐
│                      Application Layer                          │
│            (Generated Thrift Client / ThriftClientChannel)      │
└───────────────────────────────┬─────────────────────────────────┘
                                │ ThriftRequestMessage / ThriftResponseMessage
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Thrift Layer                              │
│   ThriftClientMetadataHandler → ThriftClientRequestResponseHandler│
└───────────────────────────────┬─────────────────────────────────┘
                                │ RocketRequestMessage / RocketResponseMessage
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                        Rocket Layer                              │
│   RocketClientSetupFrameHandler → RocketClientStreamStateHandler             │
│   → RocketClientRequestResponseFrameHandler → RocketClientFrameCodec   │
└───────────────────────────────┬─────────────────────────────────┘
                                │ ParsedFrame / IOBuf
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                       Framing Layer                              │
│   FrameLengthParserHandler → FrameLengthEncoderHandler           │
│   → BatchingFrameHandler                                         │
└───────────────────────────────┬─────────────────────────────────┘
                                │ IOBuf (raw bytes)
                                ▼
┌─────────────────────────────────────────────────────────────────┐
│                      Transport Layer                             │
│                      TransportHandler                            │
└─────────────────────────────────────────────────────────────────┘
```

### Key Directories

| Directory | Purpose |
|-----------|---------|
| `fast_thrift/channel_pipeline/` | Core pipeline framework (handlers, contexts, type erasure) |
| `fast_thrift/frame/` | RSocket frame types and shared protocol constants |
| `fast_thrift/frame/read/` | Inbound frame parsing (ParsedFrame, FrameViews) |
| `fast_thrift/frame/read/handler/` | Inbound frame handlers (FrameDefragmentationHandler, FrameLengthParserHandler) |
| `fast_thrift/frame/write/` | Outbound frame serialization (FrameHeaders, FrameWriter) |
| `fast_thrift/frame/write/handler/` | Outbound frame handlers (BatchingFrameHandler, FrameFragmentationHandler, FrameLengthEncoderHandler) |
| `fast_thrift/rocket/client/handler/` | Client-side RSocket handlers |
| `fast_thrift/rocket/server/handler/` | Server-side RSocket handlers |
| `fast_thrift/rocket/server/connection/` | Server connection management |
| `fast_thrift/thrift/client/` | Thrift layer client (ThriftClientChannel) |
| `fast_thrift/thrift/client/handler/` | Thrift client handlers (metadata, request/response) |
| `fast_thrift/thrift/server/` | Thrift layer server (ThriftServerChannel handles metadata + dispatch) |
| `fast_thrift/transport/` | AsyncSocket wrapper (TransportHandler) |
| `fast_thrift/bench/` | E2E benchmark suite |

---

## Detailed Skills

For in-depth information on specific topics, load the relevant skill:

| Topic | Skill Location | When to Load |
|-------|----------------|--------------|
| **Channel Pipeline** | `../../channel_pipeline/.llms/skills/channel_pipeline.md` | Understanding/modifying pipeline, adding handlers |
| **TypeErasedBox** | `../../channel_pipeline/.llms/skills/type_erasure.md` | Adding new message types, debugging size issues |
| **Handler Patterns** | `../../channel_pipeline/.llms/skills/handler.md` | Implementing new handlers, testing patterns |
| **Framing Layer** | `../../frame/.llms/skills/framing.md` | Working with frame parsing/serialization |
| **Rocket Layer** | `../../rocket/.llms/skills/rocket.md` | Rocket protocol overview |
| **Rocket Client** | `../../rocket/client/.llms/skills/rocket_client.md` | Client handlers, setup, stream state |
| **Rocket Server** | `../../rocket/server/.llms/skills/rocket_server.md` | Server handlers, setup validation |
| **Thrift Layer** | `../../thrift/.llms/skills/thrift.md` | Thrift layer overview |
| **Thrift Client** | `../../thrift/client/.llms/skills/thrift_client.md` | ThriftClientChannel, metadata handlers |
| **Thrift Server** | `../../thrift/server/.llms/skills/thrift_server.md` | ThriftServerChannel, server metadata |
| **Transport** | `../../transport/.llms/skills/transport.md` | Socket I/O, TransportHandler |
| **Benchmarking** | `common/benchmarking.md` | Running benchmarks, validating optimizations |

### Directory Structure

Skills are co-located with their code:

```
fbcode/thrift/lib/cpp2/fast_thrift/
├── .llms/
│   ├── rules/
│   │   └── fast_thrift.md      # Entry point rules
│   └── skills/
│       ├── fast_thrift_overview.md  # This file
│       └── common/
│           └── benchmarking.md
│
├── channel_pipeline/           # Moved under fast_thrift
│   └── .llms/skills/
│       ├── channel_pipeline.md     # Pipeline framework
│       ├── type_erasure.md         # TypeErasedBox constraints
│       └── handler.md              # Handler implementation & testing
│
├── frame/                      # Renamed from framing/
│   ├── .llms/skills/
│   │   └── framing.md              # Frame layer
│   ├── read/                       # Renamed from reading/
│   │   └── handler/                # Frame read handlers
│   └── write/                      # Renamed from writing/
│       └── handler/                # Frame write handlers
│
├── rocket/
│   ├── .llms/skills/
│   │   └── rocket.md               # Rocket protocol overview
│   ├── client/
│   │   ├── .llms/skills/
│   │   │   └── rocket_client.md    # Rocket client handlers
│   │   └── handler/                # Client handler implementations
│   └── server/
│       ├── .llms/skills/
│       │   └── rocket_server.md    # Rocket server handlers
│       ├── handler/                # Server handler implementations
│       └── connection/             # Connection management
│
├── thrift/
│   ├── .llms/skills/
│   │   └── thrift.md               # Thrift layer overview
│   ├── client/
│   │   ├── .llms/skills/
│   │   │   └── thrift_client.md    # Thrift client layer
│   │   └── handler/                # Client handler implementations
│   └── server/
│       └── .llms/skills/
│           └── thrift_server.md    # Thrift server layer
│
└── transport/
    └── .llms/skills/
        └── transport.md            # Transport layer
```

---

## Key Files Reference

| Purpose | File |
|---------|------|
| Pipeline builder | `fast_thrift/channel_pipeline/PipelineBuilder.h` |
| Type-erased box | `fast_thrift/channel_pipeline/TypeErasedBox.h` |
| Handler base concepts | `fast_thrift/channel_pipeline/Handler.h` |
| Client channel entry point | `fast_thrift/thrift/client/ThriftClientChannel.h` |
| Request/response messages | `fast_thrift/thrift/client/Messages.h` |
| Rocket messages | `fast_thrift/rocket/client/Messages.h` |
| Frame parsing | `fast_thrift/frame/read/FrameParser.h` |
| Frame writing | `fast_thrift/frame/write/FrameWriter.h` |
| Transport handler | `fast_thrift/transport/TransportHandler.h` |

---

## Quick Commands

```bash
# Run handler microbenchmark
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/.../bench:*_bench

# Run integration benchmark
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/bench:rocket_client_integration_bench

# E2E benchmark
buck2 run fbcode//thrift/lib/cpp2/fast_thrift/bench:benchmark_server -- --port=5001
buck2 run fbcode//thrift/lib/cpp2/fast_thrift/bench:benchmark_client -- --port=5001 --client_threads=8

# Run tests
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/...
```
