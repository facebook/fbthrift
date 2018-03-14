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
#include <folly/Benchmark.h>

#include <thrift/lib/cpp2/frozen/FrozenUtil.h>
#include <thrift/lib/cpp2/frozen/HintTypes.h>
#include <thrift/lib/cpp2/frozen/test/gen-cpp2/Example_layouts.h>
#include <thrift/lib/cpp2/frozen/test/gen-cpp2/Example_types.h>

using namespace apache::thrift;
using namespace frozen;

const int kRows = 490;
const int kCols = 51;

template <class T, class Inner = std::vector<T>>
std::vector<Inner> makeMatrix() {
  std::vector<Inner> vals;
  for (int y = 0; y < kRows; ++y) {
    vals.emplace_back();
    auto& row = vals.back();
    for (int x = 0; x < kCols; ++x) {
      row.push_back(rand() % 12000 - 6000);
    }
  }
  return vals;
}

auto vvf32 = makeMatrix<float>();
auto fvvf32 = freeze(vvf32);
auto vvi16 = makeMatrix<int16_t>();
auto vvi32 = makeMatrix<int32_t>();
auto vvi64 = makeMatrix<int64_t>();
auto fvvi16 = freeze(vvi16);
auto fvvi32 = freeze(vvi32);
auto fvvi64 = freeze(vvi64);
auto fuvvi16 = freeze(makeMatrix<int16_t, VectorUnpacked<int16_t>>());
auto fuvvi32 = freeze(makeMatrix<int32_t, VectorUnpacked<int32_t>>());
auto fuvvi64 = freeze(makeMatrix<int64_t, VectorUnpacked<int64_t>>());
auto strings = std::vector<std::string>{"one", "two", "three", "four", "five"};
auto tuple = std::make_pair(std::make_pair(1.3, 2.4), std::make_pair(4.5, 'x'));

template <class F>
void benchmarkSum(int iters, const F& matrix) {
  int s = 0;
  while (iters--) {
    for (auto& row : matrix) {
      for (auto val : row) {
        s += val;
      }
    }
  }
  folly::doNotOptimizeAway(s);
}

template <class F>
void benchmarkSumCols(int iters, const F& matrix) {
  int s = 0;
  while (iters--) {
    for (size_t x = 0; x < kCols; ++x) {
      for (size_t y = x; y < kRows; ++y) {
        s += matrix[y][x];
      }
    }
  }
  folly::doNotOptimizeAway(s);
}

template <class F>
void benchmarkSumSavedCols(int iters, const F& matrix) {
  folly::BenchmarkSuspender setup;
  std::vector<typename F::value_type> rows;
  for (size_t y = 0; y < kRows; ++y) {
    rows.push_back(matrix[y]);
  }
  setup.dismiss();

  int s = 0;
  while (iters--) {
    for (size_t x = 0; x < kCols; ++x) {
      for (size_t y = x; y < kRows; ++y) {
        s += rows[y][x];
      }
    }
  }
  folly::doNotOptimizeAway(s);
}

BENCHMARK_PARAM(benchmarkSum, vvi16);
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvi16);
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fuvvi16);
BENCHMARK_PARAM(benchmarkSum, vvi32);
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvi32);
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fuvvi32);
BENCHMARK_PARAM(benchmarkSum, vvi64);
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvi64);
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fuvvi64);
BENCHMARK_PARAM(benchmarkSum, vvf32);
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvf32);
BENCHMARK_DRAW_LINE();
BENCHMARK_PARAM(benchmarkSumCols, vvi32);
BENCHMARK_RELATIVE_PARAM(benchmarkSumCols, fvvi32);
BENCHMARK_RELATIVE_PARAM(benchmarkSumCols, fuvvi32);
BENCHMARK_RELATIVE_PARAM(benchmarkSumSavedCols, fvvi32);
BENCHMARK_RELATIVE_PARAM(benchmarkSumSavedCols, fuvvi32);

