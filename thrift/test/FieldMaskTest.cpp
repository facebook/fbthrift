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
#include <thrift/test/gen-cpp2/FieldMask_types.h>

using apache::thrift::protocol::Mask;
using apache::thrift::protocol::protocol_constants;
using namespace apache::thrift::protocol::detail;

namespace apache::thrift::test {

bool literallyEqual(MaskRef actual, MaskRef expected) {
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

TEST(FieldMaskTest, IsExclusive) {
  EXPECT_FALSE((MaskRef{protocol_constants::noneMask(), false}).isExclusive());
  EXPECT_FALSE((MaskRef{protocol_constants::allMask(), true}).isExclusive());
  EXPECT_TRUE((MaskRef{protocol_constants::allMask(), false}).isExclusive());
  EXPECT_TRUE((MaskRef{protocol_constants::noneMask(), true}).isExclusive());
  Mask m;
  m.includes_ref().emplace()[5] = protocol_constants::allMask();
  EXPECT_FALSE((MaskRef{m, false}).isExclusive());
  EXPECT_TRUE((MaskRef{m, true}).isExclusive());
  m.excludes_ref().emplace()[5] = protocol_constants::noneMask();
  EXPECT_TRUE((MaskRef{m, false}).isExclusive());
  EXPECT_FALSE((MaskRef{m, true}).isExclusive());
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

  Mask m2; // object[1][2] is not an object but has an object mask.
  auto& includes2 = m2.includes_ref().emplace();
  includes2[1].includes_ref().emplace()[2].excludes_ref().emplace()[5] =
      protocol_constants::allMask();
  includes2[2] = protocol_constants::allMask();
  EXPECT_THROW(protocol::clear(m2, barObject), std::runtime_error);
}

// Calls copy on the mask, src, and dst.
// Then checks the read-write consistency of the copy.
void testCopy(
    const Mask& mask, const protocol::Object& src, protocol::Object& dst) {
  protocol::copy(mask, src, dst);
  // copy(dst, src) should be no-op.
  protocol::Object result;
  result = src;
  protocol::copy(mask, dst, result);
  EXPECT_EQ(result, src);
}

TEST(FieldMaskTest, CopySimpleIncludes) {
  protocol::Object fooObject, barObject, bazObject;
  // foo{1: 10}
  // bar{1: "30", 2: 20}
  // baz = bar
  fooObject[FieldId{1}].emplace_i32() = 10;
  barObject[FieldId{2}].emplace_string() = "30";
  barObject[FieldId{2}].emplace_i32() = 20;
  bazObject = barObject;

  Mask m;
  // includes{1: exludes{}}
  auto& includes = m.includes_ref().emplace();
  includes[1] = protocol_constants::allMask();
  testCopy(m, fooObject, barObject);
  // bar becomes bar{1: 10, 2: 20}
  ASSERT_TRUE(barObject.contains(FieldId{1}));
  EXPECT_EQ(barObject.at(FieldId{1}).as_i32(), 10);
  ASSERT_TRUE(barObject.contains(FieldId{2}));
  EXPECT_EQ(barObject.at(FieldId{2}).as_i32(), 20);

  testCopy(protocol_constants::allMask(), fooObject, bazObject);
  // baz becomes baz{1: 10}
  ASSERT_TRUE(bazObject.contains(FieldId{1}));
  EXPECT_EQ(bazObject.at(FieldId{1}).as_i32(), 10);
  ASSERT_FALSE(bazObject.contains(FieldId{2}));

  // includes{1: exludes{}, 2: excludes{}, 3: excludes{}}
  includes[2] = protocol_constants::allMask();
  includes[3] = protocol_constants::allMask(); // no-op
  testCopy(m, barObject, bazObject);
  // baz becomes baz{1: 10, 2: 20}
  ASSERT_TRUE(bazObject.contains(FieldId{1}));
  EXPECT_EQ(bazObject.at(FieldId{1}).as_i32(), 10);
  ASSERT_TRUE(bazObject.contains(FieldId{2}));
  EXPECT_EQ(bazObject.at(FieldId{2}).as_i32(), 20);
  ASSERT_FALSE(bazObject.contains(FieldId{3}));
}

TEST(FieldMaskTest, CopySimpleExcludes) {
  protocol::Object fooObject, barObject;
  // foo{1: 10, 3: 40}
  // bar{1: "30", 2: 20}
  fooObject[FieldId{1}].emplace_i32() = 10;
  fooObject[FieldId{3}].emplace_i32() = 40;
  barObject[FieldId{1}].emplace_string() = "30";
  barObject[FieldId{2}].emplace_i32() = 20;

  Mask m;
  // excludes{1: exludes{}}
  m.excludes_ref().emplace()[1] = protocol_constants::allMask();
  testCopy(m, fooObject, barObject);
  // bar becomes bar{1: "30", 3: 40}
  ASSERT_TRUE(barObject.contains(FieldId{1}));
  EXPECT_EQ(barObject.at(FieldId{1}).as_string(), "30");
  ASSERT_TRUE(barObject.contains(FieldId{3}));
  EXPECT_EQ(barObject.at(FieldId{3}).as_i32(), 40);
  ASSERT_FALSE(barObject.contains(FieldId{2}));
}

TEST(FieldMaskTest, CopyNestedRecursive) {
  protocol::Object fooObject, barObject;
  // bar{1: foo{1: 10,
  //            2: 20},
  //     2: "40"}
  fooObject[FieldId{1}].emplace_i32() = 10;
  fooObject[FieldId{2}].emplace_i32() = 20;
  barObject[FieldId{1}].emplace_object() = fooObject;
  barObject[FieldId{2}].emplace_string() = "40";

  protocol::Object dst = barObject;
  protocol::Object& nestedObject = dst[FieldId{1}].objectValue_ref().value();
  nestedObject[FieldId{1}].emplace_object() = fooObject;
  nestedObject[FieldId{2}].emplace_i32() = 30;
  // dst{1: {1: {1: 10,
  //             2: 20},
  //         2: 30},
  //     2: "40"}

  Mask m;
  // excludes{1: exludes{1: excludes{}}}
  m.excludes_ref().emplace()[1].excludes_ref().emplace()[1] =
      protocol_constants::allMask();
  testCopy(m, barObject, dst);
  // dst becomes
  // dst{1: {1: 10
  //         2: 30},
  //     2: "40"}
  ASSERT_TRUE(dst.contains(FieldId{1}));
  protocol::Object& nested = dst.at(FieldId{1}).objectValue_ref().value();
  ASSERT_TRUE(nested.contains(FieldId{1}));
  EXPECT_EQ(nested.at(FieldId{1}).as_i32(), 10);
  ASSERT_TRUE(nested.contains(FieldId{2}));
  EXPECT_EQ(nested.at(FieldId{2}).as_i32(), 30);
  ASSERT_TRUE(dst.contains(FieldId{2}));
  EXPECT_EQ(dst.at(FieldId{2}).as_string(), "40");
}

TEST(FieldMaskTest, CopyNestedAddField) {
  protocol::Object fooObject, barObject;
  // bar{1: foo{1: 10,
  //            2: 20},
  //     2: "40"}
  fooObject[FieldId{1}].emplace_i32() = 10;
  fooObject[FieldId{2}].emplace_i32() = 20;
  barObject[FieldId{1}].emplace_object() = fooObject;
  barObject[FieldId{2}].emplace_string() = "40";

  Mask m1;
  // includes{1: includes{2: excludes{}},
  //          2: excludes{}}
  auto& includes = m1.includes_ref().emplace();
  includes[1].includes_ref().emplace()[2] = protocol_constants::allMask();
  includes[2] = protocol_constants::allMask();

  protocol::Object dst1;
  testCopy(m1, barObject, dst1);
  // dst1 becomes dst1{1: {2: 20}, 2: "40"}
  ASSERT_TRUE(dst1.contains(FieldId{1}));
  protocol::Object& nested = dst1.at(FieldId{1}).objectValue_ref().value();
  ASSERT_TRUE(nested.contains(FieldId{2}));
  EXPECT_EQ(nested.at(FieldId{2}).as_i32(), 20);
  ASSERT_FALSE(nested.contains(FieldId{1}));
  ASSERT_TRUE(dst1.contains(FieldId{2}));
  EXPECT_EQ(dst1.at(FieldId{2}).as_string(), "40");

  Mask m2;
  // excludes{1: includes{1: excludes{}, 2: excludes{}, 3: includes{}},
  //          2: includes{}}
  auto& excludes = m2.excludes_ref().emplace();
  auto& nestedIncludes = excludes[1].includes_ref().emplace();
  nestedIncludes[1] = protocol_constants::allMask();
  nestedIncludes[2] = protocol_constants::allMask();
  nestedIncludes[3] = protocol_constants::allMask();
  excludes[2] = protocol_constants::noneMask();

  protocol::Object dst2;
  testCopy(m2, barObject, dst2);
  // dst2 becomes dst2{2: "40"} (doesn't create nested object).
  ASSERT_FALSE(dst2.contains(FieldId{1}));
  ASSERT_TRUE(dst2.contains(FieldId{2}));
  EXPECT_EQ(dst2.at(FieldId{2}).as_string(), "40");
}

TEST(FieldMaskTest, CopyNestedRemoveField) {
  protocol::Object fooObject, barObject;
  // bar{1: foo{1: 10,
  //            2: 20},
  //     2: "40"}
  fooObject[FieldId{1}].emplace_i32() = 10;
  fooObject[FieldId{2}].emplace_i32() = 20;
  barObject[FieldId{1}].emplace_object() = fooObject;
  barObject[FieldId{2}].emplace_string() = "40";

  Mask m;
  // includes{1: includes{2: excludes{}},
  //          2: excludes{}}
  auto& includes = m.includes_ref().emplace();
  includes[1].includes_ref().emplace()[2] = protocol_constants::allMask();
  includes[2] = protocol_constants::allMask();

  protocol::Object src;
  testCopy(m, src, barObject);
  // bar becomes bar{1: foo{1: 10}} (doesn't delete foo object).
  ASSERT_TRUE(barObject.contains(FieldId{1}));
  protocol::Object& nested = barObject.at(FieldId{1}).objectValue_ref().value();
  ASSERT_TRUE(nested.contains(FieldId{1}));
  EXPECT_EQ(nested.at(FieldId{1}).as_i32(), 10);
  ASSERT_FALSE(nested.contains(FieldId{2}));
  ASSERT_FALSE(barObject.contains(FieldId{2}));
}

TEST(FieldMaskTest, CopyException) {
  protocol::Object fooObject, barObject, bazObject;
  // bar{1: foo{2: 20}, 2: "40"}
  fooObject[FieldId{2}].emplace_i32() = 20;
  barObject[FieldId{1}].emplace_object() = fooObject;
  barObject[FieldId{2}].emplace_string() = "40";
  // baz{2: {3: 40}}
  bazObject[FieldId{2}].emplace_object()[FieldId{3}].emplace_i32() = 40;

  Mask m1; // bar[2] is not an object but has an object mask.
  m1.includes_ref().emplace()[2].includes_ref().emplace()[3] =
      protocol_constants::allMask();
  protocol::Object copy = barObject;
  EXPECT_THROW(protocol::copy(m1, copy, barObject), std::runtime_error);
  protocol::Object empty;
  EXPECT_THROW(protocol::copy(m1, barObject, empty), std::runtime_error);
  EXPECT_THROW(protocol::copy(m1, empty, barObject), std::runtime_error);
  // baz[2] is an object, but since bar[2] is not, it still throws an error.
  EXPECT_THROW(protocol::copy(m1, barObject, bazObject), std::runtime_error);
  EXPECT_THROW(protocol::copy(m1, bazObject, barObject), std::runtime_error);
}

TEST(FieldMaskTest, IsCompatibleWithSimple) {
  EXPECT_TRUE(protocol::is_compatible_with<Foo>(protocol_constants::allMask()));
  EXPECT_TRUE(
      protocol::is_compatible_with<Foo>(protocol_constants::noneMask()));

  Mask m;
  auto& includes = m.includes_ref().emplace();
  includes[1] = protocol_constants::allMask();
  EXPECT_TRUE(protocol::is_compatible_with<Foo>(m));
  includes[3] = protocol_constants::noneMask();
  EXPECT_TRUE(protocol::is_compatible_with<Foo>(m));

  includes[2] = protocol_constants::allMask(); // doesn't exist
  EXPECT_FALSE(protocol::is_compatible_with<Foo>(m));
}

TEST(FieldMaskTest, IsCompatibleWithNested) {
  EXPECT_TRUE(protocol::is_compatible_with<Bar>(protocol_constants::allMask()));
  EXPECT_TRUE(
      protocol::is_compatible_with<Bar>(protocol_constants::noneMask()));

  // These are valid field masks for Bar.
  Mask m;
  // includes{1: excludes{}}
  m.includes_ref().emplace()[1] = protocol_constants::allMask();
  EXPECT_TRUE(protocol::is_compatible_with<Bar>(m));
  // includes{2: includes{}}
  m.includes_ref().emplace()[2] = protocol_constants::noneMask();
  EXPECT_TRUE(protocol::is_compatible_with<Bar>(m));
  // includes{1: includes{1: excludes{}}, 2: excludes{}}
  auto& includes = m.includes_ref().emplace();
  includes[1].includes_ref().emplace()[1] = protocol_constants::allMask();
  includes[2] = protocol_constants::allMask();
  EXPECT_TRUE(protocol::is_compatible_with<Bar>(m));

  // There are invalid field masks for Bar.
  // includes{3: excludes{}}
  m.includes_ref().emplace()[3] = protocol_constants::allMask();
  EXPECT_FALSE(protocol::is_compatible_with<Bar>(m));
  // includes{2: includes{1: includes{}}}
  m.includes_ref().emplace()[2].includes_ref().emplace()[1] =
      protocol_constants::noneMask();
  EXPECT_FALSE(protocol::is_compatible_with<Bar>(m));
  // includes{1: excludes{2: excludes{}}}
  m.includes_ref().emplace()[1].excludes_ref().emplace()[2] =
      protocol_constants::allMask();
  EXPECT_FALSE(protocol::is_compatible_with<Bar>(m));
}

TEST(FieldMaskTest, IsCompatibleWithAdaptedField) {
  EXPECT_TRUE(protocol::is_compatible_with<Baz>(protocol_constants::allMask()));
  EXPECT_TRUE(
      protocol::is_compatible_with<Baz>(protocol_constants::noneMask()));

  Mask m;
  // includes{1: excludes{}}
  m.includes_ref().emplace()[1] = protocol_constants::allMask();
  EXPECT_TRUE(protocol::is_compatible_with<Baz>(m));
  // includes{1: includes{1: excludes{}}}
  m.includes_ref().emplace()[1].includes_ref().emplace()[1] =
      protocol_constants::allMask();
  // adapted struct field is treated as non struct field.
  EXPECT_FALSE(protocol::is_compatible_with<Baz>(m));
}

TEST(FieldMaskTest, Ensure) {
  Mask mask;
  // mask = includes{1: includes{2: excludes{}},
  //                 2: excludes{}}
  auto& includes = mask.includes_ref().emplace();
  includes[1].includes_ref().emplace()[2] = protocol_constants::allMask();
  includes[2] = protocol_constants::allMask();

  Bar2 bar;
  ensure(mask, bar);
  ASSERT_TRUE(bar.field_3().has_value());
  ASSERT_FALSE(bar.field_3()->field_1().has_value());
  ASSERT_TRUE(bar.field_3()->field_2().has_value());
  EXPECT_EQ(bar.field_3()->field_2(), 0);
  ASSERT_TRUE(bar.field_4().has_value());
  EXPECT_EQ(bar.field_4(), "");

  // mask = includes{1: includes{2: excludes{}},
  //                 2: includes{}}
  includes[2] = protocol_constants::noneMask();

  Bar2 bar2;
  ensure(mask, bar2);
  ASSERT_TRUE(bar2.field_3().has_value());
  ASSERT_FALSE(bar2.field_3()->field_1().has_value());
  ASSERT_TRUE(bar2.field_3()->field_2().has_value());
  EXPECT_EQ(bar2.field_3()->field_2(), 0);
  ASSERT_FALSE(bar2.field_4().has_value());

  // mask = excludes{1: includes{1: excludes{},
  //                             2: excludes{}}}
  auto& excludes = mask.excludes_ref().emplace();
  auto& nestedIncludes = excludes[1].includes_ref().emplace();
  nestedIncludes[1] = protocol_constants::allMask();
  nestedIncludes[2] = protocol_constants::allMask();

  Bar2 bar3;
  ensure(mask, bar3);
  ASSERT_TRUE(bar3.field_3().has_value());
  ASSERT_FALSE(bar3.field_3()->field_1().has_value());
  ASSERT_FALSE(bar3.field_3()->field_2().has_value());
  ASSERT_TRUE(bar3.field_4().has_value());
  EXPECT_EQ(bar3.field_4(), "");
}

TEST(FieldMaskTest, EnsureException) {
  Mask mask;
  // mask = includes{1: includes{2: excludes{1: includes{}}}}
  mask.includes_ref()
      .emplace()[1]
      .includes_ref()
      .emplace()[2]
      .excludes_ref()
      .emplace()[1] = protocol_constants::noneMask();
  Bar2 bar;
  EXPECT_THROW(ensure(mask, bar), std::runtime_error); // incompatible

  // includes{1: includes{1: excludes{}}}
  mask.includes_ref().emplace()[1].includes_ref().emplace()[1] =
      protocol_constants::allMask();
  Baz baz;
  // adapted field cannot be masked.
  EXPECT_THROW(ensure(mask, baz), std::runtime_error);
}
} // namespace apache::thrift::test
