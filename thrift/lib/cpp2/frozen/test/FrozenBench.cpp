/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <folly/Benchmark.h>
#include <folly/Random.h>
#include <thrift/lib/cpp2/frozen/FrozenUtil.h>
#include <thrift/lib/cpp2/frozen/HintTypes.h>
#include <thrift/lib/cpp2/frozen/test/gen-cpp2/Example_layouts.h>
#include <thrift/lib/cpp2/frozen/test/gen-cpp2/Example_types.h>

using namespace apache::thrift;
using namespace frozen;

constexpr int kRows = 490;
constexpr int kCols = 51;

template <class T, class Inner = std::vector<T>>
std::vector<Inner> makeMatrix() {
  std::vector<Inner> vals;
  for (int y = 0; y < kRows; ++y) {
    vals.emplace_back();
    auto& row = vals.back();
    for (int x = 0; x < kCols; ++x) {
      row.push_back(folly::Random::rand32(12000) - 6000);
    }
  }
  return vals;
}

template <class F>
void benchmarkSum(size_t iters, const F& matrix) {
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
void benchmarkSumCols(size_t iters, const F& matrix) {
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
void benchmarkSumSavedCols(size_t iters, const F& matrix) {
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

auto vvi16 = makeMatrix<int16_t>();
auto vvi32 = makeMatrix<int32_t>();
auto vvi64 = makeMatrix<int64_t>();
auto fvvi16 = freeze(vvi16);
auto fvvi32 = freeze(vvi32);
auto fvvi64 = freeze(vvi64);
auto fuvvi16 = freeze(makeMatrix<int16_t, VectorUnpacked<int16_t>>());
auto fuvvi32 = freeze(makeMatrix<int32_t, VectorUnpacked<int32_t>>());
auto fuvvi64 = freeze(makeMatrix<int64_t, VectorUnpacked<int64_t>>());
auto vvf32 = makeMatrix<float>();
auto fvvf32 = freeze(vvf32);

BENCHMARK_PARAM(benchmarkSum, vvi16)
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvi16)
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fuvvi16)
BENCHMARK_PARAM(benchmarkSum, vvi32)
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvi32)
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fuvvi32)
BENCHMARK_PARAM(benchmarkSum, vvi64)
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvi64)
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fuvvi64)
BENCHMARK_PARAM(benchmarkSum, vvf32)
BENCHMARK_RELATIVE_PARAM(benchmarkSum, fvvf32)
BENCHMARK_DRAW_LINE();
BENCHMARK_PARAM(benchmarkSumCols, vvi32)
BENCHMARK_RELATIVE_PARAM(benchmarkSumCols, fvvi32)
BENCHMARK_RELATIVE_PARAM(benchmarkSumCols, fuvvi32)
BENCHMARK_RELATIVE_PARAM(benchmarkSumSavedCols, fvvi32)
BENCHMARK_RELATIVE_PARAM(benchmarkSumSavedCols, fuvvi32)

constexpr size_t kEntries = 1000000;
constexpr size_t kChunkSize = 1000;

template <class K>
K makeKey() {
  return folly::to<K>(folly::Random::rand32(kEntries));
}

template <class K>
const std::vector<K>& makeKeys() {
  // static to reuse storage across iterations.
  static std::vector<K> keys(kChunkSize);
  for (auto& key : keys) {
    key = makeKey<K>();
  }
  return keys;
}

template <class Map>
Map makeMap() {
  Map hist;
  using K = typename Map::key_type;
  for (size_t y = 0; y < kEntries; ++y) {
    ++hist[makeKey<K>()];
  }
  return hist;
}

template <class K>
using OwnedKey = typename std::conditional<
    std::is_same<K, folly::StringPiece>::value,
    std::string,
    K>::type;

template <class Map>
void benchmarkLookup(size_t iters, const Map& map) {
  using K = OwnedKey<typename Map::key_type>;
  int s = 0;
  for (;;) {
    folly::BenchmarkSuspender setup;
    auto& keys = makeKeys<K>();
    setup.dismiss();

    for (auto& key : keys) {
      if (iters-- == 0) {
        folly::doNotOptimizeAway(s);
        return;
      }
      auto found = map.find(key);
      if (found != map.end()) {
        ++s;
      }
    }
  }
  folly::doNotOptimizeAway(s);
}

BENCHMARK_DRAW_LINE();

auto hashMap_f32 = makeMap<std::unordered_map<float, int>>();
auto hashMap_i32 = makeMap<std::unordered_map<int32_t, int>>();
auto hashMap_i64 = makeMap<std::unordered_map<int64_t, int>>();
auto hashMap_str = makeMap<std::unordered_map<std::string, int>>();
auto frozenHashMap_f32 = freeze(hashMap_f32);
auto frozenHashMap_i32 = freeze(hashMap_i32);
auto frozenHashMap_i64 = freeze(hashMap_i64);
auto frozenHashMap_str = freeze(hashMap_str);

BENCHMARK_PARAM(benchmarkLookup, hashMap_f32)
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenHashMap_f32)
BENCHMARK_PARAM(benchmarkLookup, hashMap_i32)
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenHashMap_i32)
BENCHMARK_PARAM(benchmarkLookup, hashMap_i64)
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenHashMap_i64)
BENCHMARK_PARAM(benchmarkLookup, hashMap_str)
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenHashMap_str)

BENCHMARK_DRAW_LINE();

