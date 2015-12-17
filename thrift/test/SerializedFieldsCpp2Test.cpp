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


#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>

#include <thrift/test/gen-cpp2/SerializedFieldsTest_types.h>
#include <thrift/test/gen-cpp2/SerializedFieldsTest_constants.h>

#include <gtest/gtest.h>

using namespace thrift::test::serialized_fields::cpp2;
using namespace apache::thrift;
using namespace std;

const MetaStruct& kTestMetaStruct =
    SerializedFieldsTest_constants::kTestMetaStruct();
const ProxyUnknownStruct& kTestProxyUnknownStruct =
    SerializedFieldsTest_constants::kTestProxyUnknownStruct();

template<typename T>
string easySerialize(const T& a) {
  BinaryProtocolWriter prot;

  size_t bufSize = a.serializedSize(&prot);
  folly::IOBufQueue queue;
  prot.setOutput(&queue, bufSize);
  a.write(&prot);

  auto buf = queue.move();
  buf->coalesce();

  return string(buf->data(), buf->tail());
}

template<typename T>
T easyDeserialize(const string& s) {
  auto buf = IOBuf::copyBuffer(s);
  BinaryProtocolReader prot;
  prot.setInput(buf.get());
  T u;
  u.read(&prot);
  return u;
}

TEST(SerializedFields, MetaAdUnitSanityCheck) {
  string metaSerialized = easySerialize(kTestMetaStruct);
  auto metaDeserialized = easyDeserialize<MetaStruct>(metaSerialized);
  EXPECT_EQ(kTestMetaStruct, metaDeserialized);
}

TEST(SerializedFields, DifferentCppTypes) {
  EXPECT_TRUE((std::is_same<decltype(MetaStruct::i64_list),
                            decltype(DifferentImplStruct::i64_list)>::value));
  EXPECT_FALSE((std::is_same<decltype(MetaStruct::mapping),
                             decltype(DifferentImplStruct::mapping)>::value));
  EXPECT_FALSE((std::is_same<decltype(MetaStruct::setting),
                             decltype(DifferentImplStruct::setting)>::value));
  EXPECT_FALSE((std::is_same<decltype(MetaStruct::substruct),
                             decltype(DifferentImplStruct::substruct)>::value));
  EXPECT_FALSE((std::is_same<decltype(MetaStruct::substructs),
                            decltype(DifferentImplStruct::substructs)>::value));

  string metaSerialized = easySerialize(kTestMetaStruct);

  auto typesDeserialized =
      easyDeserialize<DifferentImplStruct>(metaSerialized);

  EXPECT_EQ(kTestMetaStruct.i64_list, typesDeserialized.i64_list);
  EXPECT_EQ(kTestMetaStruct.setting,
            set<string>(typesDeserialized.setting.begin(),
                        typesDeserialized.setting.end()));
  EXPECT_EQ(kTestMetaStruct.mapping,
            (map<string, string>(typesDeserialized.mapping.begin(),
                                 typesDeserialized.mapping.end())));
  EXPECT_EQ(kTestMetaStruct.substruct.id, typesDeserialized.substruct.id);
  EXPECT_EQ(kTestMetaStruct.substructs.size(),
            typesDeserialized.substructs.size());
  EXPECT_EQ(kTestMetaStruct.substructs[0].id,
            typesDeserialized.substructs[0].id);
  EXPECT_EQ(kTestMetaStruct.substructs[1].id,
            typesDeserialized.substructs[1].id);
  EXPECT_EQ(kTestMetaStruct.substructs[2].id,
            typesDeserialized.substructs[2].id);
}

TEST(SerializedFields, SomeFieldsSerialized) {
  string metaSerialized = easySerialize(kTestMetaStruct);

  auto strippedDeserialized =
      easyDeserialize<SomeFieldsSerialized>(metaSerialized);
  EXPECT_NE(nullptr, strippedDeserialized.__serialized.get());
  EXPECT_GT(strippedDeserialized.__serialized->computeChainDataLength(), 0);
  EXPECT_EQ(kTestMetaStruct.a_bite, strippedDeserialized.a_bite);
  EXPECT_EQ(kTestMetaStruct.i64_list, strippedDeserialized.i64_list);
  EXPECT_EQ(kTestMetaStruct.substruct, strippedDeserialized.substruct);

  auto strippedSerialized = easySerialize(strippedDeserialized);

  auto metaDeserialized = easyDeserialize<MetaStruct>(strippedSerialized);
  EXPECT_EQ(kTestMetaStruct.a_bite, metaDeserialized.a_bite);
  EXPECT_EQ(kTestMetaStruct.integer64, metaDeserialized.integer64);
  EXPECT_EQ(kTestMetaStruct.mapping, metaDeserialized.mapping);
  EXPECT_EQ(kTestMetaStruct.i64_list, metaDeserialized.i64_list);
  EXPECT_EQ(kTestMetaStruct.substruct, metaDeserialized.substruct);
  EXPECT_EQ(kTestMetaStruct.substructs, metaDeserialized.substructs);
  // non-serialized fields are lost
  EXPECT_EQ("", metaDeserialized.str);
  EXPECT_EQ(0, metaDeserialized.setting.size());
}

TEST(SerializedFields, ProxyUnknownStruct) {
  string metaSerialized = easySerialize(kTestMetaStruct);

  auto proxyDeserialized = easyDeserialize<ProxyUnknownStruct>(metaSerialized);
  EXPECT_NE(nullptr, proxyDeserialized.__serialized.get());
  EXPECT_GT(proxyDeserialized.__serialized->computeChainDataLength(), 0);
  EXPECT_EQ(kTestMetaStruct.a_bite, proxyDeserialized.a_bite);
  EXPECT_EQ(kTestMetaStruct.str, proxyDeserialized.str);

  auto proxySerialized = easySerialize(proxyDeserialized);

  auto metaDeserialized = easyDeserialize<MetaStruct>(proxySerialized);
  EXPECT_EQ(kTestMetaStruct, metaDeserialized);
}

TEST(SerializedFields, ChangeProtocolFine) {
  string smallSerialized = easySerialize(kTestProxyUnknownStruct);

  auto proxyDeserialized = easyDeserialize<ProxyUnknownStruct>(smallSerialized);
  EXPECT_EQ(nullptr, proxyDeserialized.__serialized.get());
  EXPECT_EQ(kTestProxyUnknownStruct.a_bite, proxyDeserialized.a_bite);
  EXPECT_EQ(kTestProxyUnknownStruct.str, proxyDeserialized.str);
  // Only one protocol is implemented for now, so let's fake different one
  proxyDeserialized.__serialized_protocol = ProtocolType::T_COMPACT_PROTOCOL;
  EXPECT_NO_THROW({ easySerialize(proxyDeserialized); });
}

TEST(SerializedFields, ChangeProtocolThrow) {
  string metaSerialized = easySerialize(kTestMetaStruct);

  auto proxyDeserialized = easyDeserialize<ProxyUnknownStruct>(metaSerialized);
  EXPECT_NE(nullptr, proxyDeserialized.__serialized.get());
  EXPECT_EQ(kTestMetaStruct.a_bite, proxyDeserialized.a_bite);
  EXPECT_EQ(kTestMetaStruct.str, proxyDeserialized.str);
  // Only one protocol is implemented for now, so let's fake different one
  proxyDeserialized.__serialized_protocol = ProtocolType::T_COMPACT_PROTOCOL;
  EXPECT_THROW({ easySerialize(proxyDeserialized); }, TProtocolException);
}
