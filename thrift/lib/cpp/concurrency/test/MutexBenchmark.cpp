/*
 * Copyright 2014-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <thrift/lib/cpp/concurrency/Mutex.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <gflags/gflags.h>

#include <folly/Benchmark.h>

DEFINE_int32(num_threads, 32, "Number of threads to run concurrency "
                              "benchmarks");
DEFINE_int32(malloc_contention_bytes, 1024 * 1024,
             "Size of a large malloc to test effects of contention in malloc");
DEFINE_int32(malloc_contention_time_us, 1, "How long to call malloc in loop");
DEFINE_int32(spin_us, 0, "Time in us to spin with lock");

typedef apache::thrift::concurrency::Mutex ThriftMutex;

template <class T>
void mallocContentionTest(int64_t iters, int64_t mallocTime) {
  char* buf = nullptr;
  for (int i = 0; i < iters; ++i) {
    T m;
    std::lock_guard<T> g(m);
    if (mallocTime) {
      auto now = std::chrono::system_clock::now();
      auto end = now + std::chrono::microseconds {mallocTime};
      do {
        now = std::chrono::system_clock::now();
        if (buf) {
          delete[] buf;
        }
        buf = new char[FLAGS_malloc_contention_bytes];
      } while (now < end);
    }
  }
}

template <class T>
void grabLockNTimes(T& m, int64_t iters) {
  for (int i = 0; i < iters; ++i) {
    std::lock_guard<T> g(m);
    if (FLAGS_spin_us > 0) {
      auto now = std::chrono::system_clock::now();
      auto end = now + std::chrono::microseconds {FLAGS_spin_us};
      do {
        now = std::chrono::system_clock::now();
      } while (now < end);
    }
  }
}

static void runConcurrently(int64_t numThreads, std::function<void ()> fn) {
  std::atomic<bool> go(false);
  std::vector<std::thread> threads;
  BENCHMARK_SUSPEND {
    for (int t = 0; t < numThreads; ++t) {
      threads.emplace_back(fn);
    }
  }
  go.store(true);
  for (auto& t : threads) {
    t.join();
  }
}

/*
 * Thrift's Mutex uses malloc to instantiate an impl. Test the cost of that with
 * and without contention in jemalloc
 */
BENCHMARK(thrift_construct_and_lock, iters) {
  mallocContentionTest<ThriftMutex>(iters, 0);
}

BENCHMARK_RELATIVE(std_construct_and_lock, iters) {
  mallocContentionTest<std::mutex>(iters, 0);
}

BENCHMARK_DRAW_LINE();

BENCHMARK(thrift_construct_and_lock_with_malloc_pressure, iters) {
  auto fn = [=]() {
    mallocContentionTest<ThriftMutex>(iters, FLAGS_malloc_contention_time_us);
  };
  runConcurrently(FLAGS_num_threads, fn);
}

BENCHMARK_RELATIVE(std_construct_and_lock_with_malloc_pressure, iters) {
  auto fn = [=]() {
    mallocContentionTest<std::mutex>(iters, FLAGS_malloc_contention_time_us);
  };
  runConcurrently(FLAGS_num_threads, fn);
}

BENCHMARK_DRAW_LINE();

/*
 * Thrift's Mutex grabs timers for stats collection, and uses virtual methods.
 * It also uses pthread mutex instead of std::mutex. Measure the overhead of
 * grabbing a lock.
 */
BENCHMARK(thrift_uncontended, iters) {
  ThriftMutex m;
  grabLockNTimes(m, iters);
}

BENCHMARK_RELATIVE(std_uncontended, iters) {
  std::mutex m;
  grabLockNTimes(m, iters);
}

BENCHMARK_DRAW_LINE();

/*
 * See if there is any difference in the contended case.
 */
BENCHMARK(thrift_contended, iters) {
  ThriftMutex m;
  runConcurrently(FLAGS_num_threads,
                  [&m, iters]() { grabLockNTimes(m, iters); });
}

BENCHMARK_RELATIVE(std_contended, iters) {
  std::mutex m;
  runConcurrently(FLAGS_num_threads,
                  [&m, iters]() { grabLockNTimes(m, iters); });
}

int main(int argc, char *argv[]) {
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  folly::runBenchmarks();
  return 0;
}

/*
Interpretting results of this benchmark can be tricky since it is multithreaded
with spins. Looking at relative % is probably not useful. Instead, compare raw
time/iter values - whatever the difference is represents the overhead of the
thrift mutex.

Consider the following set of results:

_bin/thrift/lib/cpp/concurrency/test/mutex_benchmark --bm_min_iters=10000 --spin_us=5 --num_threads=200
============================================================================
thrift/lib/cpp/concurrency/test/MutexBenchmark.cpprelative  time/iter  iters/s
============================================================================
thrift_construct_and_lock                                  101.04ns    9.90M
std_construct_and_lock                           383.67%    26.33ns   37.97M
----------------------------------------------------------------------------
thrift_construct_and_lock_with_malloc_pressure              33.89us   29.51K
std_construct_and_lock_with_mallo_pressure       329.67%    10.28us   97.28K
----------------------------------------------------------------------------
thrift_uncontended                                           5.11us  195.75K
std_uncontended                                  100.08%     5.10us  195.91K
----------------------------------------------------------------------------
thrift_contended                                             1.35ms   740.82
std_contended                                    104.39%     1.29ms   773.35
============================================================================

This suggested that with memory pressure, the cost of constructing and locking a
thrift mutex is over 23us larger than the cost of a std::mutex. As a relative
percentage, this is actually larger than the 329.67% reported.
*/
