/*
 * Copyright 2014 Facebook, Inc.
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

#include "folly/Benchmark.h"
#include "thrift/lib/cpp2/frozen/test/gen-cpp2/Example_types.h"
#include "thrift/lib/cpp2/frozen/test/gen-cpp2/Example_layouts.h"

using namespace apache::thrift;
using namespace frozen;

template <class T>
std::vector<std::vector<T>> makeMatrix() {
  std::vector<std::vector<T>> vals;
  for (int y = 0; y < 4096; ++y) {
    vals.emplace_back();
    auto& row = vals.back();
    for (int x = 0; x < 512; ++x) {
      row.push_back(rand() % 12000 - 6000);
    }
  }
  return vals;
}

auto vvf32 = makeMatrix<float>();
auto vvi16 = makeMatrix<int16_t>();
auto vvi32 = makeMatrix<int32_t>();
auto vvi64 = makeMatrix<int64_t>();
auto fvvf32 = freeze(vvf32);
auto fvvi16 = freeze(vvi16);
auto fvvi32 = freeze(vvi32);
auto fvvi64 = freeze(vvi64);

template <class T>
void benchmarkSum(int iters, const std::vector<std::vector<T>>& matrix) {
  int s = 0;
  while (iters--)
    for (auto& row : matrix)
      for (auto val : row)
        s += val;
  folly::doNotOptimizeAway(s);
}

template <class F>
void benchmarkSum(int iters, const F& matrix) {
  int s = 0;
  while (iters--)
    for (auto row : matrix)
      for (auto val : row)
        s += val;
  folly::doNotOptimizeAway(s);
}

BENCHMARK_PARAM(benchmarkSum, vvi16);
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvi16);
BENCHMARK_PARAM(benchmarkSum, vvi32);
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvi32);
BENCHMARK_PARAM(benchmarkSum, vvi64);
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvi64);
BENCHMARK_PARAM(benchmarkSum, vvf32);
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvf32);

const size_t entries = 1000000;
template <class Map>
Map makeMap() {
  Map hist;
  for (int y = 0; y < 10000000; ++y) {
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

#if 0
============================================================================
thrift/lib/cpp2/frozen/test/FrozenBench.cpp     relative  time/iter  iters/s
============================================================================
benchmarkSum(vvi16)                                        560.87us    1.78K
benchmarkSum(fvvi16)                               3.11%    18.04ms    55.43
benchmarkSum(vvi32)                                        631.93us    1.58K
benchmarkSum(fvvi32)                               4.38%    14.42ms    69.37
benchmarkSum(vvi64)                                          1.85ms   539.33
benchmarkSum(fvvi64)                              18.01%    10.29ms    97.15
benchmarkSum(vvf32)                                         10.52ms    95.04
benchmarkSum(fvvf32)                              99.75%    10.55ms    94.80
----------------------------------------------------------------------------
benchmarkLookup(map_f32)                                   297.68us    3.36K
benchmarkLookup(frozenMap_f32)                   141.90%   209.78us    4.77K
benchmarkLookup(map_i16)                                   251.54us    3.98K
benchmarkLookup(frozenMap_i16)                    77.51%   324.54us    3.08K
benchmarkLookup(map_i32)                                   288.40us    3.47K
benchmarkLookup(frozenMap_i32)                    85.67%   336.62us    2.97K
benchmarkLookup(map_i64)                                   290.03us    3.45K
benchmarkLookup(frozenMap_i64)                    88.82%   326.54us    3.06K
----------------------------------------------------------------------------
benchmarkLookup(hashMap_f32)                               100.01us   10.00K
benchmarkLookup(frozenHashMap_f32)               138.78%    72.06us   13.88K
benchmarkLookup(hashMap_i16)                                70.05us   14.28K
benchmarkLookup(frozenHashMap_i16)               107.99%    64.87us   15.42K
benchmarkLookup(hashMap_i32)                                63.55us   15.74K
benchmarkLookup(frozenHashMap_i32)                89.94%    70.65us   14.15K
benchmarkLookup(hashMap_i64)                                66.91us   14.95K
benchmarkLookup(frozenHashMap_i64)                94.22%    71.01us   14.08K
============================================================================
#endif
int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);
  folly::runBenchmarks();
  return 0;
}
