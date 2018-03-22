/*
 * Copyright 2016-present Facebook, Inc.
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

#include <thrift/lib/cpp2/fatal/container_traits_folly.h>
#include <thrift/lib/cpp2/fatal/traits_test_helpers.h>

namespace test_cpp2 {
namespace cpp_reflection {

TEST(reflection, thrift_fbstring_traits) {
  apache::thrift::test_thrift_string_traits<folly::fbstring>();
}

TEST(reflection, thrift_small_vector_list_traits) {
  apache::thrift::test_thrift_list_traits<folly::small_vector<int>>();
}

TEST(reflection, thrift_sorted_vector_set_traits) {
  apache::thrift::test_thrift_set_traits<folly::sorted_vector_set<int>>();
}

TEST(reflection, thrift_sorted_vector_map_traits) {
  apache::thrift::test_thrift_map_traits<folly::sorted_vector_map<int, int>>();
}

TEST(reflection, thrift_f14_set_traits) {
  apache::thrift::test_thrift_set_traits<folly::F14NodeSet<int>>();
  apache::thrift::test_thrift_set_traits<folly::F14ValueSet<int>>();
  apache::thrift::test_thrift_set_traits<folly::F14VectorSet<int>>();
  apache::thrift::test_thrift_set_traits<folly::F14FastSet<int>>();
}

TEST(reflection, thrift_f14_map_traits) {
  apache::thrift::test_thrift_map_traits<folly::F14NodeMap<int, int>>();
  apache::thrift::test_thrift_map_traits<folly::F14ValueMap<int, int>>();
  apache::thrift::test_thrift_map_traits<folly::F14VectorMap<int, int>>();
  apache::thrift::test_thrift_map_traits<folly::F14FastMap<int, int>>();
}

} // namespace cpp_reflection
} // namespace test_cpp2
