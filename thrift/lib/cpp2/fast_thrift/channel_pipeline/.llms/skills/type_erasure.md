---
description: TypeErasedBox size limits and requirements for fast_thrift message types
oncalls:
  - thrift
---

# TypeErasedBox & Type Erasure

## ⚠️ CRITICAL: 120-Byte Size Limit

**All pipeline messages MUST fit within 120 bytes.**

`TypeErasedBox` is the container used to pass messages between handlers. It has:
- **120 bytes** inline storage capacity
- **128 bytes** total struct size (fits in two L1 cache lines)
- **Zero heap allocation** when messages fit inline

### What Happens When You Exceed 120 Bytes

| Build Mode | Behavior |
|------------|----------|
| **Debug (`-c dbg`)** | **Compile-time error** with clear message |
| **Optimized (`-c opt`)** | **Silent undefined behavior** — may corrupt memory! |

The `static_assert` that enforces this is only active in debug builds. **Always test in debug mode when adding new message types.**

---

## How to Check Your Type's Size

```cpp
static_assert(sizeof(MyMessageType) <= 120, "MyMessageType exceeds TypeErasedBox capacity!");
static_assert(alignof(MyMessageType) <= 8, "MyMessageType alignment exceeds limit!");
static_assert(std::is_nothrow_move_constructible_v<MyMessageType>, "Must be nothrow move constructible!");
```

---

## If Your Type Exceeds 120 Bytes

### Option 1: Reduce the type's size (preferred)

- Use `#pragma pack(push, 1)` to eliminate padding
- Order fields by alignment (8B → 4B → 2B → 1B)
- Use `CompactVariant` instead of `std::variant`
- Store pointers instead of inline data
- Move large buffers to `std::unique_ptr<folly::IOBuf>`

### Option 2: Heap-allocate explicitly

```cpp
// Wrap large type in unique_ptr (8 bytes inline, data on heap)
auto box = erase_and_box(std::make_unique<MyLargeType>(...));

// Access via double indirection
auto& value = *box.get<std::unique_ptr<MyLargeType>>();
```

---

## Current Message Sizes (Reference)

| Message Type | Size | Status |
|--------------|------|--------|
| `BytesPtr` (`unique_ptr<IOBuf>`) | 8B | ✅ |
| `ParsedFrame` | ~40B | ✅ |
| `RocketRequestMessage` | ~30B | ✅ |
| `RocketResponseMessage` | ~45B | ✅ |
| `ThriftRequestMessage` | ~50B | ✅ |
| `ThriftResponseMessage` | ~60B | ✅ |

**Rule of thumb**: If your message contains more than 2-3 `unique_ptr` fields plus some scalars, measure it.

---

## CompactVariant

For messages with multiple states, use `CompactVariant<A, B, C>` (1-byte discriminator):

```cpp
CompactVariant<
    std::unique_ptr<RequestRpcMetadata>,  // Before serialization
    std::unique_ptr<folly::IOBuf>         // After serialization
> metadata;
```

---

## Memory Layout Best Practices

- **Pack structs** with `#pragma pack(push, 1)` for message types
- **Order fields by alignment** (8-byte → 4-byte → 2-byte → 1-byte)
- **Keep messages ≤120 bytes** to fit in TypeErasedBox inline storage

### Avoid

- **Heap allocation** in hot paths — use inline storage
- **Copying IOBufs** — always move
- **`std::variant`** — use `CompactVariant`

### Prefer

- **Move semantics** everywhere
- **`std::unique_ptr`** for large buffers
- **Inline storage** for small data
