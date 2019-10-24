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

#include <cstdint>
#include <string>
#include <vector>

#include <folly/Benchmark.h>

namespace apache {
namespace thrift {
namespace detail {

struct LabelledData {
  std::string name;
  std::vector<std::uint32_t> chunks;
};

inline std::vector<LabelledData> labelledData() {
  return {
      {"Zeros", {0}},
      {"One", {1, 100, 22}},
      {"Two", {300, 5 * 1000, 20 * 1000}},
      {"Four", {70000, 1000 * 1000, 100 * 1000}},
      {"Mixed", {1, 1000 * 1000, 0, 1234, 999 * 999, 0, 1000, 1234567, 100}},
  };
}

template <typename Func1, typename Func2>
void addAllBenchmarks(const char* file, Func1 runRaw, Func2 runZigzag) {
  for (auto& data : labelledData()) {
    folly::addBenchmark(file, (data.name + "Raw").c_str(), [=](unsigned iters) {
      runRaw(iters, data.chunks);
      return iters;
    });
    folly::addBenchmark(
        file, (data.name + "Zigzag").c_str(), [=](unsigned iters) {
          runZigzag(iters, data.chunks);
          return iters;
        });
  }
}

} // namespace detail
} // namespace thrift
} // namespace apache
