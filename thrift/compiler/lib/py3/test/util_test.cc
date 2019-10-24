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

#include <thrift/compiler/lib/py3/util.h>

#include <memory>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <thrift/compiler/ast/t_field.h>

using namespace apache::thrift::compiler;

class UtilTest : public testing::Test {};

TEST_F(UtilTest, get_py3_name) {
  EXPECT_EQ("foo", py3::get_py3_name(t_field(nullptr, "foo")));
  EXPECT_EQ("True_", py3::get_py3_name(t_field(nullptr, "True")));
  EXPECT_EQ("cpdef_", py3::get_py3_name(t_field(nullptr, "cpdef")));

  t_field f(nullptr, "foo");
  f.annotations_["py3.name"] = "bar";
  EXPECT_EQ("bar", py3::get_py3_name(f));
}