const size_t entries = 100000;
template <class Map>
Map makeMap() {
  Map hist;
  for (size_t y = 0; y < entries * 3; ++y) {
    auto k = (rand() * 8192 + rand()) % entries;
    ++hist[k];
  }
  return hist;
}

auto hashMap_f32 = makeMap<std::unordered_map<float, int>>();
auto hashMap_i16 = makeMap<std::unordered_map<int16_t, int>>();
auto hashMap_i32 = makeMap<std::unordered_map<int32_t, int>>();
auto hashMap_i64 = makeMap<std::unordered_map<int64_t, int>>();
auto frozenHashMap_f32 = freeze(hashMap_f32);
auto frozenHashMap_i16 = freeze(hashMap_i16);
auto frozenHashMap_i32 = freeze(hashMap_i32);
auto frozenHashMap_i64 = freeze(hashMap_i64);
auto map_f32 = std::map<float, int>(hashMap_f32.begin(), hashMap_f32.end());
auto map_i16 = std::map<int16_t, int>(hashMap_i16.begin(), hashMap_i16.end());
auto map_i32 = std::map<int32_t, int>(hashMap_i32.begin(), hashMap_i32.end());
auto map_i64 = std::map<int64_t, int>(hashMap_i64.begin(), hashMap_i64.end());
auto frozenMap_f32 = freeze(map_f32);
auto frozenMap_i16 = freeze(map_i16);
auto frozenMap_i32 = freeze(map_i32);
auto frozenMap_i64 = freeze(map_i64);

template <class Map>
void benchmarkLookup(int iters, const Map& hist) {
  int s = 0;
  while (iters--) {
    for (int n = 0; n < 1000; ++n) {
      auto found = hist.find(rand());
      if (found != hist.end()) {
        s += found->second();
      }
    }
  }
  folly::doNotOptimizeAway(s);
}

template <class T>
void benchmarkLookup(int iters, const std::map<T, int>& hist) {
  int s = 0;
  while (iters--) {
    for (int n = 0; n < 1000; ++n) {
      auto k = (rand() * 8192 + rand()) % entries;
      auto found = hist.find(k);
      if (found != hist.end()) {
        s += found->second;
      }
    }
  }
  folly::doNotOptimizeAway(s);
}

template <class T>
void benchmarkLookup(int iters, const std::unordered_map<T, int>& hist) {
  int s = 0;
  while (iters--) {
    for (int n = 0; n < 1000; ++n) {
      auto k = (rand() * 8192 + rand()) % entries;
      auto found = hist.find(k);
      if (found != hist.end()) {
        s += found->second;
      }
    }
  }
  folly::doNotOptimizeAway(s);
}

BENCHMARK_DRAW_LINE();

BENCHMARK_PARAM(benchmarkLookup, map_f32);
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenMap_f32);
BENCHMARK_PARAM(benchmarkLookup, map_i16);
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenMap_i16);
BENCHMARK_PARAM(benchmarkLookup, map_i32);
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenMap_i32);
BENCHMARK_PARAM(benchmarkLookup, map_i64);
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenMap_i64);

BENCHMARK_DRAW_LINE();

BENCHMARK_PARAM(benchmarkLookup, hashMap_f32);
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenHashMap_f32);
BENCHMARK_PARAM(benchmarkLookup, hashMap_i16);
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenHashMap_i16);
BENCHMARK_PARAM(benchmarkLookup, hashMap_i32);
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenHashMap_i32);
BENCHMARK_PARAM(benchmarkLookup, hashMap_i64);
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenHashMap_i64);

BENCHMARK_DRAW_LINE();

template <class T>
void benchmarkOldFreezeDataToString(int iters, const T& data) {
  const auto layout = maximumLayout<T>();
  size_t s = 0;
  while (iters--) {
    std::string out;
    out.resize(frozenSize(data, layout));
    folly::MutableByteRange writeRange(
        reinterpret_cast<byte*>(&out[0]), out.size());
    ByteRangeFreezer::freeze(layout, data, writeRange);
    out.resize(out.size() - writeRange.size());
    s += out.size();
  }
  folly::doNotOptimizeAway(s);
}

