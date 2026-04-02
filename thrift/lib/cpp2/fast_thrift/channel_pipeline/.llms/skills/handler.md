---
description: Handler implementation patterns and testing strategies for fast_thrift
oncalls:
  - thrift
---

# Handler Patterns

## ⚠️ Key Principle: Minimize Message Type Conversions

**Handlers should work with existing message types in-place rather than converting to new types.**

### Why This Matters

- **Allocation cost** — Creating new `TypeErasedBox` instances has overhead
- **Tight coupling** — Converting messages couples handlers to specific types
- **Pipeline inflexibility** — Conversions create implicit order dependencies between handlers

### Preferred Patterns

```cpp
// ✅ GOOD: Modify in-place via reference
Result onWrite(Context& ctx, TypeErasedBox&& msg) noexcept {
  auto& request = msg.get<LayerMessage>();
  request.someField = computeValue();
  return ctx.fireWrite(std::move(msg));
}

// ✅ GOOD: Use variant to transition state within same message type
Result onWrite(Context& ctx, TypeErasedBox&& msg) noexcept {
  auto& request = msg.get<LayerMessage>();
  auto& payload = request.frame.get<UnserializedPayload>();
  auto serialized = serialize(payload);
  request.frame = std::move(serialized);  // Variant transition, same message
  return ctx.fireWrite(std::move(msg));
}

// ❌ AVOID: Converting to a different message type (except at layer boundaries)
Result onWrite(Context& ctx, TypeErasedBox&& msg) noexcept {
  auto old = msg.take<OldType>();
  NewType newMsg{...};  // New allocation!
  return ctx.fireWrite(erase_and_box(std::move(newMsg)));
}
```

---

## Handler Implementation Patterns

### Inbound-Only Handler

```cpp
template <typename Context>
class MyInboundHandler {
 public:
  Result onRead(Context& ctx, TypeErasedBox msg) noexcept {
    auto& message = msg.get<LayerMessage>();
    // Process...
    return ctx.fireRead(std::move(msg));
  }

  // Pass-through for outbound
  Result onWrite(Context& ctx, TypeErasedBox msg) noexcept {
    return ctx.fireWrite(std::move(msg));
  }
};
```

### Duplex Handler (Tracks State)

```cpp
template <typename Context>
class MyDuplexHandler {
 public:
  Result onWrite(Context& ctx, TypeErasedBox msg) noexcept {
    auto& req = msg.get<LayerMessage>();

    // Track outbound request
    activeStreams_.insert(req.streamId);

    return ctx.fireWrite(std::move(msg));
  }

  Result onRead(Context& ctx, TypeErasedBox msg) noexcept {
    auto& response = msg.get<LayerMessage>();

    if (activeStreams_.count(response.streamId()) == 0) {
      return ctx.fireRead(std::move(msg));  // Not ours, pass through
    }

    // Handle our response
    activeStreams_.erase(response.streamId());
    return ctx.fireRead(std::move(msg));
  }

 private:
  folly::F14FastSet<uint32_t> activeStreams_;
};
```

### Pipeline Activation Handler

```cpp
template <typename Context>
class SetupHandler {
 public:
  void onPipelineActivated(Context& ctx) noexcept {
    // Send setup frame on activation
    auto setupFrame = serialize(SetupHeader{...}, std::move(metadata), nullptr);
    ctx.fireWrite(erase_and_box<std::unique_ptr<folly::IOBuf>>(std::move(setupFrame)));
  }
};
```

### Debug Handler (Logging)

```cpp
template <typename Context>
class DebugHandler {
 public:
  Result onRead(Context& ctx, TypeErasedBox msg) noexcept {
    XLOG(INFO) << "onRead: " << describe(msg);
    return ctx.fireRead(std::move(msg));
  }

  Result onWrite(Context& ctx, TypeErasedBox msg) noexcept {
    XLOG(INFO) << "onWrite: " << describe(msg);
    return ctx.fireWrite(std::move(msg));
  }
};
```

---

## Testing Patterns

### Testing Levels Overview

| Level | Purpose | When to Use |
|-------|---------|-------------|
| **Unit Tests** | Verify component correctness in isolation | Every component change |
| **Integration Tests** | Verify component interactions and pipeline behavior | Changes affecting multiple handlers or message flow |
| **E2E Tests** | Verify full client-server communication | Protocol changes, new features |
| **Benchmarks** | Detect performance regressions | Performance-critical components |

### Before Making Changes — Testing Checklist

**Always ask yourself:**

1. **Unit Tests**
   - Does this component have unit tests?
   - Do existing tests cover the changed behavior?
   - Do I need to add new test cases for edge cases?

2. **Integration Tests**
   - Does this change affect how components interact?
   - Will the pipeline behavior change?
   - Should I add/update `BasicPipelineIntegrationTest` or `RocketClientIntegrationTest`?

3. **E2E Tests**
   - Does this change the wire protocol?
   - Are there backward compatibility concerns?
   - Should I test against a real server?

4. **Benchmarks**
   - Is this a performance-critical path?
   - Should I add a microbenchmark for the component?
   - Should I run integration benchmarks to verify no regression?

---

## Running Tests

### Unit Tests

```bash
# Run all tests for a component
buck2 test //path/to/component/test:

# Run specific test
buck2 test //path/to/component/test:my_handler_test
```

### Integration Tests

**When to add/update integration tests:**
- Adding a new handler to the pipeline
- Changing message flow between handlers
- Modifying connection lifecycle (connect/disconnect)
- Adding new frame types that traverse multiple handlers

### E2E Tests

```bash
# Terminal 1: Start test server
buck2 run //path/to/bench:server -- --port=5001

# Terminal 2: Run E2E test
buck2 run //path/to/bench:client -- --port=5001 --requests=100
```

---

## Common Tasks

### Add a New Handler

1. Create header in appropriate layer directory
2. Implement handler template with `onRead`/`onWrite`
3. **Use existing message types** — avoid creating new message types
4. **Modify messages in-place** using `.get<T>()` instead of `.take<T>()`
5. **Use CompactVariant** for state transitions within the same message
6. Add to pipeline in correct order
7. **Testing considerations:**
   - Add unit tests in `test/` subdirectory for the new handler
   - If the handler interacts with other handlers, add/update integration tests
   - If the handler affects client-server communication, consider E2E tests
8. Add benchmark if performance-critical

### Modify Existing Handler Behavior

1. Understand the current behavior and its tests
2. Implement the modification
3. **Ensure no new message type conversions are introduced**
4. **Testing considerations:**
   - Update existing unit tests to reflect new behavior
   - Check if integration tests need updates
   - If changing message flow, verify pipeline integration tests pass
   - Run benchmarks to ensure no performance regression

### Message Access Patterns

```cpp
// ✅ GOOD: Get reference to modify in-place (no allocation)
auto& msg = box.get<LayerMessage>();
msg.someField = computeValue();
return ctx.fireWrite(std::move(box));

// ⚠️ Use with caution: take() consumes the message
auto msg = box.take<LayerMessage>();
// Now you own msg, but box is empty
// Only use when you need to transform into a completely different type (interface handlers)

// ❌ AVOID: Creating new TypeErasedBox unnecessarily within a layer
auto msg = box.take<OldType>();
return ctx.fireWrite(erase_and_box(NewType{...}));  // Allocation!
```

---

## Directory Structure

```
component/
├── MyHandler.h
├── MyHandler.cpp (if needed)
├── test/
│   └── MyHandlerTest.cpp      # Unit tests
└── bench/
    └── MyHandlerBench.cpp     # Benchmarks (if performance-critical)
```
