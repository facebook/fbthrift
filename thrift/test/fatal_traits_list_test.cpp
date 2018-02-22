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

#include <thrift/lib/cpp2/fatal/container_traits.h>
#include <thrift/lib/cpp2/fatal/traits_test_helpers.h>

namespace std {
static void PrintTo(deque<int>::iterator it, ostream* o) {
  *o << "deque-iterator<" << *it << ">";
}
}

namespace test_cpp2 {
namespace cpp_reflection {

TEST(reflection, thrift_std_deque_traits) {
  apache::thrift::test_thrift_list_traits<std::deque<int>>();
}

TEST(reflection, thrift_std_vector_traits) {
  apache::thrift::test_thrift_list_traits<std::vector<int>>();
}

} // namespace cpp_reflection {
} // namespace test_cpp2 {
