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

#include <chrono>

#include <fmt/core.h>
#include <glog/logging.h>
#include <gtest/gtest.h>
#include <folly/Subprocess.h>
#include <folly/synchronization/Baton.h>
#include <thrift/lib/cpp2/test/e2e/gen-cpp2/TestBiDiService.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

namespace apache::thrift {

struct HandlerResult {
  std::vector<std::string> receivedItems;
  std::string terminationReason;
  folly::Baton<> done;
};

struct EchoHandler : public ServiceHandler<detail::test::TestBiDiService> {
  explicit EchoHandler(std::shared_ptr<HandlerResult> r)
      : result(std::move(r)) {}

  folly::coro::Task<StreamTransformation<std::string, std::string>> co_echo()
      override {
    co_return StreamTransformation<std::string, std::string>{
        [result =
             this->result](folly::coro::AsyncGenerator<std::string&&> input)
            -> folly::coro::AsyncGenerator<std::string&&> {
          try {
            while (auto item = co_await input.next()) {
              result->receivedItems.push_back(*item);
              co_yield std::move(*item);
            }
            result->terminationReason = "completed";
          } catch (const std::exception& e) {
            result->terminationReason = fmt::format("exception: {}", e.what());
          }
          result->done.post();
        }};
  }

  std::shared_ptr<HandlerResult> result;
};

TEST(BiDiClientCrashTest, ServerUnblocksAfterClientCrash) {
  auto result = std::make_shared<HandlerResult>();
  ScopedServerInterfaceThread server(std::make_shared<EchoHandler>(result));
  const auto port = server.getAddress().getPort();

  const auto* clientBinary = std::getenv("BIDI_CRASH_CLIENT");
  ASSERT_NE(clientBinary, nullptr)
      << "BIDI_CRASH_CLIENT env var not set. "
      << "Run via buck test to have it set automatically.";

  folly::Subprocess clientProc(
      {clientBinary, std::to_string(port), "3"},
      folly::Subprocess::Options().pipeStdout().pipeStderr());

  const auto output = clientProc.communicate();
  LOG(INFO) << "Client stdout: " << output.first;
  LOG(INFO) << "Client stderr: " << output.second;

  const auto returnCode = clientProc.wait();
  EXPECT_TRUE(returnCode.killed());
  EXPECT_EQ(returnCode.killSignal(), SIGKILL);

  ASSERT_TRUE(result->done.try_wait_for(std::chrono::seconds(30)))
      << "Server handler did not unblock within 30s after client crash"
      << " — the server is stuck!";

  LOG(INFO) << "Server handler termination reason: "
            << result->terminationReason;
  EXPECT_NE(result->terminationReason.find("exception"), std::string::npos)
      << "Expected exception but got: " << result->terminationReason;

  EXPECT_EQ(result->receivedItems.size(), 3);
  for (size_t i = 0; i < result->receivedItems.size(); ++i) {
    EXPECT_EQ(result->receivedItems[i], fmt::format("item-{}", i));
  }
}

} // namespace apache::thrift