auto map_f32 = std::map<float, int>(hashMap_f32.begin(), hashMap_f32.end());
auto map_i32 = std::map<int32_t, int>(hashMap_i32.begin(), hashMap_i32.end());
auto map_i64 = std::map<int64_t, int>(hashMap_i64.begin(), hashMap_i64.end());
auto frozenMap_f32 = freeze(map_f32);
auto frozenMap_i32 = freeze(map_i32);
auto frozenMap_i64 = freeze(map_i64);

BENCHMARK_PARAM(benchmarkLookup, map_f32)
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenMap_f32)
BENCHMARK_PARAM(benchmarkLookup, map_i32)
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenMap_i32)
BENCHMARK_PARAM(benchmarkLookup, map_i64)
BENCHMARK_RELATIVE_PARAM(benchmarkLookup, frozenMap_i64)

BENCHMARK_DRAW_LINE();

template <class T>
void benchmarkOldFreezeDataToString(size_t iters, const T& data) {
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
void benchmarkFreezeDataToString(size_t iters, const T& data) {
  const auto layout = maximumLayout<T>();
  size_t s = 0;
  while (iters--) {
    s += freezeDataToString(data, layout).size();
  }
  folly::doNotOptimizeAway(s);
}

auto strings = std::vector<std::string>{"one", "two", "three", "four", "five"};
auto tuple = std::make_pair(std::make_pair(1.3, 2.4), std::make_pair(4.5, 'x'));

BENCHMARK_PARAM(benchmarkFreezeDataToString, vvf32)
BENCHMARK_RELATIVE_PARAM(benchmarkOldFreezeDataToString, vvf32)
BENCHMARK_PARAM(benchmarkFreezeDataToString, tuple)
BENCHMARK_RELATIVE_PARAM(benchmarkOldFreezeDataToString, tuple)
BENCHMARK_PARAM(benchmarkFreezeDataToString, strings)
BENCHMARK_RELATIVE_PARAM(benchmarkOldFreezeDataToString, strings)
BENCHMARK_PARAM(benchmarkFreezeDataToString, hashMap_i32)
BENCHMARK_RELATIVE_PARAM(benchmarkOldFreezeDataToString, hashMap_i32)

#if 0
============================================================================
thrift/lib/cpp2/frozen/test/FrozenBench.cpp     relative  time/iter  iters/s
============================================================================
benchmarkSum(vvi16)                                          4.93us  203.01K
benchmarkSum(fvvi16)                               3.86%   127.70us    7.83K
benchmarkSum(fuvvi16)                             49.45%     9.96us  100.39K
benchmarkSum(vvi32)                                          5.76us  173.51K
benchmarkSum(fvvi32)                               4.51%   127.72us    7.83K
benchmarkSum(fuvvi32)                             55.70%    10.35us   96.64K
benchmarkSum(vvi64)                                         13.80us   72.48K
benchmarkSum(fvvi64)                               7.37%   187.20us    5.34K
benchmarkSum(fuvvi64)                             75.72%    18.22us   54.88K
benchmarkSum(vvf32)                                         83.52us   11.97K
benchmarkSum(fvvf32)                              99.54%    83.90us   11.92K
----------------------------------------------------------------------------
benchmarkSumCols(vvi32)                                     35.23us   28.38K
benchmarkSumCols(fvvi32)                          12.49%   282.07us    3.55K
benchmarkSumCols(fuvvi32)                         18.26%   192.99us    5.18K
benchmarkSumSavedCols(fvvi32)                     32.34%   108.94us    9.18K
benchmarkSumSavedCols(fuvvi32)                   194.45%    18.12us   55.19K
----------------------------------------------------------------------------
benchmarkLookup(hashMap_f32)                                64.58ns   15.49M
benchmarkLookup(frozenHashMap_f32)               132.46%    48.75ns   20.51M
benchmarkLookup(hashMap_i32)                                21.59ns   46.31M
benchmarkLookup(frozenHashMap_i32)                57.54%    37.52ns   26.65M
benchmarkLookup(hashMap_i64)                                26.85ns   37.24M
benchmarkLookup(frozenHashMap_i64)                77.91%    34.47ns   29.01M
benchmarkLookup(hashMap_str)                               122.18ns    8.18M
benchmarkLookup(frozenHashMap_str)               128.36%    95.18ns   10.51M
----------------------------------------------------------------------------
benchmarkLookup(map_f32)                                   352.68ns    2.84M
benchmarkLookup(frozenMap_f32)                   177.46%   198.74ns    5.03M
benchmarkLookup(map_i32)                                   347.51ns    2.88M
benchmarkLookup(frozenMap_i32)                   110.00%   315.92ns    3.17M
benchmarkLookup(map_i64)                                   344.25ns    2.90M
benchmarkLookup(frozenMap_i64)                   123.47%   278.80ns    3.59M
----------------------------------------------------------------------------
benchmarkFreezeDataToString(vvf32)                         107.88us    9.27K
benchmarkOldFreezeDataToString(vvf32)             82.78%   130.31us    7.67K
benchmarkFreezeDataToString(tuple)                         233.81ns    4.28M
benchmarkOldFreezeDataToString(tuple)            236.93%    98.68ns   10.13M
benchmarkFreezeDataToString(strings)                       839.63ns    1.19M
benchmarkOldFreezeDataToString(strings)          297.64%   282.10ns    3.54M
benchmarkFreezeDataToString(hashMap_i32)                    41.09ms    24.34
benchmarkOldFreezeDataToString(hashMap_i32)       43.27%    94.95ms    10.53
============================================================================
#endif

int main(int argc, char** argv) {
  google::InitGoogleLogging(argv[0]);
  gflags::ParseCommandLineFlags(&argc, &argv, true);
  folly::runBenchmarks();
  return 0;
}
