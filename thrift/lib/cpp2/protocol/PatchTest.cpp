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

#include <thrift/lib/cpp2/protocol/Patch.h>

#include <stdexcept>

#include <folly/io/IOBuf.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp/util/VarintUtils.h>
#include <thrift/lib/cpp2/op/Patch.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Object.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/type/Id.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/thrift/gen-cpp2/patch_types.h>
#include <thrift/test/testset/Testset.h>
#include <thrift/test/testset/gen-cpp2/testset_types_custom_protocol.h>

namespace apache::thrift::protocol {
namespace {

class PatchTest : public testing::Test {
 protected:
  static Value asVal(bool val) { return asValueStruct<type::bool_t>(val); }

  template <typename P>
  static Value apply(const P& patchStruct, Value val) {
    // Serialize to compact.
    std::string buffer;
    CompactSerializer::serialize(patchStruct.toThrift(), &buffer);
    auto binaryObj = folly::IOBuf::wrapBuffer(buffer.data(), buffer.size());
    // Parse to Object.
    Object patchObj = parseObject<CompactProtocolReader>(*binaryObj);
    // Apply to val.
    applyPatch(patchObj, val);
    return val;
  }

  template <typename P, typename V>
  static Value apply(const P& patchStruct, V&& val) {
    return apply(patchStruct, asVal(val));
  }

  template <typename PatchType, typename F>
  void testNumericPatchObject(Value value, F unpacker) {
    // Noop
    EXPECT_EQ(42, unpacker(apply(PatchType{}, value)));

    // Assign
    EXPECT_EQ(43, unpacker(apply(PatchType{} = 43, value)));

    // Add
    EXPECT_EQ(43, unpacker(apply(PatchType{} + 1, value)));

    // Subtract
    EXPECT_EQ(41, unpacker(apply(PatchType{} - 1, value)));

    // Wrong patch provided
    EXPECT_THROW(apply(op::BoolPatch{}, value), std::runtime_error);

    // Wrong object to patch
    EXPECT_THROW(apply(PatchType{}, true), std::runtime_error);
  }

  static Object patchAddOperation(Object&& patch, auto operation, auto value) {
    auto opId = static_cast<int16_t>(operation);
    patch.members().ensure()[opId] = value;
    return std::move(patch);
  }

  static Object makePatch(auto operation, auto value) {
    return patchAddOperation(Object{}, operation, value);
  }

