---
description: Benchmarking guide for fast_thrift - microbenchmarks, integration benchmarks, E2E benchmarks, and perf harness
oncalls:
  - thrift
---

# fast_thrift Benchmarking

## RUN BENCHMARKS — DON'T THEORIZE

**When optimizing or answering performance questions:**

1. **RUN the benchmarks yourself** — don't speculate about what "should" be faster
2. **Use perf_harness** to see WHERE improvements/regressions come from
3. **Let the data answer the question** — cycles, IPC, cache misses don't lie

```bash
# Before making optimization claims, ALWAYS run:
buck2 run @//mode/opt-clang-lto //path/to:benchmark

# For hardware-level insight:
./perf_harness.sh baseline
# ... make changes ...
./perf_harness.sh run my_optimization
```

**Theory is cheap. Data is truth.**

---

## Benchmark Types

### 1. Microbenchmarks (Handler-Level)

Located in `component/bench/` or `component/handler/bench/`. Uses **folly::Benchmark** with mock contexts.

**Purpose:** Measure individual handler overhead in isolation.

**Pattern:**
```cpp
class BenchContext {
 public:
  Result fireWrite(TypeErasedBox&& msg) noexcept {
    lastWriteMsg_ = std::move(msg);
    return Result::Success;
  }
  Result fireRead(TypeErasedBox&& msg) noexcept {
    lastReadMsg_ = std::move(msg);
    return Result::Success;
  }
  void fireException(folly::exception_wrapper&&) noexcept {}
};

BENCHMARK(MyHandler_OnWrite, iters) {
  folly::BenchmarkSuspender suspender;
  MyHandler handler;
  BenchContext ctx;

  std::vector<MyMessage> messages;
  messages.reserve(iters);
  for (size_t i = 0; i < iters; ++i) {
    messages.push_back(createTestMessage(i));
  }

  suspender.dismiss();  // Start timing

  for (size_t i = 0; i < iters; ++i) {
    auto result = handler.onWrite(ctx, erase_and_box(std::move(messages[i])));
    folly::doNotOptimizeAway(result);
  }
}
```

**Run:**
```bash
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/handler/bench:rocket_client_request_response_frame_handler_bench
```

### 2. Integration Microbenchmarks (Pipeline-Level)

Located in `component/bench/`. Uses **folly::Benchmark** with real pipeline + mock transport.

**Purpose:** Measure full pipeline traversal overhead without network I/O.

**Run:**
```bash
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/bench:rocket_client_integration_bench
```

### 3. E2E Benchmarks (Client/Server)

Located in `fast_thrift/bench/`. Uses **real TCP connections** with dedicated client/server binaries.

**Purpose:** Measure true QPS and latency with actual network I/O, multiple threads, concurrent connections.

**Run:**
```bash
# Terminal 1: Start server
buck2 run fbcode//thrift/lib/cpp2/fast_thrift/bench:benchmark_server -- --port=5001 --io_threads=8

# Terminal 2: Run client
buck2 run fbcode//thrift/lib/cpp2/fast_thrift/bench:benchmark_client -- --port=5001 --client_threads=8 --runtime_s=30
```

### 4. Perf Harness (Optimization Tracking)

Located in `fast_thrift/bench/perf_harness.sh`. Automates **perf stat** collection.

**Purpose:** Track optimization progress across multiple phases with hardware-level metrics.

**Commands:**
```bash
./perf_harness.sh baseline                    # Capture baseline
./perf_harness.sh run phase_1_my_optimization # After optimization
./perf_harness.sh compare baseline phase_1    # Compare two phases
./perf_harness.sh history                     # See cumulative progress
```

---

## Choosing the Right Benchmark

| Benchmark Type | Use When |
|----------------|----------|
| **Microbenchmark** | Testing single handler overhead, A/B comparing implementations |
| **Integration Microbench** | Testing full pipeline without network noise |
| **E2E Benchmark** | Measuring real-world throughput, finding system bottlenecks |
| **Perf Harness** | Hardware-level metrics (cycles, IPC, cache) |

---

## Filtering Benchmark Noise

### Sources of Noise

| Source | Impact | Mitigation |
|--------|--------|------------|
| CPU frequency scaling | ±20% variance | Use `opt-clang-lto`, run multiple times |
| Background processes | Sporadic spikes | Dedicated hardware, kill unnecessary processes |
| Memory allocator state | First-run penalty | Warm-up iterations |
| Compiler optimizations | Dead code elimination | `folly::doNotOptimizeAway()` |

### Best Practices

```cpp
BENCHMARK(MyBenchmark, iters) {
  folly::BenchmarkSuspender suspender;  // Pause timing

  // Setup: allocate data BEFORE timing starts
  std::vector<MyMessage> messages;
  messages.reserve(iters);
  for (size_t i = 0; i < iters; ++i) {
    messages.push_back(createTestMessage(i));
  }

  suspender.dismiss();  // Start timing HERE

  for (size_t i = 0; i < iters; ++i) {
    auto result = handler.onWrite(ctx, erase_and_box(std::move(messages[i])));
    folly::doNotOptimizeAway(result);  // Prevent dead code elimination
  }
}
```

### When to Trust Results

| Observed Change | Confidence | Action |
|-----------------|------------|--------|
| < 1% | Noise | Ignore |
| 1-5% | Low | Run more iterations, check multiple benchmarks |
| 5-10% | Medium | Verify with perf harness, check IPC |
| > 10% | High | Trust, but still verify E2E |

---

## Optimization Validation Checklist

**Every optimization MUST be validated.**

1. **ALWAYS identify the right validation approach:**

   | Optimization Target | Validation Approach |
   |---------------------|---------------------|
   | Single handler hot path | Microbenchmark (handler-level) |
   | Pipeline traversal overhead | Integration microbenchmark |
   | End-to-end throughput/latency | E2E benchmark suite |
   | Hardware-level (cycles, cache, IPC) | Perf harness |

2. **Capture before/after measurements:**
   ```bash
   ./perf_harness.sh baseline
   # ... make optimization ...
   ./perf_harness.sh run phase_N_description
   ```

3. **Document the validation** in the diff summary

### Red Flags (Stop and Validate)

- ❌ "This should be faster" without measurements
- ❌ Optimization only validated with one benchmark type
- ❌ No before/after comparison
- ❌ Microbenchmark shows huge gains but E2E shows nothing

### Cross-Validation Checklist

- [ ] **Microbenchmark** shows improvement
- [ ] **Multiple runs** show consistent results
- [ ] **Perf harness** confirms cycles reduction
- [ ] **IPC** stayed same or improved
- [ ] **E2E benchmark** shows measurable improvement (or at least no regression)
