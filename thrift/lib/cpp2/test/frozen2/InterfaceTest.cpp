/*
 * Copyright 2004-present Facebook, Inc.
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

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/test/util/TestThriftServerFactory.h>

#include <thrift/lib/cpp2/test/frozen2/interface/gen-cpp2/InterfaceService.h>

#include <gtest/gtest.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::async;
using namespace ::apache::thrift::util;
using namespace ::apache::thrift::protocol;
using namespace ::apache::thrift::frozen;
using namespace ::test::frozen2;

template <typename T>
using PtrOf = std::unique_ptr<typename Layout<T>::View>;

class Handler : public InterfaceServiceSvIf {
 public:
  void testPrimitives(
      ReturnPrimitives& ret,
      int64_t i64Arg,
      double doubleArg,
      bool boolArg) override {
    ret.type = RetType::THAWED;
    ret.i64Arg = i64Arg;
    ret.doubleArg = doubleArg;
    ret.boolArg = boolArg;
  }

  void frozen2_testPrimitives(
      ReturnPrimitives& ret,
      PtrOf<int64_t> i64Arg,
      PtrOf<double> doubleArg,
      PtrOf<bool> boolArg) override {
    ret.type = RetType::FROZEN;
    ret.i64Arg = *i64Arg;
    ret.doubleArg = *doubleArg;
    ret.boolArg = *boolArg;
  }

  void testString(ReturnString& ret, std::unique_ptr<std::string> strArg)
      override {
    ret.type = RetType::THAWED;
    ret.strArg = std::move(*strArg);
  }

  void frozen2_testString(ReturnString& ret, PtrOf<std::string> strArg)
      override {
    ret.type = RetType::FROZEN;
    ret.strArg = std::string(strArg->begin(), strArg->end());
  }

  void testContainers(
      ReturnContainers& ret,
      std::unique_ptr<std::vector<int32_t>> listArg,
      std::unique_ptr<std::set<int32_t>> stArg,
      std::unique_ptr<std::map<int32_t, std::string>> mapArg) override {
    ret.type = RetType::THAWED;
    ret.listArg = std::move(*listArg);
    ret.stArg = std::move(*stArg);
    ret.mapArg = std::move(*mapArg);
  }

  void frozen2_testContainers(
      ReturnContainers& ret,
      PtrOf<std::vector<int32_t>> listArg,
      PtrOf<std::set<int32_t>> stArg,
      PtrOf<std::map<int32_t, std::string>> mapArg) override {
    ret.type = RetType::FROZEN;
    ret.listArg = std::vector<int32_t>(listArg->begin(), listArg->end());
    ret.stArg = stArg->thaw();
    ret.mapArg = mapArg->thaw();
  }

  void testStructs(
      ReturnStructs& ret,
      std::unique_ptr<TestStruct> structArg,
      TestEnum enumArg) override {
    ret.type = RetType::THAWED;
    ret.structArg = std::move(*structArg);
    ret.enumArg = enumArg;
  }

  void frozen2_testStructs(
      ReturnStructs& ret,
      PtrOf<TestStruct> structArg,
      PtrOf<TestEnum> enumArg) override {
    ret.type = RetType::FROZEN;
    ret.structArg = structArg->thaw();
    ret.enumArg = *enumArg;
  }

  void testVoid(ReturnVoid& ret) override {
    ret.type = RetType::THAWED;
  }

  void frozen2_testVoid(ReturnVoid& ret) override {
    ret.type = RetType::FROZEN;
  }
};

class InterfaceTest : public ::testing::Test {
 protected:
  void SetUp() {
    auto server = std::make_shared<apache::thrift::ThriftServer>();
    handler_ = std::make_shared<Handler>();
    server->setPort(0);
    server->setInterface(handler_);
    sst_ = std::make_unique<ScopedServerThread>(server);
  }

  std::unique_ptr<InterfaceServiceAsyncClient> getClient(
      PROTOCOL_TYPES protType) {
    auto socket = TAsyncSocket::newSocket(&eb_, *(sst_->getAddress()));
    auto channel = HeaderClientChannel::newChannel(socket);
    channel->setProtocolId(protType);
    return std::make_unique<InterfaceServiceAsyncClient>(std::move(channel));
  }

  folly::EventBase eb_;
  std::unique_ptr<ScopedServerThread> sst_;
  std::shared_ptr<Handler> handler_;
};

TEST_F(InterfaceTest, TestPrimitives) {
  auto frozenClient = getClient(PROTOCOL_TYPES::T_FROZEN2_PROTOCOL);
  ReturnPrimitives ret_frozen;
  frozenClient->sync_testPrimitives(ret_frozen, 0xBAD, 42.15, true);
  ASSERT_EQ(ret_frozen.type, RetType::FROZEN);
  ASSERT_EQ(ret_frozen.i64Arg, 0xBAD);
  ASSERT_EQ(ret_frozen.doubleArg, 42.15);
  ASSERT_TRUE(ret_frozen.boolArg);

  auto compactClient = getClient(PROTOCOL_TYPES::T_COMPACT_PROTOCOL);
  ReturnPrimitives ret_compact;
  compactClient->sync_testPrimitives(ret_compact, 0xBAD, 42.15, true);
  ASSERT_EQ(ret_compact.type, RetType::THAWED);
  ASSERT_EQ(ret_compact.i64Arg, 0xBAD);
  ASSERT_EQ(ret_compact.doubleArg, 42.15);
  ASSERT_TRUE(ret_compact.boolArg);
}

TEST_F(InterfaceTest, TestString) {
  auto frozenClient = getClient(PROTOCOL_TYPES::T_FROZEN2_PROTOCOL);
  ReturnString ret_frozen;
  frozenClient->sync_testString(ret_frozen, "Meh");
  ASSERT_EQ(ret_frozen.type, RetType::FROZEN);
  ASSERT_EQ(ret_frozen.strArg, "Meh");

  auto compactClient = getClient(PROTOCOL_TYPES::T_COMPACT_PROTOCOL);
  ReturnString ret_compact;
  compactClient->sync_testString(ret_compact, "Meh");
  ASSERT_EQ(ret_compact.type, RetType::THAWED);
  ASSERT_EQ(ret_compact.strArg, "Meh");
}

TEST_F(InterfaceTest, TestContainers) {
  std::vector<int32_t> vec = {0, 1};
  std::set<int32_t> st = {1, 2};
  std::map<int32_t, std::string> mp = {{0, "Meh"}};

  auto frozenClient = getClient(PROTOCOL_TYPES::T_FROZEN2_PROTOCOL);
  ReturnContainers ret_frozen;
  frozenClient->sync_testContainers(ret_frozen, vec, st, mp);
  ASSERT_EQ(ret_frozen.type, RetType::FROZEN);
  ASSERT_EQ(ret_frozen.listArg, vec);
  ASSERT_EQ(ret_frozen.stArg, st);
  ASSERT_EQ(ret_frozen.mapArg, mp);

  auto compactClient = getClient(PROTOCOL_TYPES::T_COMPACT_PROTOCOL);
  ReturnContainers ret_compact;
  compactClient->sync_testContainers(ret_compact, vec, st, mp);
  ASSERT_EQ(ret_compact.type, RetType::THAWED);
  ASSERT_EQ(ret_compact.listArg, vec);
  ASSERT_EQ(ret_compact.stArg, st);
  ASSERT_EQ(ret_compact.mapArg, mp);
}

TEST_F(InterfaceTest, TestStructs) {
  TestStruct strct;
  strct.i32Field = 0;

  auto frozenClient = getClient(PROTOCOL_TYPES::T_FROZEN2_PROTOCOL);
  ReturnStructs ret_frozen;
  frozenClient->sync_testStructs(ret_frozen, strct, TestEnum::Bar);
  ASSERT_EQ(ret_frozen.type, RetType::FROZEN);
  ASSERT_EQ(ret_frozen.structArg, strct);
  ASSERT_EQ(ret_frozen.enumArg, TestEnum::Bar);

  auto compactClient = getClient(PROTOCOL_TYPES::T_COMPACT_PROTOCOL);
  ReturnStructs ret_compact;
  compactClient->sync_testStructs(ret_compact, strct, TestEnum::Bar);
  ASSERT_EQ(ret_compact.type, RetType::THAWED);
  ASSERT_EQ(ret_compact.structArg, strct);
  ASSERT_EQ(ret_compact.enumArg, TestEnum::Bar);
}

TEST_F(InterfaceTest, testVoid) {
  auto frozenClient = getClient(PROTOCOL_TYPES::T_FROZEN2_PROTOCOL);
  ReturnVoid ret_frozen;
  frozenClient->sync_testVoid(ret_frozen);
  ASSERT_EQ(ret_frozen.type, RetType::FROZEN);

  auto compactClient = getClient(PROTOCOL_TYPES::T_FROZEN2_PROTOCOL);
  ReturnVoid ret_compact;
  compactClient->sync_testVoid(ret_frozen);
  ASSERT_EQ(ret_compact.type, RetType::THAWED);
}