  static Value applyContainerPatch(Object patch, Value value) {
    auto buffer = serializeObject<CompactProtocolWriter>(patch);
    Object patchObj = parseObject<CompactProtocolReader>(*buffer);

    applyPatch(patchObj, value);
    return value;
  }
};

TEST_F(PatchTest, Bool) {
  // Noop
  EXPECT_TRUE(*apply(op::BoolPatch{}, true).boolValue_ref());
  EXPECT_FALSE(*apply(op::BoolPatch{}, false).boolValue_ref());

  // Assign
  EXPECT_TRUE(*apply(op::BoolPatch{} = true, true).boolValue_ref());
  EXPECT_TRUE(*apply(op::BoolPatch{} = true, false).boolValue_ref());
  EXPECT_FALSE(*apply(op::BoolPatch{} = false, true).boolValue_ref());
  EXPECT_FALSE(*apply(op::BoolPatch{} = false, false).boolValue_ref());

  // Invert
  EXPECT_TRUE(*apply(!op::BoolPatch{}, false).boolValue_ref());
  EXPECT_FALSE(*apply(!op::BoolPatch{}, true).boolValue_ref());

  // Wrong patch provided
  EXPECT_THROW(apply(op::I16Patch{} += 1, true), std::runtime_error);

  // Wrong object to patch
  EXPECT_THROW(
      apply(op::BoolPatch{}, asValueStruct<type::i16_t>(42)),
      std::runtime_error);

  // Should we check non-patch objects passed as patch? Previous checks kind of
  // cover this.
}

TEST_F(PatchTest, Byte) {
  testNumericPatchObject<op::BytePatch>(
      asValueStruct<type::byte_t>(42),
      [](auto val) { return *val.byteValue_ref(); });
}

TEST_F(PatchTest, I16) {
  testNumericPatchObject<op::I16Patch>(
      asValueStruct<type::i16_t>(42),
      [](auto val) { return *val.i16Value_ref(); });
}

TEST_F(PatchTest, I32) {
  testNumericPatchObject<op::I32Patch>(
      asValueStruct<type::i32_t>(42),
      [](auto val) { return *val.i32Value_ref(); });
}

TEST_F(PatchTest, I64) {
  testNumericPatchObject<op::I64Patch>(
      asValueStruct<type::i64_t>(42),
      [](auto val) { return *val.i64Value_ref(); });
}

TEST_F(PatchTest, Float) {
  testNumericPatchObject<op::FloatPatch>(
      asValueStruct<type::float_t>(42),
      [](auto val) { return *val.floatValue_ref(); });
}

TEST_F(PatchTest, Double) {
  testNumericPatchObject<op::DoublePatch>(
      asValueStruct<type::double_t>(42),
      [](auto val) { return *val.doubleValue_ref(); });
}

TEST_F(PatchTest, Binary) {
  std::string data = "test", patch = "best";
  auto toPatch = folly::IOBuf::wrapBufferAsValue(data.data(), data.size());
  auto patchValue = folly::IOBuf::wrapBufferAsValue(patch.data(), patch.size());
  auto binaryData = asValueStruct<type::binary_t>(toPatch);
  // Noop
  EXPECT_TRUE(folly::IOBufEqualTo{}(
      toPatch, *apply(op::BinaryPatch{}, binaryData).binaryValue_ref()));

  // Assign
  EXPECT_TRUE(apply(op::BinaryPatch{} = folly::IOBuf(), binaryData)
                  .binaryValue_ref()
                  ->empty());
  EXPECT_TRUE(folly::IOBufEqualTo{}(
      patchValue,
      *apply(op::BinaryPatch{} = patchValue, binaryData).binaryValue_ref()));

  // Wrong patch provided
  EXPECT_THROW(apply(op::I16Patch{}, binaryData), std::runtime_error);

  // Wrong object to patch
  EXPECT_THROW(
      apply(op::BinaryPatch{} = patchValue, asValueStruct<type::i16_t>(42)),
      std::runtime_error);
}

TEST_F(PatchTest, String) {
  std::string data = "test", patch = "best";
  auto stringData = asValueStruct<type::string_t>(data);
  // Noop
  EXPECT_EQ(data, *apply(op::StringPatch{}, stringData).stringValue_ref());

  // Assign
  EXPECT_EQ(
      patch, *apply(op::StringPatch{} = patch, stringData).stringValue_ref());

  // Clear
  {
    op::StringPatch strPatch;
    strPatch.clear();
    EXPECT_TRUE(apply(strPatch, stringData).stringValue_ref()->empty());
  }

  // Append
  {
    op::StringPatch strPatch;
    strPatch.append(patch);
    EXPECT_EQ(data + patch, *apply(strPatch, stringData).stringValue_ref());
  }

  // Prepend
  {
    op::StringPatch strPatch;
    strPatch.prepend(patch);
    EXPECT_EQ(patch + data, *apply(strPatch, stringData).stringValue_ref());
  }

  // Clear, Append and Prepend in one
  {
    op::StringPatch strPatch;
    strPatch.clear();
    strPatch.append(patch);
    strPatch.prepend(patch);
    EXPECT_EQ(patch + patch, *apply(strPatch, stringData).stringValue_ref());
  }

  // Wrong patch provided
  EXPECT_THROW(apply(op::I16Patch{}, stringData), std::runtime_error);

  // Wrong object to patch
  EXPECT_THROW(
      apply(op::StringPatch{} = patch, asValueStruct<type::i16_t>(42)),
      std::runtime_error);
}

TEST_F(PatchTest, List) {
  std::vector<std::string> data = {"test"}, patch = {"new value"};
  auto value = asValueStruct<type::list<type::binary_t>>(data);
  auto patchValue = asValueStruct<type::list<type::binary_t>>(patch);

  // Noop
  {
    Object patchObj;
    EXPECT_EQ(
        *value.listValue_ref(),
        *applyContainerPatch(patchObj, value).listValue_ref());
  }

  // Assign
  {
    Object patchObj = makePatch(op::PatchOp::Assign, patchValue);
    EXPECT_EQ(
        *patchValue.listValue_ref(),
        *applyContainerPatch(patchObj, value).listValue_ref());
  }

  // Clear
  {
    Object patchObj =
        makePatch(op::PatchOp::Clear, asValueStruct<type::bool_t>(true));
    EXPECT_EQ(
        std::vector<Value>{},
        *applyContainerPatch(patchObj, value).listValue_ref());
  }

  // Patch
  {
    auto elementPatchValue = asValueStruct<type::binary_t>(std::string{"best"});
    Value fieldPatchValue;
    fieldPatchValue.objectValue_ref() =
        makePatch(op::PatchOp::Put, elementPatchValue);
    Value listElementPatch;
    listElementPatch.mapValue_ref()
        .ensure()[asValueStruct<type::i32_t>(apache::thrift::util::i32ToZigzag(
            static_cast<int32_t>(type::toOrdinal(0))))] = fieldPatchValue;
    auto patchObj = makePatch(op::PatchOp::Patch, listElementPatch);
    auto patched = *applyContainerPatch(patchObj, value).listValue_ref();
    EXPECT_EQ(
        std::vector<Value>{asValueStruct<type::binary_t>("testbest")}, patched);
  }

  // Prepend
  {
    auto expected = *patchValue.listValue_ref();
    expected.insert(
        expected.end(),
        value.listValue_ref()->begin(),
        value.listValue_ref()->end());
    Object patchObj = makePatch(op::PatchOp::Prepend, patchValue);
    EXPECT_EQ(expected, *applyContainerPatch(patchObj, value).listValue_ref());
  }

  // Append
  {
    auto expected = *value.listValue_ref();
    expected.insert(
        expected.end(),
        patchValue.listValue_ref()->begin(),
        patchValue.listValue_ref()->end());
    Object patchObj = makePatch(op::PatchOp::Put, patchValue);
    EXPECT_EQ(expected, *applyContainerPatch(patchObj, value).listValue_ref());
  }

  // Remove
  {
    Object patchObj = makePatch(
        op::PatchOp::Remove,
        asValueStruct<type::set<type::binary_t>>(std::set{"test"}));
    EXPECT_EQ(
        std::vector<Value>{},
        *applyContainerPatch(patchObj, value).listValue_ref());
  }

  // Add
  {
    Object patchObj = makePatch(
        op::PatchOp::Add,
        asValueStruct<type::set<type::binary_t>>(std::set{"test"}));
    EXPECT_EQ(
        *value.listValue_ref(),
        *applyContainerPatch(patchObj, value).listValue_ref())
        << "Shuold insert nothing";
  }
  {
    auto expected = *value.listValue_ref();
    expected.push_back(asValueStruct<type::binary_t>("best"));
    Object patchObj = makePatch(
        op::PatchOp::Add,
        asValueStruct<type::set<type::binary_t>>(std::set{"best"}));
    EXPECT_EQ(expected, *applyContainerPatch(patchObj, value).listValue_ref());
  }

  // Combination
  {
    auto patchObj = patchAddOperation(
        patchAddOperation(
            patchAddOperation(
                makePatch(
                    op::PatchOp::Add,
                    asValueStruct<type::set<type::binary_t>>(std::set{"best"})),
                op::PatchOp::Remove,
                asValueStruct<type::set<type::binary_t>>(std::set{"test"})),
            op::PatchOp::Put,
            value),
        op::PatchOp::Prepend,
        asValueStruct<type::list<type::binary_t>>(std::vector{"the"}));

    auto expected = asValueStruct<type::list<type::binary_t>>(
        std::vector{"the", "best", "test"});
    EXPECT_EQ(
        *expected.listValue_ref(),
        *applyContainerPatch(patchObj, value).listValue_ref());
  }
}

TEST_F(PatchTest, Set) {
  std::set<std::string> data = {"test"}, patch = {"new value"};
  auto value = asValueStruct<type::set<type::binary_t>>(data);
  auto patchValue = asValueStruct<type::set<type::binary_t>>(patch);

  // Noop
  {
    Object patchObj;
    EXPECT_EQ(
        *value.setValue_ref(),
        *applyContainerPatch(patchObj, value).setValue_ref());
  }

  // Assign
  {
    Object patchObj = makePatch(op::PatchOp::Assign, patchValue);
    EXPECT_EQ(
        *patchValue.setValue_ref(),
        *applyContainerPatch(patchObj, value).setValue_ref());
  }

  // Clear
  {
    Object patchObj =
        makePatch(op::PatchOp::Clear, asValueStruct<type::bool_t>(true));
    EXPECT_EQ(
        std::set<Value>{},
        *applyContainerPatch(patchObj, value).setValue_ref());
  }

  // Put
  {
    auto expected = *value.setValue_ref();
    expected.insert(
        patchValue.setValue_ref()->begin(), patchValue.setValue_ref()->end());
    Object patchObj = makePatch(op::PatchOp::Put, patchValue);
    EXPECT_EQ(expected, *applyContainerPatch(patchObj, value).setValue_ref());
  }

  // Remove
  {
    Object patchObj = makePatch(
        op::PatchOp::Remove,
        asValueStruct<type::set<type::binary_t>>(std::set{"test"}));
    EXPECT_EQ(
        std::set<Value>{},
        *applyContainerPatch(patchObj, value).setValue_ref());
  }

  // Add
  {
    Object patchObj = makePatch(
        op::PatchOp::Add,
        asValueStruct<type::set<type::binary_t>>(std::set{"test"}));
    EXPECT_EQ(
        *value.setValue_ref(),
        *applyContainerPatch(patchObj, value).setValue_ref())
        << "Shuold insert nothing";
  }
  {
    auto expected = *value.setValue_ref();
    expected.insert(asValueStruct<type::binary_t>(std::string{"best test"}));
    Object patchObj = makePatch(
        op::PatchOp::Add,
        asValueStruct<type::set<type::binary_t>>(std::set{"best test"}));
    auto patchResult = *applyContainerPatch(patchObj, value).setValue_ref();
    EXPECT_EQ(expected, patchResult);
  }

  // Combination
  {
    auto patchObj = patchAddOperation(
        patchAddOperation(
            makePatch(
                op::PatchOp::Add,
                asValueStruct<type::set<type::binary_t>>(std::set{"best"})),
            op::PatchOp::Remove,
            asValueStruct<type::set<type::binary_t>>(std::set{"test"})),
        op::PatchOp::Put,
        asValueStruct<type::set<type::binary_t>>(std::set{"rest"}));

    auto expected =
        asValueStruct<type::set<type::binary_t>>(std::set{"best", "rest"});
    EXPECT_EQ(
        *expected.setValue_ref(),
        *applyContainerPatch(patchObj, value).setValue_ref());
  }
}

TEST_F(PatchTest, Map) {
  std::map<std::string, std::string> data = {{"key", "test"}},
                                     patch = {{"new key", "new value"}};
  auto value = asValueStruct<type::map<type::binary_t, type::binary_t>>(data);
  auto patchValue =
      asValueStruct<type::map<type::binary_t, type::binary_t>>(patch);
  auto emptyMap = std::map<Value, Value>{};

  // Noop
  {
    Object patchObj;
    EXPECT_EQ(
        *value.mapValue_ref(),
        *applyContainerPatch(patchObj, value).mapValue_ref());
  }

  // Assign
  {
    Object patchObj = makePatch(op::PatchOp::Assign, patchValue);
    EXPECT_EQ(
        *patchValue.mapValue_ref(),
        *applyContainerPatch(patchObj, value).mapValue_ref());
  }

  // Clear
  {
    Object patchObj =
        makePatch(op::PatchOp::Clear, asValueStruct<type::bool_t>(true));
    EXPECT_EQ(emptyMap, *applyContainerPatch(patchObj, value).mapValue_ref());
  }

  // Remove
  {
    Object patchObj = makePatch(
        op::PatchOp::Remove,
        asValueStruct<type::set<type::binary_t>>(std::set{"key"}));
    EXPECT_EQ(emptyMap, *applyContainerPatch(patchObj, value).mapValue_ref());
  }

  // Ensure
  {
    Object patchObj = makePatch(
        op::PatchOp::EnsureStruct,
        asValueStruct<type::map<type::binary_t, type::binary_t>>(
            std::map<std::string, std::string>{{"key", "test 42"}}));
    EXPECT_EQ(
        *value.mapValue_ref(),
        *applyContainerPatch(patchObj, value).mapValue_ref())
        << "Shuold insert nothing";
  }
  {
    auto expected = *value.mapValue_ref();
    expected.emplace(
        asValueStruct<type::binary_t>(std::string{"new key"}),
        asValueStruct<type::binary_t>(std::string{"new value"}));
    Object patchObj = makePatch(
        op::PatchOp::EnsureStruct,
        asValueStruct<type::map<type::binary_t, type::binary_t>>(
            std::map<std::string, std::string>{{"new key", "new value"}}));
    auto patchResult = *applyContainerPatch(patchObj, value).mapValue_ref();
    EXPECT_EQ(expected, patchResult);
  }

  // Put
  {
    auto expected = *value.mapValue_ref();
    expected[asValueStruct<type::binary_t>(std::string{"key"})] =
        asValueStruct<type::binary_t>(std::string{"key updated value"});
    Object patchObj = makePatch(
        op::PatchOp::Put,
        asValueStruct<type::map<type::binary_t, type::binary_t>>(
            std::map<std::string, std::string>{{"key", "key updated value"}}));
    EXPECT_EQ(expected, *applyContainerPatch(patchObj, value).mapValue_ref());
  }

  // Combination
  {
    auto expected = asValueStruct<type::map<type::binary_t, type::binary_t>>(
        std::map<std::string, std::string>{
            {"new key", "new value"}, {"added key", "overridden value"}});
    auto patchObj = patchAddOperation(
        patchAddOperation(
            makePatch(
                op::PatchOp::EnsureStruct,
                asValueStruct<type::map<type::binary_t, type::binary_t>>(
                    std::map<std::string, std::string>{
                        {"added key", "added value"}})),
            op::PatchOp::Remove,
            asValueStruct<type::set<type::binary_t>>(std::set{"key"})),
        op::PatchOp::Put,
        expected);

    EXPECT_EQ(
        *expected.mapValue_ref(),
        *applyContainerPatch(patchObj, value).mapValue_ref());
  }

  // Patch
  {
    auto value =
        asValueStruct<type::map<type::binary_t, type::list<type::binary_t>>>(
            std::map<std::string, std::vector<std::string>>{
                {"key", std::vector<std::string>{"test"}}});
    auto elementPatchValue = asValueStruct<type::list<type::binary_t>>(
        std::vector<std::string>{"foo"});
    Value fieldPatchValue;
    fieldPatchValue.objectValue_ref() =
        makePatch(op::PatchOp::Put, elementPatchValue);
    Value mapPatch;
    mapPatch.mapValue_ref().ensure()[asValueStruct<type::binary_t>("key")] =
        fieldPatchValue;
    auto patchObj = makePatch(op::PatchOp::Patch, mapPatch);
    auto expected =
        asValueStruct<type::map<type::binary_t, type::list<type::binary_t>>>(
            std::map<std::string, std::vector<std::string>>{
                {"key", std::vector<std::string>{"test", "foo"}}});
    EXPECT_EQ(
        *expected.mapValue_ref(),
        *applyContainerPatch(patchObj, value).mapValue_ref());
  }

  // Ensure and PatchAfter
  {
    auto value =
        asValueStruct<type::map<type::binary_t, type::list<type::binary_t>>>(
            std::map<std::string, std::vector<std::string>>{
                {"key", std::vector<std::string>{"test"}}});
    Value fieldPatchValue;
    fieldPatchValue.objectValue_ref() = makePatch(
        op::PatchOp::Put,
        asValueStruct<type::list<type::binary_t>>(
            std::vector<std::string>{"foo"}));
    Value mapPatch;
    mapPatch.mapValue_ref().ensure()[asValueStruct<type::binary_t>("new key")] =
        fieldPatchValue;

    auto patchObj = patchAddOperation(
        makePatch(op::PatchOp::PatchAfter, mapPatch),
        op::PatchOp::EnsureStruct,
        asValueStruct<type::map<type::binary_t, type::list<type::binary_t>>>(
            std::map<std::string, std::vector<std::string>>{
                {"new key", std::vector<std::string>{}}}));

    auto expected =
        asValueStruct<type::map<type::binary_t, type::list<type::binary_t>>>(
            std::map<std::string, std::vector<std::string>>{
                {"key", std::vector<std::string>{"test"}},
                {"new key", std::vector<std::string>{"foo"}}});
    EXPECT_EQ(
        *expected.mapValue_ref(),
        *applyContainerPatch(patchObj, value).mapValue_ref());
  }
}

TEST_F(PatchTest, Struct) {
  test::testset::struct_with<type::list<type::i32_t>> valueObject;
  test::testset::struct_with<type::list<type::i32_t>> patchObject;

  valueObject.field_1_ref() = std::vector<int>{1, 2, 3};
  patchObject.field_1_ref() = std::vector<int>{3, 2, 1};

  auto value = asValueStruct<type::struct_c>(valueObject);
  auto patchValue = asValueStruct<type::struct_c>(patchObject);

  // Noop
  {
    Object patchObj;
    EXPECT_EQ(
        *value.objectValue_ref(),
        *applyContainerPatch(patchObj, value).objectValue_ref());
  }

  // Assign
  {
    Object patchObj = makePatch(op::PatchOp::Assign, patchValue);
    EXPECT_EQ(
        *patchValue.objectValue_ref(),
        *applyContainerPatch(patchObj, value).objectValue_ref());
  }

  // Clear
  {
    Object patchObj =
        makePatch(op::PatchOp::Clear, asValueStruct<type::bool_t>(true));
    EXPECT_TRUE(applyContainerPatch(patchObj, value)
                    .objectValue_ref()
                    ->members()
                    ->empty());
  }

  // Patch
  auto applyFieldPatchTest = [&](auto op, auto expected) {
    Value fieldPatchValue;
    fieldPatchValue.objectValue_ref() = makePatch(
        op, asValueStruct<type::list<type::i32_t>>(std::vector<int>{3, 2, 1}));
    Value fieldPatch;
    fieldPatch.objectValue_ref().ensure().members().ensure()[1] =
        fieldPatchValue;
    Object patchObj = makePatch(op::PatchOp::Patch, fieldPatch);
    EXPECT_EQ(
        expected,
        applyContainerPatch(patchObj, value)
            .objectValue_ref()
            ->members()
            .ensure()[1]);
  };

  applyFieldPatchTest(
      op::PatchOp::Assign, patchValue.objectValue_ref()->members().ensure()[1]);

  applyFieldPatchTest(
      op::PatchOp::Put,
      asValueStruct<type::list<type::i32_t>>(
          std::vector<int>{1, 2, 3, 3, 2, 1}));

  // Ensure and PatchAfter
  {
    test::testset::struct_with<type::list<type::i32_t>> source;
    auto sourceValue = asValueStruct<type::struct_c>(source);

    Value ensureValuePatch;
    Object ensureObject;
    ensureObject.members().ensure()[1] =
        asValueStruct<type::list<type::i32_t>>(std::vector<int32_t>{});
    ensureValuePatch.objectValue_ref() = ensureObject;

    Value fieldPatchValue;
    fieldPatchValue.objectValue_ref() = makePatch(
        op::PatchOp::Put,
        asValueStruct<type::list<type::i32_t>>(std::vector<int32_t>{42}));
    Value fieldPatch;
    fieldPatch.objectValue_ref().ensure().members().ensure()[1] =
        fieldPatchValue;

    Object patchObj = patchAddOperation(
        makePatch(op::PatchOp::PatchAfter, fieldPatch),
        op::PatchOp::EnsureStruct,
        ensureValuePatch);

    EXPECT_EQ(
        asValueStruct<type::list<type::i32_t>>(std::vector<int32_t>{42}),
        applyContainerPatch(patchObj, sourceValue)
            .objectValue_ref()
            ->members()
            .ensure()[1]);
  }
}
} // namespace
} // namespace apache::thrift::protocol
