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
#include <thrift/lib/cpp2/op/Patch.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Object.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/thrift/gen-cpp2/patch_types.h>
#include <thrift/test/testset/Testset.h>

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
  auto value = asValueStruct<type::list<type::string_t>>(data);
  auto patchValue = asValueStruct<type::list<type::string_t>>(patch);

  auto patchAddOperation = [](auto&& patch, auto operation, auto value) {
    auto opId = static_cast<int16_t>(operation);
    patch.members().ensure()[opId] = value;
    return std::move(patch);
  };

  auto makePatch = [&](auto operation, auto value) {
    return patchAddOperation(Object{}, operation, value);
  };

  auto applyListPatch = [](auto patch, auto value) {
    applyPatch(patch, value);
    return value;
  };

  // Noop
  {
    Object patchObj;
    EXPECT_EQ(
        *value.listValue_ref(),
        *applyListPatch(patchObj, value).listValue_ref());
  }

  // Assign
  EXPECT_EQ(
      *patchValue.listValue_ref(),
      *applyListPatch(makePatch(op::PatchOp::Assign, patchValue), value)
           .listValue_ref());

  // Clear
  EXPECT_EQ(
      std::vector<Value>{},
      *applyListPatch(
           makePatch(op::PatchOp::Clear, asValueStruct<type::bool_t>(true)),
           value)
           .listValue_ref());

  // Prepent
  {
    auto expected = *patchValue.listValue_ref();
    expected.insert(
        expected.end(),
        value.listValue_ref()->begin(),
        value.listValue_ref()->end());
    EXPECT_EQ(
        expected,
        *applyListPatch(makePatch(op::PatchOp::Prepend, patchValue), value)
             .listValue_ref());
  }

  // Append
  {
    auto expected = *value.listValue_ref();
    expected.insert(
        expected.end(),
        patchValue.listValue_ref()->begin(),
        patchValue.listValue_ref()->end());
    EXPECT_EQ(
        expected,
        *applyListPatch(makePatch(op::PatchOp::Put, patchValue), value)
             .listValue_ref());
  }

  // Remove
  {
    EXPECT_EQ(
        std::vector<Value>{},
        *applyListPatch(
             makePatch(
                 op::PatchOp::Remove,
                 asValueStruct<type::set<type::string_t>>(std::set{"test"})),
             value)
             .listValue_ref());
  }

  // Add
  {
    EXPECT_EQ(
        *value.listValue_ref(),
        *applyListPatch(
             makePatch(
                 op::PatchOp::Add,
                 asValueStruct<type::set<type::string_t>>(std::set{"test"})),
             value)
             .listValue_ref())
        << "Shuold insert nothing";

    auto expected = *value.listValue_ref();
    expected.push_back(asValueStruct<type::string_t>("best"));
    EXPECT_EQ(
        expected,
        *applyListPatch(
             makePatch(
                 op::PatchOp::Add,
                 asValueStruct<type::set<type::string_t>>(std::set{"best"})),
             value)
             .listValue_ref());
  }

  // Combination
  {
    auto patchObj = patchAddOperation(
        patchAddOperation(
            patchAddOperation(
                makePatch(
                    op::PatchOp::Add,
                    asValueStruct<type::set<type::string_t>>(std::set{"best"})),
                op::PatchOp::Remove,
                asValueStruct<type::set<type::string_t>>(std::set{"test"})),
            op::PatchOp::Put,
            value),
        op::PatchOp::Prepend,
        asValueStruct<type::list<type::string_t>>(std::vector{"the"}));

    auto expected = asValueStruct<type::list<type::string_t>>(
        std::vector{"the", "best", "test"});
    EXPECT_EQ(
        *expected.listValue_ref(),
        *applyListPatch(patchObj, value).listValue_ref());
  }
}

TEST_F(PatchTest, Set) {
  std::set<std::string> data = {"test"}, patch = {"new value"};
  auto value = asValueStruct<type::set<type::string_t>>(data);
  auto patchValue = asValueStruct<type::set<type::string_t>>(patch);

  auto patchAddOperation = [](auto&& patch, auto operation, auto value) {
    auto opId = static_cast<int16_t>(operation);
    patch.members().ensure()[opId] = value;
    return std::move(patch);
  };

  auto makePatch = [&](auto operation, auto value) {
    return patchAddOperation(Object{}, operation, value);
  };

  auto applySetPatch = [](auto patch, auto value) {
    applyPatch(patch, value);
    return value;
  };

  // Noop
  {
    Object patchObj;
    EXPECT_EQ(
        *value.setValue_ref(), *applySetPatch(patchObj, value).setValue_ref());
  }

  // Assign
  EXPECT_EQ(
      *patchValue.setValue_ref(),
      *applySetPatch(makePatch(op::PatchOp::Assign, patchValue), value)
           .setValue_ref());

  // Clear
  EXPECT_EQ(
      std::set<Value>{},
      *applySetPatch(
           makePatch(op::PatchOp::Clear, asValueStruct<type::bool_t>(true)),
           value)
           .setValue_ref());

  // Append
  {
    auto expected = *value.setValue_ref();
    expected.insert(
        patchValue.setValue_ref()->begin(), patchValue.setValue_ref()->end());
    EXPECT_EQ(
        expected,
        *applySetPatch(makePatch(op::PatchOp::Put, patchValue), value)
             .setValue_ref());
  }

  // Remove
  {
    EXPECT_EQ(
        std::set<Value>{},
        *applySetPatch(
             makePatch(
                 op::PatchOp::Remove,
                 asValueStruct<type::set<type::string_t>>(std::set{"test"})),
             value)
             .setValue_ref());
  }

  // Add
  {
    EXPECT_EQ(
        *value.setValue_ref(),
        *applySetPatch(
             makePatch(
                 op::PatchOp::Add,
                 asValueStruct<type::set<type::string_t>>(std::set{"test"})),
             value)
             .setValue_ref())
        << "Shuold insert nothing";

    auto expected = *value.setValue_ref();
    expected.insert(asValueStruct<type::string_t>(std::string{"best test"}));
    auto patchResult = *applySetPatch(
                            makePatch(
                                op::PatchOp::Add,
                                asValueStruct<type::set<type::string_t>>(
                                    std::set{"best test"})),
                            value)
                            .setValue_ref();
    EXPECT_EQ(expected, patchResult);
  }

  // Combination
  {
    auto patchObj = patchAddOperation(
        patchAddOperation(
            makePatch(
                op::PatchOp::Add,
                asValueStruct<type::set<type::string_t>>(std::set{"best"})),
            op::PatchOp::Remove,
            asValueStruct<type::set<type::string_t>>(std::set{"test"})),
        op::PatchOp::Put,
        asValueStruct<type::set<type::string_t>>(std::set{"rest"}));

    auto expected =
        asValueStruct<type::set<type::string_t>>(std::set{"best", "rest"});
    EXPECT_EQ(
        *expected.setValue_ref(),
        *applySetPatch(patchObj, value).setValue_ref());
  }
}

} // namespace
} // namespace apache::thrift::protocol
