/*
 * Copyright 2013-present Facebook, Inc.
 *
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

#include <thrift/test/gen-cpp/NoExcept_types.h>
#include <thrift/test/gen-cpp2/NoExcept_types.h>

#include <glog/logging.h>
#include <gtest/gtest.h>

namespace thrift { namespace test { namespace no_except {

#define TEST_NOEXCEPT_CTOR_1(Type) \
  do { \
    Type a; \
    EXPECT_TRUE(noexcept(Type(std::move(a)))) << #Type; \
  } while (false)

#define TEST_NOEXCEPT_ASSIGN_1(Type) \
  do { \
    Type a; \
    Type b; \
    EXPECT_TRUE(noexcept(b = std::move(a))) << #Type; \
  } while (false)

#define TEST_NOEXCEPT_CTOR(Type) \
  do { \
    TEST_NOEXCEPT_CTOR_1(Type); \
    TEST_NOEXCEPT_CTOR_1(cpp2::Type); \
  } while (false)

#define TEST_NOEXCEPT_ALL(Type) \
  do { \
    TEST_NOEXCEPT_CTOR_1(Type); \
    TEST_NOEXCEPT_ASSIGN_1(Type); \
    TEST_NOEXCEPT_CTOR_1(cpp2::Type); \
    TEST_NOEXCEPT_ASSIGN_1(cpp2::Type); \
  } while (false)

TEST(NoExcept, noexcept) {
  TEST_NOEXCEPT_ALL(Simple);
  TEST_NOEXCEPT_CTOR(SimpleWithString);
  TEST_NOEXCEPT_ALL(List);
  TEST_NOEXCEPT_CTOR(Set);
  TEST_NOEXCEPT_CTOR(Map);
  TEST_NOEXCEPT_ALL(Complex);
  TEST_NOEXCEPT_CTOR(ComplexWithStringAndMap);
}

#undef TEST_NOEXCEPT_ALL
#undef TEST_NOEXCEPT_CTOR
#undef TEST_NOEXCEPT_ASSIGN_1
#undef TEST_NOEXCEPT_CTOR_1

}}}  // namespaces
