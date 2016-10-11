/*
 * Copyright 2016 Facebook, Inc.
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

#include <thrift/lib/cpp2/util/MapCopy.h>

#include <folly/Benchmark.h>
#include <folly/Foreach.h>

#include <gtest/gtest.h>

#include <fatal/test/random_data.h>

#include <algorithm>
#include <map>
#include <vector>
#include <unordered_set>

using pair_vect = std::vector<std::pair<std::string, std::string>>;
using folly::BenchmarkSuspender;

void get_strings(
  pair_vect& data,
  std::size_t n
) {
  data = pair_vect();

  fatal::random_data r;
  std::unordered_set<std::string> seen;

  data.reserve(n);
  seen.reserve(n);

  while (seen.size() < n) {
    auto const size = r() % 100;
    auto key = r.string(size);
    if(seen.insert(key).second) {
      data.emplace_back(key, r.string(size));
    }
  }

  assert(seen.size() == n);
  assert(data.size() == n);

  std::sort(data.begin(), data.end());
}

template <typename BenchFunc>
std::size_t do_bench(
  const pair_vect& use_data,
  BenchmarkSuspender& braces,
  BenchFunc const& func
) {
  std::map<std::string, std::string> m;
  pair_vect data = use_data;

  braces.dismissing([&]() {
    func(m, data);
  });

  assert(m.size() == data.size());
  return m.size();
}

#define BODY_SETUP(size)     \
  BenchmarkSuspender braces; \
  static bool data_set_up;  \
  static pair_vect data;    \
  if(!data_set_up) { \
    data_set_up = true; \
    get_strings(data, size); \
  }

#define BODY_SORTED(size) \
  BODY_SETUP(size) \
  while(iters--) { \
    auto const ret = do_bench(data, braces, [&](auto& m2, auto& d2) { \
      for (auto& i : d2) { \
        m2.emplace(std::move(i)); \
      }  \
    });  \
    assert(size == ret); \
  }

#define BODY_LEVEL(size) \
  BODY_SETUP(size) \
  while(iters--) { \
    auto const ret = do_bench(data, braces, [&](auto& m2, auto& d2) { \
      auto tmp_data = d2; \
      apache::thrift::move_ordered_map_vec(m2, tmp_data); \
    });  \
    assert(size == ret); \
  }

#define COMPARE(num, name) \
BENCHMARK(map_insertion_sorted_order_##name, iters) { \
  BODY_SORTED(num) \
} \
BENCHMARK_RELATIVE(map_insertion_level_order_##name, iters) { \
  BODY_LEVEL(num) \
}

COMPARE(5,       _5)
COMPARE(20,      _20)
COMPARE(50,      _50)
COMPARE(75,      _75)
COMPARE(100,     _100)
COMPARE(200,     _200)
COMPARE(250,     _250)
COMPARE(500,     _500)
COMPARE(750,     _750)
COMPARE(1000,    _1k)
COMPARE(2500,    _2_5k)
COMPARE(5000,    _5k)
COMPARE(7500,    _7_5k)
COMPARE(10000,   _10k)
COMPARE(20000,   _20k)
COMPARE(50000,   _50k)
COMPARE(100000,  _100k)
COMPARE(1000000, _1M)

DEFINE_uint64(runs, 1,
  "number of times to execute benchmarks");

int main(int argc, char** argv) {
  google::ParseCommandLineFlags(&argc, &argv, true);

  while(FLAGS_runs--) {
    folly::runBenchmarks();
    std::cout.flush();
    std::cerr.flush();
  }

  return 0;
}
