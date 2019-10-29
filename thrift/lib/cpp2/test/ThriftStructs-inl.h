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

#pragma once
#include <string>
#include <vector>

#include <thrift/lib/cpp2/test/gen-cpp2/ProtocolBenchData_types.h>

// template specifications for thrift struct

template <>
thrift::benchmark::Empty create<thrift::benchmark::Empty>() {
  return thrift::benchmark::Empty();
}

template <>
thrift::benchmark::SmallInt create<thrift::benchmark::SmallInt>() {
  thrift::benchmark::SmallInt d;
  d.smallint = 5;
  return d;
}

template <>
thrift::benchmark::BigInt create<thrift::benchmark::BigInt>() {
  thrift::benchmark::BigInt d;
  d.bigint = 0x1234567890abcdefL;
  return d;
}

template <>
thrift::benchmark::SmallString create<thrift::benchmark::SmallString>() {
  thrift::benchmark::SmallString d;
  d.str = "small string";
  return d;
}

template <>
thrift::benchmark::BigString create<thrift::benchmark::BigString>() {
  thrift::benchmark::BigString d;
  d.str = std::string(10000, 'a');
  return d;
}

template <>
thrift::benchmark::BigBinary create<thrift::benchmark::BigBinary>() {
  auto buf = folly::IOBuf::create(10000);
  buf->append(10000);
  thrift::benchmark::BigBinary d;
  d.bin = std::move(buf);
  return d;
}

template <>
thrift::benchmark::LargeBinary create<thrift::benchmark::LargeBinary>() {
  auto buf = folly::IOBuf::create(10000000);
  buf->append(10000000);
  thrift::benchmark::LargeBinary d;
  d.bin = std::move(buf);
  return d;
}

template <>
thrift::benchmark::Mixed create<thrift::benchmark::Mixed>() {
  thrift::benchmark::Mixed d;
  d.int32 = 5;
  d.int64 = 12345;
  d.b = true;
  d.str = "hellohellohellohello";
  return d;
}

template <>
thrift::benchmark::SmallListInt create<thrift::benchmark::SmallListInt>() {
  std::srand(1);
  std::vector<int> vec;
  for (int i = 0; i < 10; i++) {
    vec.push_back(std::rand());
  }
  thrift::benchmark::SmallListInt d;
  d.lst = std::move(vec);
  return d;
}

template <>
thrift::benchmark::BigListInt create<thrift::benchmark::BigListInt>() {
  std::srand(1);
  std::vector<int> vec;
  for (int i = 0; i < 10000; i++) {
    vec.push_back(std::rand());
  }
  thrift::benchmark::BigListInt d;
  d.lst = std::move(vec);
  return d;
}

template <>
thrift::benchmark::BigListMixed create<thrift::benchmark::BigListMixed>() {
  std::vector<thrift::benchmark::Mixed> vec(
      10000, create<thrift::benchmark::Mixed>());
  thrift::benchmark::BigListMixed d;
  d.lst = std::move(vec);
  return d;
}

template <>
thrift::benchmark::LargeListMixed create<thrift::benchmark::LargeListMixed>() {
  std::vector<thrift::benchmark::Mixed> vec(
      1000000, create<thrift::benchmark::Mixed>());
  thrift::benchmark::LargeListMixed d;
  d.lst = std::move(vec);
  return d;
}

template <>
thrift::benchmark::LargeMapInt create<thrift::benchmark::LargeMapInt>() {
  std::srand(1);
  thrift::benchmark::LargeMapInt l;
  for (int i = 0; i < 1000000; i++) {
    l.m[i] = std::rand();
  }
  return l;
}

template <>
thrift::benchmark::NestedMapRaw create<thrift::benchmark::NestedMapRaw>() {
  thrift::benchmark::NestedMapRaw map;
  populateMap([&](int i, int j, int k, int l, int m, int v) {
    map.m[i][j][k][l][m] = v;
  });
  return map;
}

template <>
thrift::benchmark::SortedVecNestedMapRaw
create<thrift::benchmark::SortedVecNestedMapRaw>() {
  thrift::benchmark::SortedVecNestedMapRaw map;
  populateMap([&](int i, int j, int k, int l, int m, int v) {
    map.m[i][j][k][l][m] = v;
  });
  return map;
}

template <>
thrift::benchmark::NestedMap create<thrift::benchmark::NestedMap>() {
  thrift::benchmark::NestedMap map;
  populateMap([&](int i, int j, int k, int l, int m, int v) {
    map.m[i].m[j].m[k].m[l].m[m] = v;
  });
  return map;
}

template <>
thrift::benchmark::SortedVecNestedMap
create<thrift::benchmark::SortedVecNestedMap>() {
  thrift::benchmark::SortedVecNestedMap map;
  populateMap([&](int i, int j, int k, int l, int m, int v) {
    map.m[i].m[j].m[k].m[l].m[m] = v;
  });
  return map;
}

