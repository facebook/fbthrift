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

#include <folly/init/Init.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <thrift/lib/cpp2/transport/core/testutil/TestServiceClientExtension.h>
#include <thrift/lib/cpp2/transport/core/testutil/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/transport/rsocket/client/RSThriftClient.h>
#include <thrift/lib/cpp2/transport/util/ConnectionManager.h>
#include <yarpl/Flowable.h>

#include <iostream>

using namespace apache::thrift;
using namespace testutil::testservice;

DEFINE_string(host, "::1", "host to connect to");
DEFINE_int32(port, 7777, "Port for the TestRoomService");

// ConnectionManager depends on this flag.
DECLARE_string(transport);

int main(int argc, char** argv) {
  folly::init(&argc, &argv);

  FLAGS_transport = "rsocket";

  try {
    auto mgr = ConnectionManager::getInstance();

    auto connection = mgr->getConnection(FLAGS_host, FLAGS_port);
    auto channel = RSThriftClient::Ptr(new RSThriftClient(connection));
    channel->setProtocolId(apache::thrift::protocol::T_COMPACT_PROTOCOL);

    auto client =
        std::make_unique<TestServiceAsyncClientExtended>(std::move(channel));

    // Channel - say hello
    auto input = yarpl::flowable::Flowables::justN(
        {std::string("Fuat"), std::string("Scott"), std::string("Dylan")});
    auto result = client->helloChannel(input);
    // Streaming PRC calls are lazy, they will happen only When
    // the result is subscribed to.
    result->subscribe(
        [](const std::string& str) { std::cout << str << std::endl; });

    getchar();
  } catch (apache::thrift::transport::TTransportException& ex) {
    LOG(ERROR) << "Request failed: " << ex.what();
  } catch (TestServiceException& ex) {
    LOG(ERROR) << "Request failed: " << ex.what();
  }

  return 0;
}
