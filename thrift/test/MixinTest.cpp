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

#include <folly/portability/GTest.h>
#include <thrift/test/gen-cpp2/MixinTest_types.h>

TEST(Mixin, Simple) {
  cpp2::Foo s;
  s.m2_ref()->m1_ref()->field1_ref() = "1";
  s.m2_ref()->field2_ref() = "2";
  s.m3_ref()->field3_ref() = "3";
  s.field4_ref() = "4";
}
