---
description: Framing layer for fast_thrift - frame parsing, serialization, and frame types
oncalls:
  - thrift
---

# Framing Layer

## Parse-Once Design

fast_thrift uses a parse-once design for efficiency:

```cpp
// Parse frame once, cache common fields
auto frame = parseFrame(std::move(buffer));

// Access cached fields (O(1), no parsing)
uint32_t streamId = frame.streamId();
FrameType type = frame.type();
bool hasMetadata = frame.flags().metadata();

// Lazy access for frame-specific fields
if (type == FrameType::REQUEST_STREAM) {
  RequestStreamView view(frame);
  uint32_t initialN = view.initialRequestN();  // Parsed on-demand
}
```

---

## Writing Frames

```cpp
#include <thrift/lib/cpp2/fast_thrift/frame/write/FrameWriter.h>

auto frame = serialize(
    RequestResponseHeader{.streamId = 42, .follows = false},
    std::move(metadata),
    std::move(data)
);
```

---

## Frame Types

| FrameType | Has Payload | Client Sends | Server Sends |
|-----------|-------------|--------------|--------------|
| SETUP | ✓ | ✓ | |
| REQUEST_RESPONSE | ✓ | ✓ | |
| REQUEST_STREAM | ✓ | ✓ | |
| PAYLOAD | ✓ | ✓ | ✓ |
| ERROR | ✓ | ✓ | ✓ |
| REQUEST_N | | ✓ | ✓ |
| CANCEL | | ✓ | |
| KEEPALIVE | ✓ | ✓ | ✓ |

---

## Key Directories

| Directory | Purpose |
|-----------|---------|
| `fast_thrift/frame/` | RSocket frame parsing/serialization |
| `fast_thrift/frame/read/` | Inbound frame parsing (ParsedFrame, FrameViews) |
| `fast_thrift/frame/write/` | Outbound frame serialization (FrameHeaders, FrameWriter) |

---

## Adding New Frame Type Support

1. Add to `FrameType` enum in `framing/FrameType.h`
2. Add descriptor in `framing/FrameDescriptor.h`
3. Create `*Header` struct in `frame/write/FrameHeaders.h`
4. Add `serialize()` overload in `frame/write/FrameWriter.h`
5. Create `*View` class in `frame/read/FrameViews.h` (if needed)
6. Update handler(s) to process new frame type
7. **Testing:**
   - Add unit tests for serialization/deserialization in `frame/test/`
   - Add unit tests for the frame view in `frame/read/test/`
   - Update integration tests if the frame affects pipeline behavior
   - Add E2E tests to verify wire compatibility
