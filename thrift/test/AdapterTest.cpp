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

#include <thrift/test/AdapterTest.h>

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/Adapt.h>
#include <thrift/test/gen-cpp2/adapter_types.h>

namespace apache::thrift::test {
namespace {

template <typename Actual, typename Expected>
struct AssertSameType;
template <typename T>
struct AssertSameType<T, T> {};

TEST(AdaptTest, AdaptedT) {
  AssertSameType<adapt_detail::adapted_t<OverloadedAdatper, int64_t>, Num>();
  AssertSameType<
      adapt_detail::adapted_t<OverloadedAdatper, std::string>,
      String>();
}

TEST(AdaptTest, CodeGen) {
  AdaptTestStruct data;
  // TODO(afuller): Support adapter in code gen.
  // AssertSameType<decltype(*data.delay_ref()), std::chrono::milliseconds&>();
  AssertSameType<decltype(*data.delay_ref()), int64_t&>();
}

} // namespace
} // namespace apache::thrift::test
