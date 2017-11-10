/*
 * Copyright 2017-present Facebook, Inc.
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
/*
 * TODO(@denplusplus, by 11/04/2017) Remove.
 */
#include <string>

#include <gtest/gtest.h>

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include <thrift/test/gen-cpp2/NewServer.h>
#include <thrift/test/gen-cpp2/OldServer.h>
#include <thrift/test/gen-cpp2/forward_compatibility_types.h>

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using namespace folly;
using namespace forward_compatibility;
using namespace std;

namespace {

template <typename T, typename Writer>
string easySerialize(const T& a) {
  Writer prot;

  size_t bufSize = a.serializedSize(&prot);
  folly::IOBufQueue queue;
  prot.setOutput(&queue, bufSize);
  a.write(&prot);

  auto buf = queue.move();
  buf->coalesce();

  return string(buf->data(), buf->tail());
}

template <typename T, typename Reader>
T easyDeserialize(const string& s) {
  auto buf = IOBuf::copyBuffer(s);
  Reader prot;
  prot.setInput(buf.get());
  T u;
  u.read(&prot);
  return u;
}

template <typename Reader, typename Writer>
void testForwardCompatibility() {
  OldStructure s;
  s.features[1] = 1.0;
  s.features[100] = 3.14;
  static_assert(
      std::is_same<typename decltype(s.features)::key_type, int16_t>::value,
      "key must be i16");
  static_assert(
      std::is_same<typename decltype(s.features)::mapped_type, double>::value,
      "value must be double");
  auto n = easyDeserialize<NewStructure, Reader>(
      easySerialize<OldStructure, Writer>(s));
  EXPECT_EQ(n.features[1], 1.0);
  EXPECT_EQ(n.features[100], 3.14f);
  static_assert(
      std::is_same<typename decltype(n.features)::key_type, int32_t>::value,
      "key must be i32");
  static_assert(
      std::is_same<typename decltype(n.features)::mapped_type, float>::value,
      "value must be float");
  auto n2 = easyDeserialize<NewStructure, Reader>(
      easySerialize<NewStructure, Writer>(n));
  EXPECT_EQ(n2.features[1], 1.0);
  EXPECT_EQ(n2.features[100], 3.14f);
}

template <typename Reader, typename Writer>
void testForwardCompatibilityNested() {
  OldStructureNested s;
  s.featuresList.emplace_back();
  s.featuresList[0][1] = 1.0;
  s.featuresList[0][100] = 3.14;
  auto n = easyDeserialize<NewStructureNested, Reader>(
      easySerialize<OldStructureNested, Writer>(s));
  EXPECT_EQ(n.featuresList[0][1], 1.0);
  EXPECT_EQ(n.featuresList[0][100], 3.14f);
}

template <typename Reader, typename Writer>
void testForwardCompatibilityComplexMap() {
  OldMapMapStruct s;
  s.features[1][2] = 3.14;
  s.features[2][1] = 2.71;
  auto n = easyDeserialize<NewMapMapStruct, Reader>(
      easySerialize<OldMapMapStruct, Writer>(s));
  EXPECT_EQ(n.features[1][2], 3.14);
  EXPECT_EQ(n.features[2][1], 2.71);
}

template <typename Reader, typename Writer>
void testForwardCompatibilityComplexList() {
  OldMapListStruct s;
  s.features[1].push_back(3.14);
  s.features[1].push_back(2.71);
  s.features[2];
  s.features[3].push_back(12345.56);
  auto n = easyDeserialize<NewMapListStruct, Reader>(
      easySerialize<OldMapListStruct, Writer>(s));
  EXPECT_NEAR(n.features.at(1).at(0), 3.14, 1e-3);
  EXPECT_NEAR(n.features.at(1).at(1), 2.71, 1e-3);
  EXPECT_EQ(n.features.at(2).size(), 0);
  EXPECT_NEAR(n.features.at(3).at(0), 12345.56, 1e-3);
  EXPECT_EQ(n.features.size(), 3);
}

class OldServerHandler : public OldServerSvIf {
 public:
  void get(OldStructure& s) override {
    s.features[1] = 1.0;
    s.features[100] = 3.14;
  }
};

std::unique_ptr<ScopedServerInterfaceThread> createThriftServer() {
  auto server = std::make_shared<ThriftServer>();
  server->setPort(0);
  server->setInterface(std::make_unique<OldServerHandler>());
  return std::make_unique<ScopedServerInterfaceThread>(server);
}

} // namespace

TEST(ForwardCompatibility, Simple) {
  testForwardCompatibility<BinaryProtocolReader, BinaryProtocolWriter>();
  testForwardCompatibility<CompactProtocolReader, CompactProtocolWriter>();
  testForwardCompatibility<
      SimpleJSONProtocolReader,
      SimpleJSONProtocolWriter>();
}

TEST(ForwardCompatibility, Nested) {
  testForwardCompatibilityNested<BinaryProtocolReader, BinaryProtocolWriter>();
  testForwardCompatibilityNested<
      CompactProtocolReader,
      CompactProtocolWriter>();
  testForwardCompatibilityNested<
      SimpleJSONProtocolReader,
      SimpleJSONProtocolWriter>();
}

TEST(ForwardCompatibility, MapMap) {
  testForwardCompatibilityComplexMap<
      BinaryProtocolReader,
      BinaryProtocolWriter>();
  testForwardCompatibilityComplexMap<
      CompactProtocolReader,
      CompactProtocolWriter>();
  testForwardCompatibilityComplexMap<
      SimpleJSONProtocolReader,
      SimpleJSONProtocolWriter>();
}

TEST(ForwardCompatibility, MapList) {
  testForwardCompatibilityComplexList<
      BinaryProtocolReader,
      BinaryProtocolWriter>();
  testForwardCompatibilityComplexList<
      CompactProtocolReader,
      CompactProtocolWriter>();
  testForwardCompatibilityComplexList<
      SimpleJSONProtocolReader,
      SimpleJSONProtocolWriter>();
}

TEST(ForwardCompatibility, Server) {
  EventBase eb;
  auto serverThread = createThriftServer();
  auto serverAddr = serverThread->getAddress();
  std::shared_ptr<TAsyncSocket> socket(
      TAsyncSocket::newSocket(&eb, serverAddr));
  auto channel = HeaderClientChannel::newChannel(socket);
  NewServerAsyncClient client(std::move(channel));
  NewStructure res;
  client.sync_get(res);
  EXPECT_EQ(res.features[1], 1.0);
  EXPECT_EQ(res.features[100], 3.14f);
}
