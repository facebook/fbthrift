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

#include <thrift/lib/cpp/protocol/neutronium/InternTable.h>

#include <gtest/gtest.h>

using namespace apache::thrift::protocol::neutronium;

TEST(InternTable, Simple) {
  std::unique_ptr<folly::IOBuf> buf;

  {
    InternTable tab;
    EXPECT_EQ(0, tab.add("hello"));
    EXPECT_EQ(1, tab.add("world"));
    EXPECT_EQ(2, tab.add("goodbye"));
    EXPECT_EQ(1, tab.add("world"));
    EXPECT_EQ("hello", tab.get(0).toString());
    EXPECT_EQ("world", tab.get(1).toString());
    EXPECT_EQ("goodbye", tab.get(2).toString());
    buf = tab.serialize();
  }

  {
    InternTable tab;
    tab.deserialize(std::move(buf));
    EXPECT_EQ("hello", tab.get(0).toString());
    EXPECT_EQ("world", tab.get(1).toString());
    EXPECT_EQ("goodbye", tab.get(2).toString());
    EXPECT_EQ(3, tab.add("meow"));
    EXPECT_EQ(1, tab.add("world"));
    EXPECT_EQ("meow", tab.get(3).toString());
  }
}
