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

#include <bitset>
#include <boost/mp11.hpp>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/gen/module_types_h.h>

TEST(IssetBitsetTest, basic) {
  const int N = 100;
  apache::thrift::detail::isset_bitset<N, true> packed;
  apache::thrift::detail::isset_bitset<N, false> unpacked;
  std::bitset<8> bits;
  for (int i = 0; i < N; i++) {
    if (i % 8 == 0) {
      bits = 0;
    }
    for (bool b : {true, false, true}) {
      boost::mp11::mp_with_index<N>(i, [&](auto index) {
        packed.set(index, b);
        unpacked.set(index, b);
        EXPECT_EQ(packed.get(index), b);
        EXPECT_EQ(unpacked.get(index), b);

        bits.set(index % 8, b);
        EXPECT_EQ(packed.at(index), bits.to_ulong());
        EXPECT_EQ(unpacked.at(index), b ? 1 : 0);
        EXPECT_EQ(packed.bit(index), index % 8);
        EXPECT_EQ(unpacked.bit(index), 0);
      });
    }
  }
}

TEST(IssetBitsetTest, compare_size) {
  static_assert(sizeof(apache::thrift::detail::isset_bitset<1>) == 1);
  static_assert(sizeof(apache::thrift::detail::isset_bitset<2>) == 2);
  static_assert(sizeof(apache::thrift::detail::isset_bitset<7, true>) == 1);
  static_assert(sizeof(apache::thrift::detail::isset_bitset<8, true>) == 1);
  static_assert(sizeof(apache::thrift::detail::isset_bitset<9, true>) == 2);
}
