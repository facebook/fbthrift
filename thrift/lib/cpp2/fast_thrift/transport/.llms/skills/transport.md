---
description: Transport layer - bridges channel_pipeline with async socket I/O
oncalls:
  - thrift
---

# Transport Layer

This skill covers the transport handler that bridges the fast_thrift channel pipeline with the underlying async socket.

---

## Overview

`TransportHandler` is the lowest layer of the pipeline, sitting between the framing layer and the actual network socket.

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                           Framing Layer                                      │
│            (FrameLengthParserHandler / FrameLengthEncoderHandler)            │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                    ┌───────────────┴───────────────┐
                    │ BytesPtr (IOBuf)              │
                    └───────────────┬───────────────┘
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                         TransportHandler                                     │
│  - Implements AsyncTransport::ReadCallback                                   │
│  - Implements AsyncTransport::WriteCallback                                  │
│  - Implements OutboundTransportHandler concept                               │
│  - Extends folly::DelayedDestruction                                         │
└─────────────────────────────────────────────────────────────────────────────┘
                                    │
                                    ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                        AsyncTransport (Socket)                               │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

## TransportHandler

Bridges the channel_pipeline framework with the underlying async transport (socket).

### Interfaces Implemented

| Interface | Purpose |
|-----------|---------|
| `AsyncTransport::ReadCallback` | Receives data from socket, fires to pipeline |
| `AsyncTransport::WriteCallback` | Handles async write completion |
| `OutboundTransportHandler` concept | Receives bytes from pipeline, writes to socket |
| `folly::DelayedDestruction` | Safe destruction during callbacks |

### Key Behaviors

- **Single pending write**: Only one write can be pending at a time; additional writes return `Backpressure`
- **Backpressure handling**: Pauses/resumes reads based on pipeline flow control
- **Connection lifecycle**: Fires `activate()` on the pipeline when socket is ready
- **Close callback**: Notifies connection manager when transport closes

---

## Creation and Setup

### Factory Method

```cpp
auto transportHandler = TransportHandler::create(
    std::move(socket),      // AsyncTransport::UniquePtr
    minBufferSize,          // default: 4096
    maxBufferSize           // default: 65536
);
```

### Two-Phase Initialization

1. Create handler with socket
2. Set pipeline reference before any read/write operations

```cpp
// Phase 1: Create
auto transport = TransportHandler::create(std::move(socket));

// Phase 2: Set pipeline (must be called before operations)
transport->setPipeline(pipeline);
```

---

## Read Path (Inbound)

### `getReadBuffer(void** bufReturn, size_t* lenReturn)`
- Provides buffer for socket to read into
- Uses `IOBufQueue` for efficient memory management
- Preallocates between `minBufferSize_` and `maxBufferSize_`

### `readDataAvailable(size_t len)`
- Called when data is available
- Wraps data in `TypeErasedBox` and fires to pipeline
- Handles results:
  - `Success`: Continue reading
  - `Backpressure`: Pause reading
  - `Error`: Close connection

### `readBufferAvailable(IOBuf)`
- Alternative callback when buffer is provided directly
- Same handling as `readDataAvailable`

### `readEOF()`
- Called when peer closes connection
- Fires `TTransportException::END_OF_FILE`

### `readErr(AsyncSocketException)`
- Called on read errors
- Converts to `TTransportException` and closes

---

## Write Path (Outbound)

### `write(BytesPtr bytes) -> Result`
- Writes bytes to underlying socket
- Returns immediately with:
  - `Success`: Write completed (rare for async)
  - `Backpressure`: Write pending, caller should wait
  - `Error`: Write failed

### `writeSuccess()`
- Called when async write completes
- Decrements pending write count
- Notifies pipeline via `onWriteReady()`

### `writeErr(size_t bytesWritten, AsyncSocketException)`
- Called on write failure
- Closes connection with error

---

## Flow Control

### `pauseRead()`
- Stops reading from socket
- Sets `socket_->setReadCB(nullptr)`
- Used when pipeline signals backpressure

### `resumeRead()`
- Resumes reading from socket
- Sets `socket_->setReadCB(this)`
- Called when pipeline can accept more data

---

## Connection Lifecycle

### `onConnect()`
- Called when socket is connected
- Resumes reading
- Fires `pipeline_->activate()`

### `onClose(exception_wrapper)`
- Called when transport should close
- Closes socket immediately
- Fires exception to pipeline
- Invokes close callback

### Close Callback

```cpp
transport->setCloseCallback([]() {
    // Connection closed, cleanup resources
});
```

---

## Internal State

| Field | Type | Description |
|-------|------|-------------|
| `socket_` | `AsyncTransport::UniquePtr` | Underlying socket |
| `readBufQueue_` | `IOBufQueue` | Read buffer management |
| `pipeline_` | `PipelineImpl*` | Pipeline reference |
| `pipelineGuard_` | `DestructorGuard` | Prevents pipeline destruction |
| `readPaused_` | `bool` | Read flow control state |
| `writePending_` | `uint32_t` | Pending write count |
| `closed_` | `bool` | Connection closed flag |
| `closeCallback_` | `Function<void()>` | Close notification |

---

## Testing

```bash
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/transport/test:transport_handler_test
```

---

## Benchmarks

```bash
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/transport/bench:transport_handler_bench
```

---

## Test Utilities

### TestAsyncTransport

Mock transport for unit testing (in `test/TestAsyncTransport.h`):
- Captures written data
- Injects read data
- Simulates errors and EOF

### BenchAsyncTransport

Benchmark transport (in `bench/BenchAsyncTransport.h`):
- Minimal overhead for benchmarking
- Captures/injects data without actual I/O

---

## Key Files

| File | Purpose |
|------|---------|
| `TransportHandler.h` | Main transport implementation |
| `test/TestAsyncTransport.h` | Mock transport for testing |
| `bench/BenchAsyncTransport.h` | Mock transport for benchmarks |