template <>
thrift::benchmark::LargeMixed create<thrift::benchmark::LargeMixed>() {
  thrift::benchmark::LargeMixed d;
  d.var1 = 5;
  d.var2 = 12345;
  d.var3 = true;
  d.var4 = "hello";
  d.var5 = 5;
  d.var6 = 12345;
  d.var7 = true;
  d.var8 = "hello";
  d.var9 = 5;
  d.var10 = 12345;
  d.var11 = true;
  d.var12 = "hello";
  d.var13 = 5;
  d.var14 = 12345;
  d.var15 = true;
  d.var16 = "hello";
  d.var17 = 5;
  d.var18 = 12345;
  d.var19 = true;
  d.var20 = "hello";
  d.var21 = 5;
  d.var22 = 12345;
  d.var23 = true;
  d.var24 = "hello";
  d.var25 = 5;
  d.var26 = 12345;
  d.var27 = true;
  d.var28 = "hello";
  d.var29 = 5;
  d.var30 = 12345;
  d.var31 = true;
  d.var32 = "hello";
  d.var33 = 5;
  d.var34 = 12345;
  d.var35 = true;
  d.var36 = "hello";
  d.var37 = 5;
  d.var38 = 12345;
  d.var39 = true;
  d.var40 = "hello";
  d.var41 = 5;
  d.var42 = 12345;
  d.var43 = true;
  d.var44 = "hello";
  d.var45 = 5;
  d.var46 = 12345;
  d.var47 = true;
  d.var48 = "hello";
  d.var49 = 5;
  d.var50 = 12345;
  d.var51 = true;
  d.var52 = "hello";
  d.var53 = 5;
  d.var54 = 12345;
  d.var55 = true;
  d.var56 = "hello";
  d.var57 = 5;
  d.var58 = 12345;
  d.var59 = true;
  d.var60 = "hello";
  d.var61 = 5;
  d.var62 = 12345;
  d.var63 = true;
  d.var64 = "hello";
  d.var65 = 5;
  d.var66 = 12345;
  d.var67 = true;
  d.var68 = "hello";
  d.var69 = 5;
  d.var70 = 12345;
  d.var71 = true;
  d.var72 = "hello";
  d.var73 = 5;
  d.var74 = 12345;
  d.var75 = true;
  d.var76 = "hello";
  d.var77 = 5;
  d.var78 = 12345;
  d.var79 = true;
  d.var80 = "hello";
  d.var81 = 5;
  d.var82 = 12345;
  d.var83 = true;
  d.var84 = "hello";
  d.var85 = 5;
  d.var86 = 12345;
  d.var87 = true;
  d.var88 = "hello";
  d.var89 = 5;
  d.var90 = 12345;
  d.var91 = true;
  d.var92 = "hello";
  d.var93 = 5;
  d.var94 = 12345;
  d.var95 = true;
  d.var96 = "hello";
  d.var97 = 5;
  d.var98 = 12345;
  d.var99 = true;
  d.var100 = "hello";
  return d;
}

template <>
thrift::benchmark::MixedInt create<thrift::benchmark::MixedInt>() {
  std::srand(1);
  thrift::benchmark::MixedInt d;
  d.var1 = std::rand();
  d.var2 = std::rand();
  d.var3 = std::rand();
  d.var4 = std::rand();
  d.var5 = std::rand();
  d.var6 = std::rand();
  d.var7 = std::rand();
  d.var8 = std::rand();
  d.var9 = std::rand();
  d.varx = std::rand();
  d.vary = std::rand();
  d.varz = std::rand();
  return d;
}

template <>
thrift::benchmark::BigListMixedInt
create<thrift::benchmark::BigListMixedInt>() {
  std::vector<thrift::benchmark::MixedInt> vec(
      10000, create<thrift::benchmark::MixedInt>());
  thrift::benchmark::BigListMixedInt d;
  d.lst = std::move(vec);
  return d;
}

template <>
thrift::benchmark::ComplexStruct create<thrift::benchmark::ComplexStruct>() {
  thrift::benchmark::ComplexStruct d;
  d.var1 = create<thrift::benchmark::Empty>();
  d.var2 = create<thrift::benchmark::SmallInt>();
  d.var3 = create<thrift::benchmark::BigInt>();
  d.var4 = create<thrift::benchmark::SmallString>();
  d.var5 = create<thrift::benchmark::BigString>();
  d.var6 = create<thrift::benchmark::Mixed>();
  d.var7 = create<thrift::benchmark::SmallListInt>();
  d.var8 = create<thrift::benchmark::BigListInt>();
  d.var9 = create<thrift::benchmark::LargeListMixed>();
  d.var10 = create<thrift::benchmark::LargeMapInt>();
  d.var11 = create<thrift::benchmark::LargeMixed>();
  d.var12 = create<thrift::benchmark::NestedMap>();
  return d;
}
