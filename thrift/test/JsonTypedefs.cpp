/*
 * Copyright 2014-present Facebook, Inc.
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

#include <common/init/Init.h>
#include <gtest/gtest.h>
#include <thrift/test/gen-cpp/JsonReaderTest_types.h>

int main(int argc, char *argv[]) {
  testing::InitGoogleTest(&argc, argv);
  facebook::initFacebook(&argc, &argv);
  return RUN_ALL_TESTS();
}

TEST(JsonTypedefs, test) {
  JsonTypedefs a;
  a.readFromJson("{\"x\":{\"1\":11}, \"y\":[2], \"z\":[3], \"w\":4}");
  EXPECT_FALSE(a.x.empty());
  EXPECT_FALSE(a.y.empty());
  EXPECT_FALSE(a.z.empty());
  EXPECT_NE(a.w, 0);
}
