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

#include "thrift/lib/cpp2/protocol/CompactProtocol.h"
#include "thrift/lib/cpp2/protocol/BinaryProtocol.h"
#include "thrift/lib/cpp/protocol/TBinaryProtocol.h"
#include "thrift/lib/cpp/protocol/TCompactProtocol.h"
#include "thrift/lib/cpp/transport/TBufferTransports.h"

#include "thrift/test/gen-cpp2/DebugProtoTest_types.h"
#include "thrift/test/gen-cpp/DebugProtoTest_types.h"

#include <math.h>

#include "external/gflags/gflags.h"
#include <gtest/gtest.h>

using namespace thrift::test::debug;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace std;

namespace thrift { namespace test { namespace debug {

bool Empty::operator<(Empty const& other) const {
  // It is empty, so all are equal.
  return false;
}

}}}

cpp2::OneOfEach ooe;
unique_ptr<IOBuf> buf;

template <typename Object>
void testOoe(Object& ooe) {
  ASSERT_EQ(ooe.im_true, true);
  ASSERT_EQ(ooe.im_false, false);
  ASSERT_EQ((uint8_t)ooe.a_bite, 0xd6);
  ASSERT_EQ(ooe.integer16, 27000);
  ASSERT_EQ(ooe.integer32, 1 << 24);
  ASSERT_EQ(ooe.integer64, (uint64_t)6000 * 1000 * 1000);
  ASSERT_EQ(ooe.double_precision, M_PI);
  ASSERT_EQ(ooe.float_precision, (float)12.345);
  ASSERT_EQ(ooe.some_characters, "JSON THIS! \"\1");
  ASSERT_EQ(ooe.zomg_unicode, "\xd7\n\a\t");
  ASSERT_EQ(ooe.base64, "\1\2\3\255");
  ASSERT_EQ(ooe.rank_map.size(), 2);
  ASSERT_EQ(ooe.rank_map[567419810], (float)0.211184);
  ASSERT_EQ(ooe.rank_map[507959914], (float)0.080382);
}

template <typename Cpp2Writer, typename Cpp2Reader, typename CppProtocol>
void runTest() {
  cpp2::OneOfEach ooe2;
  Cpp2Writer prot;
  Cpp2Reader protReader;

  // Verify writing with cpp2
  size_t bufSize = ooe.serializedSize(&prot);
  IOBufQueue queue(IOBufQueue::cacheChainLength());

  prot.setOutput(&queue, bufSize);
  ooe.write(&prot);

  bufSize = queue.chainLength();
  auto buf = queue.move();

  // Try deserialize with cpp2
  protReader.setInput(buf.get());
  ooe2.read(&protReader);
  testOoe(ooe2);

  // Try deserialize with cpp
  std::shared_ptr<TTransport> buf2(
    new TMemoryBuffer(buf->writableData(), bufSize));
  CppProtocol cppProt(buf2);
  OneOfEach cppOoe;
  cppOoe.read(&cppProt);

  // Try to serialize with cpp
  buf2.reset(new TMemoryBuffer());
  CppProtocol cppProt2(buf2);
  cppOoe.write(&cppProt2);

  std::string buffer = dynamic_pointer_cast<TMemoryBuffer>(
    buf2)->getBufferAsString();
  std::unique_ptr<folly::IOBuf> buf3(
    folly::IOBuf::wrapBuffer(buffer.data(), buffer.size()));

  protReader.setInput(buf3.get());
  ooe2.read(&protReader);
  testOoe(ooe2);
}

TEST(protocol2, binary) {
  runTest<BinaryProtocolWriter, BinaryProtocolReader, TBinaryProtocol>();
}

TEST(protocol2, compact) {
  runTest<CompactProtocolWriter, CompactProtocolReader, TCompactProtocol>();
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  GFLAGS_INIT(argc, argv);

  ooe.im_true   = true;
  ooe.im_false  = false;
  ooe.a_bite    = 0xd6;
  ooe.integer16 = 27000;
  ooe.integer32 = 1 << 24;
  ooe.integer64 = (uint64_t)6000 * 1000 * 1000;
  ooe.double_precision = M_PI;
  ooe.float_precision  = (float)12.345;
  ooe.some_characters  = "JSON THIS! \"\1";
  ooe.zomg_unicode     = "\xd7\n\a\t";
  ooe.base64 = "\1\2\3\255";
  ooe.string_string_map["one"] = "two";
  ooe.string_string_hash_map["three"] = "four";
  ooe.rank_map[567419810] = (float)0.211184;
  ooe.rank_map[507959914] = (float)0.080382;

  return RUN_ALL_TESTS();
}
