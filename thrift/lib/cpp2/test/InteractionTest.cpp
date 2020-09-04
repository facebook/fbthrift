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

#include <memory>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/test/gen-cpp2/HandlerGeneric.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace ::testing;
using namespace apache::thrift;
using namespace apache::thrift::test;

namespace {
class DummyChannel : public RequestChannel {
 public:
  void sendRequestResponse(
      const RpcOptions&,
      folly::StringPiece,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr) override {
    std::terminate();
  }
  void sendRequestNoResponse(
      const RpcOptions&,
      folly::StringPiece,
      SerializedRequest&&,
      std::shared_ptr<apache::thrift::transport::THeader>,
      RequestClientCallback::Ptr) override {
    std::terminate();
  }
  void setCloseCallback(CloseCallback*) override {
    std::terminate();
  }
  folly::EventBase* getEventBase() const override {
    std::terminate();
  }
  uint16_t getProtocolId() override {
    std::terminate();
  }

  void terminateInteraction(InteractionId id) override {
    releaseInteractionId(std::move(id));
  }
  void seedInteractionId(int64_t id) {
    nextId_ = id;
  }
  InteractionId createInteraction(folly::StringPiece) override {
    return createInteractionId(nextId_);
  }

 private:
  int64_t nextId_{0};
};
} // namespace

struct Handler : HandlerGenericSvIf {
  void get_string(std::string& out) override {
    if (auto interaction = getConnectionContext()->getInteractionId()) {
      out = std::to_string(interaction);
      if (auto create = getConnectionContext()->getInteractionCreate()) {
        out += *create->interactionName_ref();
      }
    } else {
      out = "";
    }
  }
};

TEST(InteractionTest, NoIDPropagated) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  RpcOptions rpcOpts;
  std::string out;
  client.sync_get_string(rpcOpts, out);
  EXPECT_EQ(out, "");
}

TEST(InteractionTest, Register) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  folly::via(
      &eb, [&] { client.getChannel()->registerInteraction("Transaction", 42); })
      .getVia(&eb);

  RpcOptions rpcOpts;
  DummyChannel dummy;
  dummy.seedInteractionId(42);
  auto id = dummy.createInteraction("");
  rpcOpts.setInteractionId(id);
  std::string out;
  client.sync_get_string(rpcOpts, out);
  EXPECT_EQ(out, "42Transaction");

  eb.runInEventBaseThread([channel = client.getChannelShared(),
                           ka = folly::getKeepAliveToken(eb),
                           id = std::move(id)]() mutable {
    channel->terminateInteraction(std::move(id));
  });
}

TEST(InteractionTest, Create) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  auto id = folly::via(
                &eb,
                [channel = client.getChannel()] {
                  return channel->createInteraction("Transaction");
                })
                .getVia(&eb);
  RpcOptions rpcOpts;
  rpcOpts.setInteractionId(id);
  std::string out;
  client.sync_get_string(rpcOpts, out);
  EXPECT_EQ(out, "1Transaction");

  DummyChannel().terminateInteraction(std::move(id));
}

TEST(InteractionTest, TerminateUsed) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  folly::via(
      &eb,
      [&, channel = client.getChannel()] {
        channel->registerInteraction("Transaction", 42);
      })
      .getVia(&eb);

  RpcOptions rpcOpts;
  DummyChannel dummy;
  dummy.seedInteractionId(42);
  auto id = dummy.createInteraction("");
  rpcOpts.setInteractionId(id);
  std::string out;
  client.sync_get_string(rpcOpts, out);

  folly::getKeepAliveToken(eb).add([channel = client.getChannelShared(),
                                    id = std::move(id)](auto&&) mutable {
    channel->terminateInteraction(std::move(id));
  });
}

TEST(InteractionTest, TerminateUnused) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  std::string out;
  client.sync_get_string(out); // sends setup frame

  DummyChannel dummy;
  dummy.seedInteractionId(42);
  auto id = dummy.createInteraction("");

  folly::via(
      &eb,
      [&,
       channel = client.getChannel(),
       id = dummy.createInteraction("")]() mutable {
        channel->registerInteraction("Transaction", 42);
        channel->terminateInteraction(std::move(id));
      })
      .getVia(&eb);

  // This is a contract violation. Don't do it!
  RpcOptions rpcOpts;
  rpcOpts.setInteractionId(id);
  dummy.terminateInteraction(std::move(id));
  client.sync_get_string(rpcOpts, out);

  // This checks that we clean up unused interactions in the map
  EXPECT_EQ(out, "42");
}

TEST(InteractionTest, TerminateWithoutSetup) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  folly::EventBase eb;
  HandlerGenericAsyncClient client(
      RocketClientChannel::newChannel(folly::AsyncSocket::UniquePtr(
          new folly::AsyncSocket(&eb, runner.getAddress()))));

  DummyChannel dummy;
  dummy.seedInteractionId(42);

  folly::via(
      &eb,
      [&,
       channel = client.getChannel(),
       id = dummy.createInteraction("")]() mutable {
        channel->registerInteraction("Transaction", 42);
        channel->terminateInteraction(std::move(id));
      })
      .getVia(&eb);

  // This is a contract violation. Don't do it!
  RpcOptions rpcOpts;

  auto id = dummy.createInteraction("");
  rpcOpts.setInteractionId(id);
  dummy.terminateInteraction(std::move(id));
  std::string out;
  client.sync_get_string(rpcOpts, out);

  // This checks that we clean up unused interactions in the map
  EXPECT_EQ(out, "42");
}

TEST(InteractionTest, TerminatePRC) {
  ScopedServerInterfaceThread runner{std::make_shared<Handler>()};
  auto client =
      runner.newClient<HandlerGenericAsyncClient>(nullptr, [](auto socket) {
        return RocketClientChannel::newChannel(std::move(socket));
      });

  RpcOptions rpcOpts;

  auto id = client->getChannel()->createInteraction("Transaction");
  rpcOpts.setInteractionId(id);
  std::string out;
  client->sync_get_string(rpcOpts, out);
  EXPECT_EQ(out, std::to_string(id) + "Transaction");

  client->getChannel()->terminateInteraction(std::move(id));
}
