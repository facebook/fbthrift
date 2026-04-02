---
description: Rules for working with fast_thrift - the most performant Thrift transport
apply_to_regex: "fbcode/thrift/lib/cpp2/fast_thrift/.*"
oncalls:
  - thrift
---

# fast_thrift Development Rules

> These rules are automatically applied when working in the fast_thrift directory.

## ⚠️ MANDATORY: Load the Overview Skill First

**BEFORE doing ANY work in fast_thrift** (code changes, answering questions, debugging, optimizing):

```
skill fast_thrift_overview.md
```

Then load the specific skill for your task:

| Task | Skill to Load |
|------|---------------|
| Understanding/modifying pipeline | `channel_pipeline/.llms/skills/channel_pipeline.md` |
| Adding new message types | `channel_pipeline/.llms/skills/type_erasure.md` |
| Implementing handlers | `channel_pipeline/.llms/skills/handler.md` |
| Working with frames | `frame/.llms/skills/framing.md` |
| Stream management, Rocket protocol | `rocket/.llms/skills/rocket.md` |
| Running/writing benchmarks | `.llms/skills/common/benchmarking.md` |

---

## Critical Constraints (Always Remember)

1. **TypeErasedBox limit**: 120 bytes (compile error in debug, **silent UB in opt!**)
2. **Run benchmarks, don't theorize**: When optimizing or answering perf questions, execute the benchmarks
3. **Every component needs**: unit tests + benchmarks (where meaningful)

---

## Quick Reference

### Design Philosophy

- **Zero virtualization** — No virtual functions in the hot path
- **Composable by design** — Users plug in handlers as needed
- **Parse once, access many** — Headers cached on first parse
- **Inline everything** — Messages ≤120 bytes fit in `TypeErasedBox`
- **Move, never copy** — Zero-copy semantics throughout

### Key Commands

```bash
# Run handler microbenchmark
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/.../bench:*_bench

# Run integration benchmark
buck2 run @//mode/opt-clang-lto fbcode//thrift/lib/cpp2/fast_thrift/rocket/client/bench:rocket_client_integration_bench

# Perf harness
cd fbcode/thrift/lib/cpp2/fast_thrift/bench
./perf_harness.sh baseline
./perf_harness.sh run my_optimization

# E2E benchmark
buck2 run fbcode//thrift/lib/cpp2/fast_thrift/bench:benchmark_server -- --port=5001
buck2 run fbcode//thrift/lib/cpp2/fast_thrift/bench:benchmark_client -- --port=5001 --client_threads=8

# Run tests
buck2 test fbcode//thrift/lib/cpp2/fast_thrift/...
```
