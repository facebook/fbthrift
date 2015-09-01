/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
* software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <thrift/test/gen-cpp2/OrderTest_types.h>

#include <gtest/gtest.h>

using namespace thrift::test::cpp2;
using namespace testing;

TEST(OrderTest, EqNotEq) {
  QQQ qqq(apache::thrift::FRAGILE, 5);
  Order a(apache::thrift::FRAGILE, 1, qqq);
  Order b(apache::thrift::FRAGILE, 2, qqq);
  Order c(apache::thrift::FRAGILE, 2, QQQ());
  Order d(apache::thrift::FRAGILE, 2, QQQ(qqq));
  EXPECT_FALSE(a == b);
  EXPECT_TRUE(a != b);
  EXPECT_TRUE(a == a);
  EXPECT_TRUE(b != c);
  EXPECT_TRUE(b == d);
  EXPECT_FALSE(b != d);
}
