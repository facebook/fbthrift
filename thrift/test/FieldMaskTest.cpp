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
#include <thrift/test/gen-cpp2/FieldMask_types.h>

using apache::thrift::protocol::allMask;
using apache::thrift::protocol::Mask;
using apache::thrift::protocol::MaskWrapper;
using apache::thrift::protocol::MaskWrapperInit;
using apache::thrift::protocol::noneMask;
using namespace apache::thrift::protocol::detail;

namespace apache::thrift::test {

bool literallyEqual(MaskRef actual, MaskRef expected) {
  return actual.mask == expected.mask &&
      actual.is_exclusion == expected.is_exclusion;
}

TEST(FieldMaskTest, ExampleFieldMask) {
  // includes{7: excludes{},
  //          9: includes{5: excludes{},
  //                      6: excludes{}}}
  Mask m;
  auto& includes = m.includes_ref().emplace();
  includes[7] = allMask();
  auto& nestedIncludes = includes[9].includes_ref().emplace();
  nestedIncludes[5] = allMask();
  nestedIncludes[6] = allMask();
  includes[8] = noneMask(); // not required
}
TEST(FieldMaskTest, ExampleMapMask) {
  // includes_map{7: excludes_map{},
  //              3: excludes{}}
  Mask m;
  auto& includes_map = m.includes_map_ref().emplace();
  includes_map[7].excludes_map_ref().emplace();
  includes_map[3] = allMask();
}

TEST(FieldMaskTest, Constant) {
  EXPECT_EQ(allMask().excludes_ref()->size(), 0);
  EXPECT_EQ(noneMask().includes_ref()->size(), 0);
}

TEST(FieldMaskTest, IsAllMask) {
  EXPECT_TRUE((MaskRef{allMask(), false}).isAllMask());
  EXPECT_TRUE((MaskRef{noneMask(), true}).isAllMask());
  EXPECT_FALSE((MaskRef{noneMask(), false}).isAllMask());
  EXPECT_FALSE((MaskRef{allMask(), true}).isAllMask());
  {
    Mask m;
    m.excludes_ref().emplace()[5] = allMask();
    EXPECT_FALSE((MaskRef{m, false}).isAllMask());
    EXPECT_FALSE((MaskRef{m, true}).isAllMask());
  }
  {
    Mask m;
    m.includes_map_ref().emplace()[3].includes_map_ref().emplace();
    EXPECT_FALSE((MaskRef{m, false}).isAllMask());
    EXPECT_FALSE((MaskRef{m, true}).isAllMask());
  }
}

TEST(FieldMaskTest, IsNoneMask) {
  EXPECT_TRUE((MaskRef{noneMask(), false}).isNoneMask());
  EXPECT_TRUE((MaskRef{allMask(), true}).isNoneMask());
  EXPECT_FALSE((MaskRef{allMask(), false}).isNoneMask());
  EXPECT_FALSE((MaskRef{noneMask(), true}).isNoneMask());
  {
    Mask m;
    m.excludes_ref().emplace()[5] = noneMask();
    EXPECT_FALSE((MaskRef{m, false}).isNoneMask());
    EXPECT_FALSE((MaskRef{m, true}).isNoneMask());
  }
  {
    Mask m;
    m.includes_map_ref().emplace()[3].includes_map_ref().emplace();
    EXPECT_FALSE((MaskRef{m, false}).isNoneMask());
    EXPECT_FALSE((MaskRef{m, true}).isNoneMask());
  }
}

TEST(FieldMaskTest, IsExclusive) {
  EXPECT_FALSE((MaskRef{noneMask(), false}).isExclusive());
  EXPECT_FALSE((MaskRef{allMask(), true}).isExclusive());
  EXPECT_TRUE((MaskRef{allMask(), false}).isExclusive());
  EXPECT_TRUE((MaskRef{noneMask(), true}).isExclusive());
  {
    Mask m;
    m.includes_ref().emplace()[5] = allMask();
    EXPECT_FALSE((MaskRef{m, false}).isExclusive());
    EXPECT_TRUE((MaskRef{m, true}).isExclusive());
  }
  {
    Mask m;
    m.excludes_ref().emplace()[5] = noneMask();
    EXPECT_TRUE((MaskRef{m, false}).isExclusive());
    EXPECT_FALSE((MaskRef{m, true}).isExclusive());
  }
  {
    Mask m;
    m.includes_map_ref().emplace()[5] = allMask();
    EXPECT_FALSE((MaskRef{m, false}).isExclusive());
    EXPECT_TRUE((MaskRef{m, true}).isExclusive());
  }
  {
    Mask m;
    m.excludes_map_ref().emplace()[5] = noneMask();
    EXPECT_TRUE((MaskRef{m, false}).isExclusive());
    EXPECT_FALSE((MaskRef{m, true}).isExclusive());
  }
}

TEST(FieldMaskTest, MaskRefIsMask) {
  EXPECT_TRUE((MaskRef{allMask(), false}).isFieldMask());
  EXPECT_FALSE((MaskRef{allMask(), false}).isMapMask());
  EXPECT_TRUE((MaskRef{noneMask(), true}).isFieldMask());
  EXPECT_FALSE((MaskRef{noneMask(), true}).isMapMask());
  {
    Mask m;
    m.includes_ref().emplace()[5] = allMask();
    EXPECT_TRUE((MaskRef{m, false}).isFieldMask());
    EXPECT_FALSE((MaskRef{m, true}).isMapMask());
  }
  {
    Mask m;
    m.excludes_ref().emplace()[5] = noneMask();
    EXPECT_TRUE((MaskRef{m, false}).isFieldMask());
    EXPECT_FALSE((MaskRef{m, true}).isMapMask());
  }
  {
    Mask m;
    m.includes_map_ref().emplace()[5] = allMask();
    EXPECT_FALSE((MaskRef{m, false}).isFieldMask());
    EXPECT_TRUE((MaskRef{m, true}).isMapMask());
  }
  {
    Mask m;
    m.excludes_map_ref().emplace()[5] = noneMask();
    EXPECT_FALSE((MaskRef{m, false}).isFieldMask());
    EXPECT_TRUE((MaskRef{m, true}).isMapMask());
  }
}

TEST(FieldMaskTest, MaskRefGetIncludes) {
  Mask m;
  // includes{8: excludes{},
  //          9: includes{4: excludes{}}
  auto& includes = m.includes_ref().emplace();
  includes[8] = allMask();
  includes[9].includes_ref().emplace()[4] = allMask();

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
  excludes[8] = allMask();
  excludes[9].includes_ref().emplace()[4] = allMask();

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

TEST(FieldMaskTest, SchemalessClear) {
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
  includes[2] = allMask();
  auto& nestedExcludes = includes[1].excludes_ref().emplace();
  nestedExcludes[5].excludes_ref().emplace()[5] =
      allMask(); // The object doesn't have this field.
  nestedExcludes[1] = allMask();
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

TEST(FieldMaskTest, SchemalessClearException) {
  protocol::Object bazObject;
  // bar{2: "40"}
  bazObject[FieldId{2}].emplace_string() = "40";

  Mask m1; // object[2] is not an object but has an object mask.
  auto& includes = m1.includes_ref().emplace();
  includes[2].includes_ref().emplace()[4] = noneMask();
  EXPECT_THROW(protocol::clear(m1, bazObject), std::runtime_error);

  protocol::Object fooObject, barObject;
  // bar{1: foo{2: 20}, 2: "40"}
  fooObject[FieldId{2}].emplace_i32() = 20;
  barObject[FieldId{1}].emplace_object() = fooObject;
  barObject[FieldId{2}].emplace_string() = "40";

  Mask m2; // object[1][2] is not an object but has an object mask.
  auto& includes2 = m2.includes_ref().emplace();
  includes2[1].includes_ref().emplace()[2].excludes_ref().emplace()[5] =
      allMask();
  includes2[2] = allMask();
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

TEST(FieldMaskTest, SchemalessCopySimpleIncludes) {
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
  includes[1] = allMask();
  testCopy(m, fooObject, barObject);
  // bar becomes bar{1: 10, 2: 20}
  ASSERT_TRUE(barObject.contains(FieldId{1}));
  EXPECT_EQ(barObject.at(FieldId{1}).as_i32(), 10);
  ASSERT_TRUE(barObject.contains(FieldId{2}));
  EXPECT_EQ(barObject.at(FieldId{2}).as_i32(), 20);

  testCopy(allMask(), fooObject, bazObject);
  // baz becomes baz{1: 10}
  ASSERT_TRUE(bazObject.contains(FieldId{1}));
  EXPECT_EQ(bazObject.at(FieldId{1}).as_i32(), 10);
  ASSERT_FALSE(bazObject.contains(FieldId{2}));

  // includes{1: exludes{}, 2: excludes{}, 3: excludes{}}
  includes[2] = allMask();
  includes[3] = allMask(); // no-op
  testCopy(m, barObject, bazObject);
  // baz becomes baz{1: 10, 2: 20}
  ASSERT_TRUE(bazObject.contains(FieldId{1}));
  EXPECT_EQ(bazObject.at(FieldId{1}).as_i32(), 10);
  ASSERT_TRUE(bazObject.contains(FieldId{2}));
  EXPECT_EQ(bazObject.at(FieldId{2}).as_i32(), 20);
  ASSERT_FALSE(bazObject.contains(FieldId{3}));
}

TEST(FieldMaskTest, SchemalessCopySimpleExcludes) {
  protocol::Object fooObject, barObject;
  // foo{1: 10, 3: 40}
  // bar{1: "30", 2: 20}
  fooObject[FieldId{1}].emplace_i32() = 10;
  fooObject[FieldId{3}].emplace_i32() = 40;
  barObject[FieldId{1}].emplace_string() = "30";
  barObject[FieldId{2}].emplace_i32() = 20;

  Mask m;
  // excludes{1: exludes{}}
  m.excludes_ref().emplace()[1] = allMask();
  testCopy(m, fooObject, barObject);
  // bar becomes bar{1: "30", 3: 40}
  ASSERT_TRUE(barObject.contains(FieldId{1}));
  EXPECT_EQ(barObject.at(FieldId{1}).as_string(), "30");
  ASSERT_TRUE(barObject.contains(FieldId{3}));
  EXPECT_EQ(barObject.at(FieldId{3}).as_i32(), 40);
  ASSERT_FALSE(barObject.contains(FieldId{2}));
}

TEST(FieldMaskTest, SchemalessCopyNestedRecursive) {
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
  m.excludes_ref().emplace()[1].excludes_ref().emplace()[1] = allMask();
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

TEST(FieldMaskTest, SchemalessCopyNestedAddField) {
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
  includes[1].includes_ref().emplace()[2] = allMask();
  includes[2] = allMask();

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
  nestedIncludes[1] = allMask();
  nestedIncludes[2] = allMask();
  nestedIncludes[3] = allMask();
  excludes[2] = noneMask();

  protocol::Object dst2;
  testCopy(m2, barObject, dst2);
  // dst2 becomes dst2{2: "40"} (doesn't create nested object).
  ASSERT_FALSE(dst2.contains(FieldId{1}));
  ASSERT_TRUE(dst2.contains(FieldId{2}));
  EXPECT_EQ(dst2.at(FieldId{2}).as_string(), "40");
}

TEST(FieldMaskTest, SchemalessCopyNestedRemoveField) {
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
  includes[1].includes_ref().emplace()[2] = allMask();
  includes[2] = allMask();

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

TEST(FieldMaskTest, SchemalessCopyException) {
  protocol::Object fooObject, barObject, bazObject;
  // bar{1: foo{2: 20}, 2: "40"}
  fooObject[FieldId{2}].emplace_i32() = 20;
  barObject[FieldId{1}].emplace_object() = fooObject;
  barObject[FieldId{2}].emplace_string() = "40";
  // baz{2: {3: 40}}
  bazObject[FieldId{2}].emplace_object()[FieldId{3}].emplace_i32() = 40;

  Mask m1; // bar[2] is not an object but has an object mask.
  m1.includes_ref().emplace()[2].includes_ref().emplace()[3] = allMask();
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
  EXPECT_TRUE(protocol::is_compatible_with<Foo>(allMask()));
  EXPECT_TRUE(protocol::is_compatible_with<Foo>(noneMask()));

  Mask m;
  auto& includes = m.includes_ref().emplace();
  includes[1] = allMask();
  EXPECT_TRUE(protocol::is_compatible_with<Foo>(m));
  includes[3] = noneMask();
  EXPECT_TRUE(protocol::is_compatible_with<Foo>(m));

  includes[2] = allMask(); // doesn't exist
  EXPECT_FALSE(protocol::is_compatible_with<Foo>(m));
}

TEST(FieldMaskTest, IsCompatibleWithNested) {
  EXPECT_TRUE(protocol::is_compatible_with<Bar>(allMask()));
  EXPECT_TRUE(protocol::is_compatible_with<Bar>(noneMask()));

  // These are valid field masks for Bar.
  Mask m;
  // includes{1: excludes{}}
  m.includes_ref().emplace()[1] = allMask();
  EXPECT_TRUE(protocol::is_compatible_with<Bar>(m));
  // includes{2: includes{}}
  m.includes_ref().emplace()[2] = noneMask();
  EXPECT_TRUE(protocol::is_compatible_with<Bar>(m));
  // includes{1: includes{1: excludes{}}, 2: excludes{}}
  auto& includes = m.includes_ref().emplace();
  includes[1].includes_ref().emplace()[1] = allMask();
  includes[2] = allMask();
  EXPECT_TRUE(protocol::is_compatible_with<Bar>(m));

  // There are invalid field masks for Bar.
  // includes{3: excludes{}}
  m.includes_ref().emplace()[3] = allMask();
  EXPECT_FALSE(protocol::is_compatible_with<Bar>(m));
  // includes{2: includes{1: includes{}}}
  m.includes_ref().emplace()[2].includes_ref().emplace()[1] = noneMask();
  EXPECT_FALSE(protocol::is_compatible_with<Bar>(m));
  // includes{1: excludes{2: excludes{}}}
  m.includes_ref().emplace()[1].excludes_ref().emplace()[2] = allMask();
  EXPECT_FALSE(protocol::is_compatible_with<Bar>(m));
}

TEST(FieldMaskTest, IsCompatibleWithAdaptedField) {
  EXPECT_TRUE(protocol::is_compatible_with<Baz>(allMask()));
  EXPECT_TRUE(protocol::is_compatible_with<Baz>(noneMask()));

  Mask m;
  // includes{1: excludes{}}
  m.includes_ref().emplace()[1] = allMask();
  EXPECT_TRUE(protocol::is_compatible_with<Baz>(m));
  // includes{1: includes{1: excludes{}}}
  m.includes_ref().emplace()[1].includes_ref().emplace()[1] = allMask();
  // adapted struct field is treated as non struct field.
  EXPECT_FALSE(protocol::is_compatible_with<Baz>(m));
}

TEST(FieldMaskTest, Ensure) {
  Mask mask;
  // mask = includes{1: includes{2: excludes{}},
  //                 2: excludes{}}
  auto& includes = mask.includes_ref().emplace();
  includes[1].includes_ref().emplace()[2] = allMask();
  includes[2] = allMask();

  {
    Bar2 bar;
    ensure(mask, bar);
    ASSERT_TRUE(bar.field_3().has_value());
    ASSERT_FALSE(bar.field_3()->field_1().has_value());
    ASSERT_TRUE(bar.field_3()->field_2().has_value());
    EXPECT_EQ(bar.field_3()->field_2(), 0);
    ASSERT_TRUE(bar.field_4().has_value());
    EXPECT_EQ(bar.field_4(), "");
  }

  // mask = includes{1: includes{2: excludes{}},
  //                 2: includes{}}
  includes[2] = noneMask();

  {
    Bar2 bar;
    ensure(mask, bar);
    ASSERT_TRUE(bar.field_3().has_value());
    ASSERT_FALSE(bar.field_3()->field_1().has_value());
    ASSERT_TRUE(bar.field_3()->field_2().has_value());
    EXPECT_EQ(bar.field_3()->field_2(), 0);
    ASSERT_FALSE(bar.field_4().has_value());
  }

  // test excludes mask
  // mask = excludes{1: includes{1: excludes{},
  //                             2: excludes{}}}
  auto& excludes = mask.excludes_ref().emplace();
  auto& nestedIncludes = excludes[1].includes_ref().emplace();
  nestedIncludes[1] = allMask();
  nestedIncludes[2] = allMask();

  {
    Bar2 bar;
    ensure(mask, bar);
    ASSERT_TRUE(bar.field_3().has_value());
    ASSERT_FALSE(bar.field_3()->field_1().has_value());
    ASSERT_FALSE(bar.field_3()->field_2().has_value());
    ASSERT_TRUE(bar.field_4().has_value());
    EXPECT_EQ(bar.field_4(), "");
  }
}

TEST(FieldMaskTest, EnsureException) {
  Mask mask;
  // mask = includes{1: includes{2: excludes{1: includes{}}}}
  mask.includes_ref()
      .emplace()[1]
      .includes_ref()
      .emplace()[2]
      .excludes_ref()
      .emplace()[1] = noneMask();
  Bar2 bar;
  EXPECT_THROW(ensure(mask, bar), std::runtime_error); // incompatible

  // includes{1: includes{1: excludes{}}}
  mask.includes_ref().emplace()[1].includes_ref().emplace()[1] = allMask();
  Baz baz;
  // adapted field cannot be masked.
  EXPECT_THROW(ensure(mask, baz), std::runtime_error);
}

TEST(FieldMaskTest, SchemafulClear) {
  Mask mask;
  // mask = includes{1: includes{2: excludes{}},
  //                 2: excludes{}}
  auto& includes = mask.includes_ref().emplace();
  includes[1].includes_ref().emplace()[2] = allMask();
  includes[2] = allMask();

  {
    Bar2 bar;
    ensure(mask, bar);
    protocol::clear(mask, bar); // clear fields that are ensured
    ASSERT_TRUE(bar.field_3().has_value());
    ASSERT_FALSE(bar.field_3()->field_1().has_value());
    ASSERT_FALSE(bar.field_3()->field_2().has_value());
    ASSERT_TRUE(bar.field_4().has_value());
    EXPECT_EQ(bar.field_4().value(), "");
  }

  {
    Bar2 bar;
    protocol::clear(mask, bar); // no-op
    ASSERT_FALSE(bar.field_3().has_value());
    ASSERT_FALSE(bar.field_4().has_value());
  }

  {
    Bar2 bar;
    bar.field_4() = "Hello";
    protocol::clear(noneMask(), bar); // no-op
    ASSERT_TRUE(bar.field_4().has_value());
    EXPECT_EQ(bar.field_4().value(), "Hello");

    protocol::clear(allMask(), bar);
    ASSERT_TRUE(bar.field_4().has_value());
    EXPECT_EQ(bar.field_4().value(), "");
  }

  // test optional fields
  {
    Bar2 bar;
    bar.field_3().ensure();
    protocol::clear(allMask(), bar);
    EXPECT_FALSE(bar.field_3().has_value());
  }
  {
    Bar2 bar;
    bar.field_3().ensure();
    bar.field_3()->field_1() = 1;
    bar.field_3()->field_2() = 2;
    protocol::clear(mask, bar);
    ASSERT_TRUE(bar.field_3().has_value());
    ASSERT_TRUE(bar.field_3()->field_1().has_value());
    EXPECT_EQ(bar.field_3()->field_1(), 1);
    ASSERT_FALSE(bar.field_3()->field_2().has_value());
    ASSERT_FALSE(bar.field_4().has_value());

    protocol::clear(allMask(), bar);
    ASSERT_FALSE(bar.field_3().has_value());
    ASSERT_FALSE(bar.field_4().has_value());
  }
}

TEST(FieldMaskTest, SchemafulClearExcludes) {
  // tests excludes mask
  // mask2 = excludes{1: includes{1: excludes{}}}
  Mask mask;
  auto& excludes = mask.excludes_ref().emplace();
  excludes[1].includes_ref().emplace()[1] = allMask();
  Bar bar;
  bar.foo().emplace().field1() = 1;
  bar.foo()->field2() = 2;
  protocol::clear(mask, bar);
  ASSERT_TRUE(bar.foo().has_value());
  ASSERT_TRUE(bar.foo()->field1().has_value());
  EXPECT_EQ(bar.foo()->field1(), 1);
  ASSERT_TRUE(bar.foo()->field2().has_value());
  EXPECT_EQ(bar.foo()->field2(), 0);
  ASSERT_FALSE(bar.foos().has_value());
}

TEST(FieldMaskTest, SchemafulClearEdgeCase) {
  // Clear sets the field to intristic default even if the field isn't set.
  CustomDefault obj;
  protocol::clear(allMask(), obj);
  EXPECT_EQ(obj.field(), "");

  // Makes sure clear doesn't use has_value() for all fields.
  TerseWrite obj2;
  protocol::clear(allMask(), obj2);

  Mask m;
  m.includes_ref().emplace()[2].includes_ref().emplace()[1] = allMask();
  protocol::clear(m, obj2);
}

TEST(FieldMaskTest, SchemafulClearException) {
  Bar2 bar;
  Mask m1; // m1 = includes{2: includes{4: includes{}}}
  auto& includes = m1.includes_ref().emplace();
  includes[2].includes_ref().emplace()[4] = noneMask();
  EXPECT_THROW(protocol::clear(m1, bar), std::runtime_error);

  Mask m2; // m2 = includes{1: includes{2: includes{5: excludes{}}}}
  auto& includes2 = m2.includes_ref().emplace();
  includes2[1].includes_ref().emplace()[2].excludes_ref().emplace()[5] =
      allMask();
  includes2[2] = allMask();
  EXPECT_THROW(protocol::clear(m2, bar), std::runtime_error);
}

TEST(FieldMaskTest, SchemafulCopy) {
  Bar2 src, empty;
  // src = {1: {1: 10, 2: 20}, 2: "40"}
  src.field_3().ensure().field_1() = 10;
  src.field_3()->field_2() = 20;
  src.field_4() = "40";

  Mask mask;
  // includes{1: includes{2: excludes{}},
  //          2: excludes{}}
  auto& includes = mask.includes_ref().emplace();
  includes[1].includes_ref().emplace()[2] = allMask();
  includes[2] = allMask();

  // copy empty
  {
    Bar2 dst;
    copy(mask, empty, dst);
    EXPECT_EQ(dst, empty);
  }

  // test copy field to dst
  {
    Bar2 dst;
    dst.field_3().ensure().field_1() = 30;
    dst.field_3()->field_2() = 40;
    copy(mask, src, dst);
    ASSERT_TRUE(dst.field_3().has_value());
    ASSERT_TRUE(dst.field_3()->field_1().has_value());
    EXPECT_EQ(dst.field_3()->field_1().value(), 30);
    ASSERT_TRUE(dst.field_3()->field_2().has_value());
    EXPECT_EQ(dst.field_3()->field_2().value(), 20);
    ASSERT_TRUE(dst.field_4().has_value());
    EXPECT_EQ(dst.field_4(), "40");
  }

  // test add field to dst
  {
    Bar2 dst;
    copy(mask, src, dst);
    ASSERT_TRUE(dst.field_3().has_value());
    ASSERT_FALSE(dst.field_3()->field_1().has_value());
    ASSERT_TRUE(dst.field_3()->field_2().has_value());
    EXPECT_EQ(dst.field_3()->field_2().value(), 20);
    EXPECT_EQ(dst.field_4(), "40");
  }

  // test remove field from src
  {
    Bar2 dst;
    copy(mask, empty, src);
    // src = {1: {1: 10}, 2: "40"}
    ASSERT_TRUE(src.field_3().has_value());
    ASSERT_TRUE(src.field_3()->field_1().has_value());
    EXPECT_EQ(src.field_3()->field_1().value(), 10);
    ASSERT_FALSE(src.field_3()->field_2().has_value());
    EXPECT_EQ(src.field_4(), "");

    src.field_4() = "40";
    copy(mask, src, dst); // this does not create a new object.
    ASSERT_FALSE(dst.field_3().has_value());
    ASSERT_TRUE(dst.field_4().has_value());
    EXPECT_EQ(dst.field_4().value(), "40");

    copy(allMask(), empty, src);
    ASSERT_FALSE(src.field_3().has_value());
    EXPECT_EQ(src.field_4(), "");
  }
}
TEST(FieldMaskTest, SchemafulCopyExcludes) {
  // test excludes mask
  // mask2 = excludes{1: includes{1: excludes{}}}
  Mask mask;
  auto& excludes = mask.excludes_ref().emplace();
  excludes[1].includes_ref().emplace()[1] = allMask();
  Bar src, dst;
  src.foo().emplace().field1() = 1;
  src.foo()->field2() = 2;
  protocol::copy(mask, src, dst);
  ASSERT_FALSE(dst.foo()->field1().has_value());
  ASSERT_TRUE(dst.foo()->field2().has_value());
  EXPECT_EQ(dst.foo()->field2(), 2);
  ASSERT_FALSE(dst.foos().has_value());
}

TEST(FieldMaskTest, SchemafulCopyTerseWrite) {
  // Makes sure copy doesn't use has_value() for all fields.
  TerseWrite src, dst;
  src.field() = 4;
  src.foo()->field1() = 5;
  protocol::copy(allMask(), src, dst);
  EXPECT_EQ(dst, src);

  Mask m;
  m.includes_ref().emplace()[2].includes_ref().emplace()[1] = allMask();
  protocol::copy(m, src, dst);
  EXPECT_EQ(dst, src);
}

TEST(FieldMaskTest, SchemafulCopyException) {
  Bar2 src, dst;
  Mask m1; // m1 = includes{2: includes{4: includes{}}}
  auto& includes = m1.includes_ref().emplace();
  includes[2].includes_ref().emplace()[4] = noneMask();
  EXPECT_THROW(protocol::copy(m1, src, dst), std::runtime_error);

  Mask m2; // m2 = includes{1: includes{2: includes{5: excludes{}}}}
  auto& includes2 = m2.includes_ref().emplace();
  includes2[1].includes_ref().emplace()[2].excludes_ref().emplace()[5] =
      allMask();
  includes2[2] = allMask();
  EXPECT_THROW(protocol::copy(m2, src, dst), std::runtime_error);
}

TEST(FIeldMaskTest, LogicalOpSimple) {
  // maskA = includes{1: excludes{},
  //                  2: excludes{},
  //                  3: includes{}}
  Mask maskA;
  {
    auto& includes = maskA.includes_ref().emplace();
    includes[1] = allMask();
    includes[2] = allMask();
    includes[3] = noneMask();
  }

  // maskB = includes{2: excludes{},
  //                  3: excludes{}}
  Mask maskB;
  {
    auto& includes = maskB.includes_ref().emplace();
    includes[2] = allMask();
    includes[3] = allMask();
  }

  // maskA | maskB == includes{1: excludes{},
  //                           2: excludes{},
  //                           3: excludes{}}
  Mask maskUnion;
  {
    auto& includes = maskUnion.includes_ref().emplace();
    includes[1] = allMask();
    includes[2] = allMask();
    includes[3] = allMask();
  }
  EXPECT_EQ(maskA | maskB, maskUnion);
  EXPECT_EQ(maskB | maskA, maskUnion);

  // maskA & maskB == includes{2: excludes{}}
  Mask maskIntersect;
  { maskIntersect.includes_ref().emplace()[2] = allMask(); }
  EXPECT_EQ(maskA & maskB, maskIntersect);
  EXPECT_EQ(maskB & maskA, maskIntersect);

  // maskA - maskB == includes{1: excludes{}}
  Mask maskSubtractAB;
  { maskSubtractAB.includes_ref().emplace()[1] = allMask(); }
  EXPECT_EQ(maskA - maskB, maskSubtractAB);

  // maskB - maskA == includes{3: excludes{}}
  Mask maskSubtractBA;
  { maskSubtractBA.includes_ref().emplace()[3] = allMask(); }
  EXPECT_EQ(maskB - maskA, maskSubtractBA);
}

TEST(FieldMaskTest, LogicalOpBothIncludes) {
  // maskA = includes{1: includes{2: excludes{}},
  //                  3: includes{4: excludes{},
  //                              5: excludes{}}}
  Mask maskA;
  {
    auto& includes = maskA.includes_ref().emplace();
    includes[1].includes_ref().emplace()[2] = allMask();
    auto& includes2 = includes[3].includes_ref().emplace();
    includes2[4] = allMask();
    includes2[5] = allMask();
  }

  // maskB = includes{1: excludes{},
  //                  3: includes{5: excludes{},
  //                              6: excludes{}},
  //                  7: excludes{}}
  Mask maskB;
  {
    auto& includes = maskB.includes_ref().emplace();
    includes[1] = allMask();
    auto& includes2 = includes[3].includes_ref().emplace();
    includes2[5] = allMask();
    includes2[6] = allMask();
    includes[7] = allMask();
  }

  // maskA | maskB == includes{1: excludes{},
  //                           3: includes{4: excludes{},
  //                                       5: excludes{},
  //                                       6: excludes{}},
  //                           7: excludes{}}
  Mask maskUnion;
  {
    auto& includes = maskUnion.includes_ref().emplace();
    includes[1] = allMask();
    auto& includes2 = includes[3].includes_ref().emplace();
    includes2[4] = allMask();
    includes2[5] = allMask();
    includes2[6] = allMask();
    includes[7] = allMask();
  }
  EXPECT_EQ(maskA | maskB, maskUnion);
  EXPECT_EQ(maskB | maskA, maskUnion);

  // maskA & maskB == includes{1: includes{2: excludes{}},
  //                           3: includes{5: excludes{}}}
  Mask maskIntersect;
  {
    auto& includes = maskIntersect.includes_ref().emplace();
    includes[1].includes_ref().emplace()[2] = allMask();
    includes[3].includes_ref().emplace()[5] = allMask();
  }
  EXPECT_EQ(maskA & maskB, maskIntersect);
  EXPECT_EQ(maskB & maskA, maskIntersect);

  // maskA - maskB == includes{3: includes{4: excludes{}}}
  Mask maskSubtractAB;
  {
    maskSubtractAB.includes_ref().emplace()[3].includes_ref().emplace()[4] =
        allMask();
  }
  EXPECT_EQ(maskA - maskB, maskSubtractAB);

  // maskB - maskA == includes{1: excludes{2: excludes{}},
  //                           3: includes{6: excludes{}},
  //                           7: excludes{}}
  Mask maskSubtractBA;
  {
    auto& includes = maskSubtractBA.includes_ref().emplace();
    includes[1].excludes_ref().emplace()[2] = allMask();
    auto& includes2 = includes[3].includes_ref().emplace();
    includes2[6] = allMask();
    includes[7] = allMask();
  }
  EXPECT_EQ(maskB - maskA, maskSubtractBA);
}

TEST(FieldMaskTest, LogicalOpBothExcludes) {
  // maskA = excludes{1: excludes{2: excludes{}},
  //                  3: includes{4: includes{},
  //                              5: excludes{}}}
  Mask maskA;
  {
    auto& excludes = maskA.excludes_ref().emplace();
    excludes[1].excludes_ref().emplace()[2] = allMask();
    auto& includes = excludes[3].includes_ref().emplace();
    includes[4] = noneMask();
    includes[5] = allMask();
  }

  // maskB = excludes{1: includes{},
  //                  3: includes{5: excludes{},
  //                              6: excludes{}},
  //                  7: excludes{}}
  Mask maskB;
  {
    auto& excludes = maskB.excludes_ref().emplace();
    excludes[1] = noneMask();
    auto& includes = excludes[3].includes_ref().emplace();
    includes[5] = allMask();
    includes[6] = allMask();
    excludes[7] = allMask();
  }

  // maskA | maskB == excludes{3: includes{5: excludes{}}}
  Mask maskUnion;
  {
    maskUnion.excludes_ref().emplace()[3].includes_ref().emplace()[5] =
        allMask();
  }
  EXPECT_EQ(maskA | maskB, maskUnion);
  EXPECT_EQ(maskB | maskA, maskUnion);

  // maskA & maskB == excludes{1: excludes{2: excludes{}},
  //                           3: includes{5: excludes{},
  //                                       6: excludes{}},
  //                           7: excludes{}}
  Mask maskIntersect;
  {
    auto& excludes = maskIntersect.excludes_ref().emplace();
    excludes[1].excludes_ref().emplace()[2] = allMask();
    auto& includes = excludes[3].includes_ref().emplace();
    includes[5] = allMask();
    includes[6] = allMask();
    excludes[7] = allMask();
  }
  EXPECT_EQ(maskA & maskB, maskIntersect);
  EXPECT_EQ(maskB & maskA, maskIntersect);

  // maskA - maskB == includes{3: includes{6: excludes{}},
  //                           7: excludes{}}
  Mask maskSubtractAB;
  {
    auto& includes = maskSubtractAB.includes_ref().emplace();
    includes[3].includes_ref().emplace()[6] = allMask();
    includes[7] = allMask();
  }
  EXPECT_EQ(maskA - maskB, maskSubtractAB);

  // maskB - maskA == includes{1: excludes{2: excludes{}}}
  Mask maskSubtractBA;
  {
    maskSubtractBA.includes_ref().emplace()[1].excludes_ref().emplace()[2] =
        allMask();
  }
  EXPECT_EQ(maskB - maskA, maskSubtractBA);
}

TEST(FieldMaskTest, LogicalOpIncludesExcludes) {
  // maskA = includes{1: includes{2: excludes{}},
  //                  3: includes{4: excludes{},
  //                              5: excludes{}}}
  Mask maskA;
  {
    auto& includes = maskA.includes_ref().emplace();
    includes[1].includes_ref().emplace()[2] = allMask();
    auto& includes2 = includes[3].includes_ref().emplace();
    includes2[4] = allMask();
    includes2[5] = allMask();
  }

  // maskB = excludes{1: includes{},
  //                  3: includes{5: excludes{},
  //                              6: excludes{}},
  //                  7: excludes{}}
  Mask maskB;
  {
    auto& excludes = maskB.excludes_ref().emplace();
    excludes[1] = noneMask();
    auto& includes = excludes[3].includes_ref().emplace();
    includes[5] = allMask();
    includes[6] = allMask();
    excludes[7] = allMask();
  }

  // maskA | maskB == excludes{3: includes{6: excludes{}},
  //                           7: excludes{}}
  Mask maskUnion;
  {
    auto& excludes = maskUnion.excludes_ref().emplace();
    excludes[3].includes_ref().emplace()[6] = allMask();
    excludes[7] = allMask();
  }
  EXPECT_EQ(maskA | maskB, maskUnion);
  EXPECT_EQ(maskB | maskA, maskUnion);

  // maskA & maskB == includes{1: includes{2: excludes{}},
  //                           3: includes{4: excludes{}}}
  Mask maskIntersect;
  {
    auto& includes = maskIntersect.includes_ref().emplace();
    includes[1].includes_ref().emplace()[2] = allMask();
    includes[3].includes_ref().emplace()[4] = allMask();
  }
  EXPECT_EQ(maskA & maskB, maskIntersect);
  EXPECT_EQ(maskB & maskA, maskIntersect);

  // maskA - maskB == includes{3: includes{5: excludes{}}}
  Mask maskSubtractAB;
  {
    maskSubtractAB.includes_ref().emplace()[3].includes_ref().emplace()[5] =
        allMask();
  }
  EXPECT_EQ(maskA - maskB, maskSubtractAB);

  // maskB - maskA == excludes{1: includes{2: excludes{}},
  //                           3: includes{4: excludes{},
  //                                       5: excludes{},
  //                                       6: excludes{}},
  //                           7: excludes{}}
  Mask maskSubtractBA;
  {
    auto& excludes = maskSubtractBA.excludes_ref().emplace();
    excludes[1].includes_ref().emplace()[2] = allMask();
    auto& includes = excludes[3].includes_ref().emplace();
    includes[4] = allMask();
    includes[5] = allMask();
    includes[6] = allMask();
    excludes[7] = allMask();
  }
  EXPECT_EQ(maskB - maskA, maskSubtractBA);
}

TEST(FieldMaskTest, MaskWrapperSimple) {
  {
    MaskWrapper<Foo> wrapper(MaskWrapperInit::none);
    EXPECT_EQ(wrapper.toThrift(), noneMask());
    wrapper.excludes();
    EXPECT_EQ(wrapper.toThrift(), noneMask());
    wrapper.includes();
    EXPECT_EQ(wrapper.toThrift(), allMask());
    wrapper.excludes(noneMask());
    EXPECT_EQ(wrapper.toThrift(), allMask());
  }
  {
    MaskWrapper<Foo> wrapper(MaskWrapperInit::all);
    EXPECT_EQ(wrapper.toThrift(), allMask());
    wrapper.includes();
    EXPECT_EQ(wrapper.toThrift(), allMask());
    wrapper.excludes();
    EXPECT_EQ(wrapper.toThrift(), noneMask());
    wrapper.includes(noneMask());
    EXPECT_EQ(wrapper.toThrift(), noneMask());
  }

  // Test includes
  {
    // includes{1: excludes{},
    //          3: excludes{}}
    MaskWrapper<Foo> wrapper(MaskWrapperInit::none);
    wrapper.includes<tag::field1>();
    wrapper.includes<tag::field2>();
    Mask expected;
    auto& includes = expected.includes_ref().emplace();
    includes[1] = allMask();
    includes[3] = allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
  {
    MaskWrapper<Foo> wrapper(MaskWrapperInit::all);
    wrapper.includes<tag::field1>();
    wrapper.includes<tag::field2>();
    EXPECT_EQ(wrapper.toThrift(), allMask());
  }
  {
    // includes{1: excludes{}}
    MaskWrapper<Foo> wrapper(MaskWrapperInit::none);
    wrapper.includes<tag::field1>();
    wrapper.includes<tag::field1>(); // including twice is fine
    Mask expected;
    expected.includes_ref().emplace()[1] = allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }

  // Test excludes
  {
    // excludes{3: excludes{}}
    MaskWrapper<Foo> wrapper(MaskWrapperInit::all);
    wrapper.excludes<tag::field1>(noneMask());
    wrapper.excludes<tag::field2>();
    Mask expected;
    expected.excludes_ref().emplace()[3] = allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
  {
    MaskWrapper<Foo> wrapper(MaskWrapperInit::none);
    wrapper.excludes<tag::field1>();
    wrapper.excludes<tag::field2>();
    EXPECT_EQ(wrapper.toThrift(), noneMask());
  }
  {
    // excludes{3: excludes{}}
    MaskWrapper<Foo> wrapper(MaskWrapperInit::all);
    wrapper.excludes<tag::field2>();
    wrapper.excludes<tag::field2>(); // excluding twice is fine
    Mask expected;
    expected.excludes_ref().emplace()[3] = allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }

  // Test includes and excludes
  {
    MaskWrapper<Foo> wrapper(MaskWrapperInit::none);
    wrapper.includes<tag::field2>();
    wrapper.excludes<tag::field2>(); // excluding the field we included
    EXPECT_EQ(wrapper.toThrift(), noneMask());
  }
  {
    MaskWrapper<Foo> wrapper(MaskWrapperInit::all);
    wrapper.excludes<tag::field2>();
    wrapper.includes<tag::field2>(); // including the field we excluded
    EXPECT_EQ(wrapper.toThrift(), allMask());
  }
}

TEST(FieldMaskTest, MaskWrapperNested) {
  // Test includes
  {
    // includes{1: includes{1: excludes{}}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::none);
    wrapper.includes<tag::field_3, tag::field_1>();
    Mask expected;
    expected.includes_ref().emplace()[1].includes_ref().emplace()[1] =
        allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
  {
    // includes{1: includes{1: excludes{}}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::none);
    wrapper.includes<tag::field_3, tag::field_1>();
    wrapper.includes<tag::field_3, tag::field_1>(); // including twice is fine
    Mask expected;
    expected.includes_ref().emplace()[1].includes_ref().emplace()[1] =
        allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
  {
    // includes{1: includes{1: excludes{}}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::none);
    Mask nestedMask;
    nestedMask.includes_ref().emplace()[1] = allMask();
    wrapper.includes<tag::field_3>(nestedMask);
    Mask expected;
    expected.includes_ref().emplace()[1].includes_ref().emplace()[1] =
        allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
  {
    // includes{1: includes{1: excludes{},
    //                      2: excludes{}},
    //          2: excludes{}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::none);
    wrapper.includes<tag::field_4>();
    wrapper.includes<tag::field_3, tag::field_1>();
    wrapper.includes<tag::field_3, tag::field_2>();
    Mask expected;
    auto& includes = expected.includes_ref().emplace();
    includes[2] = allMask();
    auto& includes2 = includes[1].includes_ref().emplace();
    includes2[1] = allMask();
    includes2[2] = allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
  {
    // includes{1: excludes{}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::none);
    wrapper.includes<tag::field_3, tag::field_1>();
    wrapper.includes<tag::field_3>();
    Mask expected;
    expected.includes_ref().emplace()[1] = allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }

  // Test excludes
  {
    // excludes{1: includes{1: excludes{}}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::all);
    wrapper.excludes<tag::field_3, tag::field_1>();
    Mask expected;
    expected.excludes_ref().emplace()[1].includes_ref().emplace()[1] =
        allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
  {
    // excludes{1: includes{1: excludes{}}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::all);
    wrapper.excludes<tag::field_3, tag::field_1>();
    wrapper.excludes<tag::field_3, tag::field_1>(); // excluding twice is fine
    Mask expected;
    expected.excludes_ref().emplace()[1].includes_ref().emplace()[1] =
        allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
  {
    // excludes{1: includes{1: excludes{}}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::all);
    Mask nestedMask;
    nestedMask.includes_ref().emplace()[1] = allMask();
    wrapper.excludes<tag::field_3>(nestedMask);
    Mask expected;
    expected.excludes_ref().emplace()[1].includes_ref().emplace()[1] =
        allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
  {
    // excludes{1: includes{1: excludes{},
    //                      2: excludes{}},
    //          2: excludes{}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::all);
    wrapper.excludes<tag::field_4>();
    wrapper.excludes<tag::field_3, tag::field_1>();
    wrapper.excludes<tag::field_3, tag::field_2>();
    Mask expected;
    auto& excludes = expected.excludes_ref().emplace();
    excludes[2] = allMask();
    auto& includes = excludes[1].includes_ref().emplace();
    includes[1] = allMask();
    includes[2] = allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
  {
    // excludes{1: excludes{}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::all);
    wrapper.excludes<tag::field_3, tag::field_1>();
    wrapper.excludes<tag::field_3>();
    Mask expected;
    expected.excludes_ref().emplace()[1] = allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }

  // Test includes and excludes
  {
    // includes{1: excludes{1: excludes{}}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::none);
    wrapper.includes<tag::field_3>();
    wrapper.excludes<tag::field_3, tag::field_1>();
    Mask expected;
    expected.includes_ref().emplace()[1].excludes_ref().emplace()[1] =
        allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
  {
    // excludes{1: excludes{1: excludes{}}}
    MaskWrapper<Bar2> wrapper(MaskWrapperInit::all);
    wrapper.excludes<tag::field_3>();
    wrapper.includes<tag::field_3, tag::field_1>();
    Mask expected;
    expected.excludes_ref().emplace()[1].excludes_ref().emplace()[1] =
        allMask();
    EXPECT_EQ(wrapper.toThrift(), expected);
  }
}

TEST(FieldMaskTest, ReverseMask) {
  EXPECT_EQ(reverseMask(allMask()), noneMask());
  EXPECT_EQ(reverseMask(noneMask()), allMask());
  // inclusiveMask and exclusiveMask are reverse of each other.
  Mask inclusiveMask;
  auto& includes = inclusiveMask.includes_ref().emplace();
  includes[1] = allMask();
  includes[2].includes_ref().emplace()[3] = allMask();
  Mask exclusiveMask;
  auto& excludes = exclusiveMask.excludes_ref().emplace();
  excludes[1] = allMask();
  excludes[2].includes_ref().emplace()[3] = allMask();

  EXPECT_EQ(reverseMask(inclusiveMask), exclusiveMask);
  EXPECT_EQ(reverseMask(exclusiveMask), inclusiveMask);
}
} // namespace apache::thrift::test
