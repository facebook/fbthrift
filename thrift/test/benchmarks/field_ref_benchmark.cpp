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
#include <folly/Demangle.h>
#include <folly/Traits.h>
#include <folly/init/Init.h>
#include <glog/logging.h>
#include <thrift/test/testset/gen-cpp2/gen_struct_all_for_each_field.h>

using namespace apache::thrift;
using namespace std;

const int kIterationCount = 1'000'000;

template <class T>
using DetectValueUnchecked = decltype(T{}.field_1_ref().value_unchecked());

template <class Struct>
void add_benchmark() {
  string name = folly::demangle(typeid(Struct).name()).toStdString();
  name = name.substr(name.rfind("::") + 2);

  folly::addBenchmark(__FILE__, name, [] {
    static Struct s;
    for (int i = 0; i < kIterationCount; i++) {
      s.field_1_ref() = "a";
      folly::doNotOptimizeAway(s.field_1_ref());
      folly::doNotOptimizeAway(s.field_1_ref().value());
    }
    return 1;
  });

  folly::addBenchmark(__FILE__, '%' + name + "_unsafe", [] {
    static Struct s;
    for (int i = 0; i < kIterationCount; i++) {
      if constexpr (folly::is_detected_v<DetectValueUnchecked, Struct>) {
        s.field_1_ref().value_unchecked() = "a";
        folly::doNotOptimizeAway(s.field_1_ref());
        folly::doNotOptimizeAway(s.field_1_ref().value_unchecked());
      } else {
        s.field_1_ref().value() = "a";
        folly::doNotOptimizeAway(s.field_1_ref());
        folly::doNotOptimizeAway(s.field_1_ref().value());
      }
    }
    return 1;
  });
}

int main(int argc, char** argv) {
  folly::init(&argc, &argv);
  add_benchmark<test::struct_string>();
  add_benchmark<test::struct_optional_string>();
  add_benchmark<test::struct_required_string>();
  add_benchmark<test::struct_optional_string_cpp_ref>();
  folly::runBenchmarks();
  return 0;
}
