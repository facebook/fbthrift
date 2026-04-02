---
description: Channel Pipeline framework for fast_thrift - handlers, contexts, pipelines, and message flow
oncalls:
  - thrift
---

# Channel Pipeline Framework

## Core Concepts

**Pipeline**: A bidirectional chain of handlers processing messages.

```cpp
// Outbound: App → Transport (onWrite)
// Inbound:  Transport → App (onRead)
```

**Handler**: A unit of processing. Handlers are templates parameterized by Context.

```cpp
template <typename Context>
class MyHandler {
 public:
  // Inbound message processing
  channel_pipeline::Result onRead(Context& ctx, channel_pipeline::TypeErasedBox msg) noexcept;

  // Outbound message processing
  channel_pipeline::Result onWrite(Context& ctx, channel_pipeline::TypeErasedBox msg) noexcept;

  // Connection events
  void onPipelineActivated(Context& ctx) noexcept;
  void onPipelineDeactivated(Context& ctx) noexcept;
  void onException(Context& ctx, folly::exception_wrapper&& e) noexcept;
};
```

**TypeErasedBox**: Type-erased container with 120-byte inline storage (128-byte total struct). Messages that fit inline avoid heap allocation.

**Context**: Provides methods to fire events to next/previous handlers:
- `ctx.fireRead(msg)` → forward inbound
- `ctx.fireWrite(msg)` → forward outbound
- `ctx.activate()` → pipeline activated
- `ctx.fireException(e)` → propagate error

---

## ⚠️ CRITICAL: Single Message Type Per Layer

**Each layer of handlers should use a single message type. Conversion happens ONLY at layer boundaries via interface handlers.**

### The Pattern

```
┌─────────────────────────────────────────────────────────────────┐
│  Layer A Handlers                                                │
│  (all use MessageTypeA — can be reordered freely)               │
└─────────────────────────────────────────────────────────────────┘
                              │
                    [Interface Handler]
                    (converts A → B)
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  Layer B Handlers                                                │
│  (all use MessageTypeB — can be reordered freely)               │
└─────────────────────────────────────────────────────────────────┘
                              │
                    [Interface Handler]
                    (converts B → C)
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│  Layer C Handlers                                                │
│  (all use MessageTypeC — can be reordered freely)               │
└─────────────────────────────────────────────────────────────────┘
```

### Why This Matters

1. **Handler Flexibility** — Handlers within a layer can be added, removed, or reordered without breaking anything
2. **No Hidden Dependencies** — Handlers don't depend on specific upstream handlers having transformed the message
3. **Allocation Cost** — Converting messages creates new `TypeErasedBox` instances, which has overhead
4. **Clear Boundaries** — Interface handlers make layer transitions explicit and predictable

### Best Practices

```cpp
// ✅ GOOD: Modify message in-place within a layer
Result onWrite(Context& ctx, TypeErasedBox&& msg) noexcept {
  auto& request = msg.get<LayerMessage>();
  request.someField = computeValue();  // Modify in-place
  return ctx.fireWrite(std::move(msg)); // Forward same message
}

// ✅ GOOD: Use CompactVariant for state transitions within same message
Result onWrite(Context& ctx, TypeErasedBox&& msg) noexcept {
  auto& request = msg.get<LayerMessage>();
  auto& payload = request.frame.get<UnseriazliedPayload>();
  auto serialized = serialize(payload);
  request.frame = std::move(serialized);  // Variant transition, same message type
  return ctx.fireWrite(std::move(msg));
}

// ⚠️ ONLY at layer boundary (interface handler)
Result onWrite(Context& ctx, TypeErasedBox&& msg) noexcept {
  auto layerAMsg = msg.take<LayerAMessage>();
  LayerBMessage layerBMsg = convertAtBoundary(layerAMsg);
  return ctx.fireWrite(erase_and_box(std::move(layerBMsg)));
}
```

### Use CompactVariant for State Transitions

When a message needs to represent different states (e.g., before/after serialization), use `CompactVariant` instead of creating separate message types:

```cpp
struct LayerMessage {
  // Single message type with variant for different states
  CompactVariant<
      UnseriazliedPayload,          // Before serialization
      std::unique_ptr<folly::IOBuf> // After serialization
  > frame;

  uint32_t handle;
  FrameType frameType;
};
```

This allows handlers within the layer to:
- Work with the same message type throughout
- Transition state via the variant (no new allocation)
- Remain loosely coupled and freely reorderable

---

## Building Pipelines

```cpp
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>

auto pipeline = PipelineBuilder<HeadHandler, TailHandler, Allocator>()
    .setEventBase(evb)
    .setHead(headHandler.get())
    .setTail(tailHandler.get())
    .setAllocator(&allocator)
    .addHandler<FramingHandler>()
    .addHandler<LayerAHandler1>()
    .addHandler<LayerAHandler2>()
    .addHandler<LayerAInterfaceHandler>()  // Converts LayerA → LayerB
    .addHandler<LayerBHandler1>()
    .addHandler<LayerBHandler2>()
    .build();
```

### Composability

The pipeline is a **compile-time composition** of handlers:
- Handlers are templates parameterized by `Context`
- No runtime polymorphism — handler chain is fixed at build time
- Users add/remove handlers to customize behavior
- Each handler does one thing well

```cpp
// Users compose their own pipeline
auto pipeline = PipelineBuilder<Head, Tail, Allocator>()
    .addHandler<FramingHandler>()
    .addHandler<ProtocolHandler>()
    .addHandler<MyCustomHandler>()  // User's custom logic
    .build();
```

---

## Result Type

Handlers return `channel_pipeline::Result`:
- `Result::Ok` — continue processing
- `Result::Backpressure` — operation accepted but slow down
- `Result::Error` — operation failed

---

## Connection Ownership Pattern

The channel does NOT own pipeline components. Use a connection struct:

```cpp
template <typename ClientType>
struct ClientConnection {
  channel_pipeline::SimpleBufferAllocator allocator;  // Destroyed last
  TransportHandler::Ptr transportHandler;
  channel_pipeline::PipelineImpl::Ptr pipeline;
  std::unique_ptr<ClientType> client;                 // Destroyed first
};
```