template <class T>
void benchmarkFreezeDataToString(int iters, const T& data) {
  const auto layout = maximumLayout<T>();
  size_t s = 0;
  while (iters--) {
    s += freezeDataToString(data, layout).size();
  }
  folly::doNotOptimizeAway(s);
}

BENCHMARK_PARAM(benchmarkFreezeDataToString, vvf32);
BENCHMARK_RELATIVE_PARAM(benchmarkOldFreezeDataToString, vvf32);
BENCHMARK_PARAM(benchmarkFreezeDataToString, tuple);
BENCHMARK_RELATIVE_PARAM(benchmarkOldFreezeDataToString, tuple);
BENCHMARK_PARAM(benchmarkFreezeDataToString, strings);
BENCHMARK_RELATIVE_PARAM(benchmarkOldFreezeDataToString, strings);
BENCHMARK_PARAM(benchmarkFreezeDataToString, hashMap_i32);
BENCHMARK_RELATIVE_PARAM(benchmarkOldFreezeDataToString, hashMap_i32);

#if 0
============================================================================
thrift/lib/cpp2/frozen/test/FrozenBench.cpp     relative  time/iter  iters/s
============================================================================
benchmarkSum(vvi16)                                          4.13us  241.93K
benchmarkSum(fvvi16)                               3.80%   108.74us    9.20K
benchmarkSum(fuvvi16)                             48.05%     8.60us  116.24K
benchmarkSum(vvi32)                                          4.70us  212.83K
benchmarkSum(fvvi32)                               4.35%   107.97us    9.26K
benchmarkSum(fuvvi32)                             55.06%     8.53us  117.19K
benchmarkSum(vvi64)                                         10.83us   92.34K
benchmarkSum(fvvi64)                              14.84%    72.98us   13.70K
benchmarkSum(fuvvi64)                             65.67%    16.49us   60.64K
benchmarkSum(vvf32)                                         69.91us   14.30K
benchmarkSum(fvvf32)                              99.85%    70.02us   14.28K
----------------------------------------------------------------------------
benchmarkSumCols(vvi32)                                     14.90us   67.11K
benchmarkSumCols(fvvi32)                           6.75%   220.84us    4.53K
benchmarkSumCols(fuvvi32)                         10.51%   141.85us    7.05K
benchmarkSumSavedCols(fvvi32)                     15.29%    97.46us   10.26K
benchmarkSumSavedCols(fuvvi32)                   124.74%    11.95us   83.70K
----------------------------------------------------------------------------
benchmarkLookup(map_f32)                                   241.65us    4.14K
benchmarkLookup(frozenMap_f32)                   423.24%    57.09us   17.51K
benchmarkLookup(map_i16)                                   212.87us    4.70K
benchmarkLookup(frozenMap_i16)                    85.52%   248.91us    4.02K
benchmarkLookup(map_i32)                                   229.78us    4.35K
benchmarkLookup(frozenMap_i32)                   171.00%   134.37us    7.44K
benchmarkLookup(map_i64)                                   231.73us    4.32K
benchmarkLookup(frozenMap_i64)                   205.83%   112.58us    8.88K
----------------------------------------------------------------------------
benchmarkLookup(hashMap_f32)                                88.28us   11.33K
benchmarkLookup(frozenHashMap_f32)               220.12%    40.11us   24.93K
benchmarkLookup(hashMap_i16)                                51.96us   19.24K
benchmarkLookup(frozenHashMap_i16)               108.97%    47.68us   20.97K
benchmarkLookup(hashMap_i32)                                48.50us   20.62K
benchmarkLookup(frozenHashMap_i32)               133.37%    36.37us   27.50K
benchmarkLookup(hashMap_i64)                                51.68us   19.35K
benchmarkLookup(frozenHashMap_i64)                61.69%    83.77us   11.94K
============================================================================
#endif

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  folly::runBenchmarks();
  return 0;
}
