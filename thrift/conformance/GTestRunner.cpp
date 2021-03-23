/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <thrift/conformance/GTestHarness.h>

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/Subprocess.h>
#include <folly/Synchronized.h>
#include <folly/io/async/AsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/stop_watch.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace apache::thrift::conformance {
namespace {

enum class ChannelType {
  Header = 1,
  Rocket,
};

// Gets and env value, returning dflt if not found.
const char* getEnvOr(const char* name, const char* dflt) {
  if (const char* value = std::getenv(name)) {
    return value;
  }
  return dflt;
}

// Gets an env value, throwing an exception if not found.
const char* getEnvOrThrow(const char* name) {
  if (const char* value = std::getenv(name)) {
    return value;
  }
  folly::throw_exception<std::runtime_error>(
      fmt::format("{} environment variable is required.", name));
}

// Creates a client for the localhost.
std::unique_ptr<ConformanceServiceAsyncClient> createClient(
    folly::EventBase* eb, int port, ChannelType type = ChannelType::Header) {
  folly::AsyncTransport::UniquePtr socket(
      new folly::AsyncSocket(eb, folly::SocketAddress("::1", port)));
  switch (type) {
    case ChannelType::Header:
      return std::make_unique<ConformanceServiceAsyncClient>(
          HeaderClientChannel::newChannel(std::move(socket)));
    case ChannelType::Rocket:
      return std::make_unique<ConformanceServiceAsyncClient>(
          RocketClientChannel::newChannel(std::move(socket)));
    default:
      throw std::invalid_argument(
          "Unknown channel type: " + std::to_string(int(type)));
  }
}

// Bundles a server process and client.
class ClientAndServer {
 public:
  explicit ClientAndServer(std::string cmd)
      : server_(
            std::vector<std::string>{std::move(cmd)},
            folly::Subprocess::Options().pipeStdout()) {
    LOG(INFO) << "Starting binary: " << cmd;
    std::string port;
    server_.communicate(
        folly::Subprocess::readLinesCallback(
            [&port](int, folly::StringPiece s) {
              port = std::string(s);
              return true;
            }),
        [](int, int) { return true; });
    LOG(INFO) << "Using port: " << port;
    client_ = createClient(&eb_, folly::to<int>(port));
  }

  ~ClientAndServer() {
    server_.sendSignal(SIGINT);
    server_.waitOrTerminateOrKill(
        std::chrono::seconds(1), std::chrono::seconds(1));
  }

  ConformanceServiceAsyncClient& getClient() { return *client_; }

 private:
  folly::EventBase eb_;
  folly::Subprocess server_;
  std::unique_ptr<ConformanceServiceAsyncClient> client_;
};

// Reads the contents of a file into a string.
// Returns "" if filename == nullptr.
std::string readFromFile(const char* filename) {
  if (filename == nullptr) {
    return {};
  }
  std::string data;
  if (!folly::readFile(filename, data)) {
    folly::throw_exception<std::invalid_argument>(
        fmt::format("Could not read file: {}", filename));
  }
  return data;
}

// Runs the given command and returns its stdout as a string.
std::string readFromCmd(const std::vector<std::string>& argv) {
  folly::Subprocess proc(argv, folly::Subprocess::Options().pipeStdout());
  std::string result = proc.communicate().first;
  // NOTE: proc.waitOrTermiateOrKill() logs, so we can't use it before main().
  proc.sendSignal(SIGINT);
  if (proc.waitTimeout(std::chrono::seconds(1)).running()) {
    proc.terminate();
    if (proc.waitTimeout(std::chrono::seconds(1)).running()) {
      proc.kill();
      proc.wait();
    }
  }

  return result;
}

// Reads or generates the test suites to run.
std::vector<TestSuite> getSuites() {
  std::vector<TestSuite> result;

  std::vector<std::string> suiteGens;
  folly::split(
      ",", getEnvOr("THRIFT_CONFORMANCE_TEST_SUITE_GENS", ""), suiteGens);
  for (auto cmd : suiteGens) {
    if (!cmd.empty()) {
      result.emplace_back(BinarySerializer::deserialize<TestSuite>(
          readFromCmd({std::move(cmd)})));
    }
  }
  if (result.empty()) {
    folly::throw_exception<std::invalid_argument>(fmt::format(
        "No test suites found:\n"
        "  THRIFT_CONFORMANCE_TEST_SUITE_GENS={}",
        getEnvOr("THRIFT_CONFORMANCE_TEST_SUITE_GENS", "<unset>")));
  }
  return result;
}

// Read the list of non-conforming tests.
std::set<std::string> getNonconforming() {
  return parseNonconforming(
      readFromFile(std::getenv("THRIFT_CONFORMANCE_NONCONFORMING")));
}

// Creates a map from name to client provider, using lazily initalized
// ClientAndServers.
client_fn_map getServers() {
  auto cmds = parseCmds(getEnvOrThrow("THRIFT_CONFORMANCE_SERVER_BINARIES"));
  client_fn_map result;
  for (const auto& entry : cmds) {
    result.emplace(
        entry.first,
        [name = std::string(entry.first),
         cmd = std::string(entry.second)]() -> ConformanceServiceAsyncClient& {
          static folly::Synchronized<
              std::map<std::string_view, std::unique_ptr<ClientAndServer>>>
              clients;
          auto lockedClients = clients.wlock();

          // Get or create ClientAndServer in the static map.
          auto itr = lockedClients->find(name);
          if (itr == lockedClients->end()) {
            itr = lockedClients->emplace_hint(
                itr, name, std::make_unique<ClientAndServer>(cmd));
          }
          return itr->second->getClient();
        });
  }
  return result;
}

// Register the tests with gtest.
THRIFT_CONFORMANCE_TEST(getSuites(), getServers(), getNonconforming());

} // namespace
} // namespace apache::thrift::conformance
