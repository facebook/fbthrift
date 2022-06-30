/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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
#include <thrift/lib/cpp2/FieldMask.h>
#include <thrift/lib/thrift/gen-cpp2/protocol_constants.h>
#include <thrift/lib/thrift/gen-cpp2/protocol_types.h>

using apache::thrift::protocol::Mask;
using apache::thrift::protocol::protocol_constants;
using namespace apache::thrift::protocol::detail;

namespace apache::thrift::test {

bool literallyEqual(const MaskRef& actual, const MaskRef& expected) {
  return actual.mask == expected.mask &&
      actual.is_exclusion == expected.is_exclusion;
}

TEST(FieldMaskTest, Example) {
  // example masks
  // includes{7: excludes{},
  //          9: includes{5: excludes{},
  //                      6: excludes{}}}
  Mask m;
  auto& includes = m.includes_ref().emplace();
  includes[7] = protocol_constants::allMask();
  auto& nestedIncludes = includes[9].includes_ref().emplace();
  nestedIncludes[5] = protocol_constants::allMask();
  nestedIncludes[6] = protocol_constants::allMask();
  includes[8] = protocol_constants::noneMask(); // not required
}

TEST(FieldMaskTest, Constant) {
  EXPECT_EQ(protocol_constants::allMask().excludes_ref()->size(), 0);
  EXPECT_EQ(protocol_constants::noneMask().includes_ref()->size(), 0);
}

TEST(FieldMaskTest, IsAllMask) {
  EXPECT_TRUE((MaskRef{protocol_constants::allMask(), false}).isAllMask());
  EXPECT_TRUE((MaskRef{protocol_constants::noneMask(), true}).isAllMask());
  EXPECT_FALSE((MaskRef{protocol_constants::noneMask(), false}).isAllMask());
  EXPECT_FALSE((MaskRef{protocol_constants::allMask(), true}).isAllMask());
  Mask m;
  m.excludes_ref().emplace()[5] = protocol_constants::allMask();
  EXPECT_FALSE((MaskRef{m, false}).isAllMask());
  EXPECT_FALSE((MaskRef{m, true}).isAllMask());
}

TEST(FieldMaskTest, IsNoneMask) {
  EXPECT_TRUE((MaskRef{protocol_constants::noneMask(), false}).isNoneMask());
  EXPECT_TRUE((MaskRef{protocol_constants::allMask(), true}).isNoneMask());
  EXPECT_FALSE((MaskRef{protocol_constants::allMask(), false}).isNoneMask());
  EXPECT_FALSE((MaskRef{protocol_constants::noneMask(), true}).isNoneMask());
  Mask m;
  m.excludes_ref().emplace()[5] = protocol_constants::noneMask();
  EXPECT_FALSE((MaskRef{m, false}).isNoneMask());
  EXPECT_FALSE((MaskRef{m, true}).isNoneMask());
}

TEST(FieldMaskTest, MaskRefGetIncludes) {
  Mask m;
  // includes{8: excludes{},
  //          9: includes{4: excludes{}}
  auto& includes = m.includes_ref().emplace();
  includes[8] = protocol_constants::allMask();
  includes[9].includes_ref().emplace()[4] = protocol_constants::allMask();

  EXPECT_TRUE(
      (MaskRef{m, false}).get(FieldId{7}).isNoneMask()); // doesn't exist
  EXPECT_TRUE((MaskRef{m, true}).get(FieldId{7}).isAllMask()); // doesn't exist
  EXPECT_TRUE((MaskRef{m, false}).get(FieldId{8}).isAllMask());
  EXPECT_TRUE((MaskRef{m, true}).get(FieldId{8}).isNoneMask());
  EXPECT_TRUE(literallyEqual(
      (MaskRef{m, false}).get(FieldId{9}), (MaskRef{includes[9], false})));
  EXPECT_TRUE(literallyEqual(
      (MaskRef{m, true}).get(FieldId{9}), (MaskRef{includes[9], true})));
  // recursive calls to MaskRef Get
  EXPECT_TRUE((MaskRef{m, false}).get(FieldId{9}).get(FieldId{4}).isAllMask());
  EXPECT_TRUE((MaskRef{m, true}).get(FieldId{9}).get(FieldId{4}).isNoneMask());
  EXPECT_TRUE((MaskRef{m, false})
                  .get(FieldId{9})
                  .get(FieldId{5})
                  .isNoneMask()); // doesn't exist
  EXPECT_TRUE((MaskRef{m, true})
                  .get(FieldId{9})
                  .get(FieldId{5})
                  .isAllMask()); // doesn't exist
}

TEST(FieldMaskTest, MaskRefGetExcludes) {
  Mask m;
  // excludes{8: excludes{},
  //          9: includes{4: excludes{}}
  auto& excludes = m.excludes_ref().emplace();
  excludes[8] = protocol_constants::allMask();
  excludes[9].includes_ref().emplace()[4] = protocol_constants::allMask();

  EXPECT_TRUE((MaskRef{m, false}).get(FieldId{7}).isAllMask()); // doesn't exist
  EXPECT_TRUE((MaskRef{m, true}).get(FieldId{7}).isNoneMask()); // doesn't exist
  EXPECT_TRUE((MaskRef{m, false}).get(FieldId{8}).isNoneMask());
  EXPECT_TRUE((MaskRef{m, true}).get(FieldId{8}).isAllMask());
  EXPECT_TRUE(literallyEqual(
      (MaskRef{m, false}).get(FieldId{9}), (MaskRef{excludes[9], true})));
  EXPECT_TRUE(literallyEqual(
      (MaskRef{m, true}).get(FieldId{9}), (MaskRef{excludes[9], false})));
  // recursive calls to MaskRef Get
  EXPECT_TRUE((MaskRef{m, false}).get(FieldId{9}).get(FieldId{4}).isNoneMask());
  EXPECT_TRUE((MaskRef{m, true}).get(FieldId{9}).get(FieldId{4}).isAllMask());
  EXPECT_TRUE((MaskRef{m, false})
                  .get(FieldId{9})
                  .get(FieldId{5})
                  .isAllMask()); // doesn't exist
  EXPECT_TRUE((MaskRef{m, true})
                  .get(FieldId{9})
                  .get(FieldId{5})
                  .isNoneMask()); // doesn't exist
}

TEST(FieldMaskTest, Clear) {
  protocol::Object fooObject, barObject, bazObject;
  // bar{1: foo{1: baz{1: 30},
  //            2: 10},
  //     2: "40",
  //     3: 5}
  bazObject[FieldId{1}].emplace_i32() = 30;
  fooObject[FieldId{1}].emplace_object() = bazObject;
  fooObject[FieldId{2}].emplace_i32() = 10;
  barObject[FieldId{1}].emplace_object() = fooObject;
  barObject[FieldId{2}].emplace_string() = "40";
  barObject[FieldId{3}].emplace_i32() = 5;

  Mask mask;
  // includes {2: excludes{},
  //           1: excludes{5: excludes{5: excludes{}},
  //                       1: excludes{}}}
  auto& includes = mask.includes_ref().emplace();
  includes[2] = protocol_constants::allMask();
  auto& nestedExcludes = includes[1].excludes_ref().emplace();
  nestedExcludes[5].excludes_ref().emplace()[5] =
      protocol_constants::allMask(); // The object doesn't have this field.
  nestedExcludes[1] = protocol_constants::allMask();
  // This clears object[1][2] and object[2].
  protocol::clear(mask, barObject);

  ASSERT_TRUE(barObject.contains(FieldId{1}));
  protocol::Object& foo = barObject.at(FieldId{1}).objectValue_ref().value();
  ASSERT_TRUE(foo.contains(FieldId{1}));
  ASSERT_TRUE(foo.at(FieldId{1}).objectValue_ref()->contains(FieldId{1}));
  EXPECT_EQ(foo.at(FieldId{1}).objectValue_ref()->at(FieldId{1}).as_i32(), 30);
  EXPECT_FALSE(foo.contains(FieldId{2}));
  EXPECT_FALSE(barObject.contains(FieldId{2}));
  EXPECT_TRUE(barObject.contains(FieldId{3}));
  EXPECT_EQ(barObject.at(FieldId{3}).as_i32(), 5);
}

TEST(FieldMaskTest, ClearException) {
  protocol::Object bazObject;
  // bar{2: "40"}
  bazObject[FieldId{2}].emplace_string() = "40";

  Mask m1; // object[2] is not an object but has an object mask.
  auto& includes = m1.includes_ref().emplace();
  includes[2].includes_ref().emplace()[4] = protocol_constants::noneMask();
  EXPECT_THROW(protocol::clear(m1, bazObject), std::runtime_error);

  protocol::Object fooObject, barObject;
  // bar{1: foo{2: 20}, 2: "40"}
  fooObject[FieldId{2}].emplace_i32() = 20;
  barObject[FieldId{1}].emplace_object() = fooObject;
  barObject[FieldId{2}].emplace_string() = "40";

  Mask m2; // object[1][2] is not an object but has am object mask.
  auto& includes2 = m2.includes_ref().emplace();
  includes2[1].includes_ref().emplace()[2].excludes_ref().emplace()[5] =
      protocol_constants::allMask();
  includes2[2] = protocol_constants::allMask();
  EXPECT_THROW(protocol::clear(m2, barObject), std::runtime_error);
}
} // namespace apache::thrift::test
