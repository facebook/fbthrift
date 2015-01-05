/*
 * Copyright 2014 Facebook, Inc.
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

#include <limits>

#include <folly/json.h>
#include <gtest/gtest.h>

#include <thrift/lib/cpp/ClientUtil.h>
#include <thrift/lib/cpp/async/TEventServer.h>
#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/util/TEventServerCreator.h>
#include <thrift/lib/thrift/gen-cpp/dynamic_types.h>
#include <thrift/test/gen-cpp/DynamicTestService.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::protocol;
using namespace apache::thrift::server;
using namespace apache::thrift::transport;
using namespace apache::thrift::util;

static dynamic kDynamics[] = {
  // NULL
  nullptr,

  // BOOL
  false,
  true,

  // INT
  numeric_limits<int64_t>::min(),
  -1,
  0,
  1,
  numeric_limits<int64_t>::max(),

  // DOUBLE
  numeric_limits<double>::lowest(),
  numeric_limits<double>::min(),
  -1.0,
  0.0,
  1.0,
  numeric_limits<double>::max(),
  numeric_limits<double>::epsilon(),

  // STRING
  "",
  std::string(10, '\0'),
  "Hello World",

  // ARRAY
  { },
  { nullptr },
  { 0 },
  { nullptr, 0, false, 0.0, "", { }, dynamic::object },
  { 1, true, 3.14, "Goodnight Moon", { 1 }, dynamic::object("a", "A")},

  // OBJECT
  dynamic::object,

  dynamic::object
    (std::string(10, '\0'), nullptr),

  dynamic::object
    ("a", nullptr)
    ("b", false)
    ("c", 0)
    ("d", 0.0)
    ("e", "")
    ("f", { })
    ("g", dynamic::object),

  dynamic::object
    ("b", true)
    ("c", 1)
    ("d", 3.14)
    ("e", "Goodnight Moon")
    ("f", { 1 })
    ("g", dynamic::object("a", "A")),
};


class TestServiceHandler : public DynamicTestServiceIf {
 public:
  void echo(SerializableDynamic& out, const SerializableDynamic& in) {
    out = in;
  }
};

std::shared_ptr<TServer> getServer() {
  auto handler = std::make_shared<TestServiceHandler>();
  auto processor = std::make_shared<DynamicTestServiceProcessor>(handler);
  TEventServerCreator serverCreator(processor, 0);
  std::shared_ptr<TEventServer> server(serverCreator.createEventServer());
  return server;
}

class RoundtripTestFixture : public ::testing::TestWithParam<dynamic> {
};

////////////////////////////////////////////////////////////////////////////////

TEST_P(RoundtripTestFixture, RoundtripDynamics) {
  const SerializableDynamic expected = GetParam();

  auto transport = make_shared<TMemoryBuffer>();
  auto protocol = make_shared<TBinaryProtocol>(transport);
  expected.write(protocol.get());
  protocol->getTransport()->flush();

  SerializableDynamic actual;
  actual.read(protocol.get());
  EXPECT_EQ(expected, actual) << "Expected: " << toJson(*expected)
                              << " Actual: " << toJson(*actual);
}

TEST_P(RoundtripTestFixture, RoundtripContainer) {
  Container expected;
  expected.data = GetParam();

  auto transport = make_shared<TMemoryBuffer>();
  auto protocol = make_shared<TBinaryProtocol>(transport);
  expected.write(protocol.get());
  protocol->getTransport()->flush();

  Container actual;
  actual.read(protocol.get());
  EXPECT_EQ(expected, actual) << "Expected: " << toJson(*expected.data)
                              << " Actual: " << toJson(*actual.data);
}

TEST_P(RoundtripTestFixture, SerializeOverHandler) {
  ScopedServerThread sst(getServer());
  shared_ptr<DynamicTestServiceClient> client =
      createClientPtr<DynamicTestServiceClient>(sst.getAddress());

  const SerializableDynamic expected = GetParam();
  SerializableDynamic actual;
  client->echo(actual, expected);
  EXPECT_EQ(expected, actual) << "Expected: " << toJson(*expected)
                              << " Actual: " << toJson(*actual);
}

INSTANTIATE_TEST_CASE_P(All,
                        RoundtripTestFixture,
                        ::testing::ValuesIn(kDynamics));
