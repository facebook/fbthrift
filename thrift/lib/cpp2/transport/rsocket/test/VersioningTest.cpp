/*
 * Copyright 2018-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/rsocket/test/util/TestUtil.h>
#include <thrift/lib/cpp2/transport/rsocket/test/util/VersionServicesMock.h>

namespace apache {
namespace thrift {

// Testing transport layers for resilience to changes in the service interface
class VersioningTest : public TestSetup {
 private:
 public:
  void SetUp() override {
    // Setup two servers, for VersionOld and VersionNew
    handlerOld_ = std::make_shared<StrictMock<OldServiceMock>>();
    serverOld_ = createServer(
        std::make_shared<ThriftServerAsyncProcessorFactory<OldServiceMock>>(
            handlerOld_),
        portOld_);

    handlerNew_ = std::make_shared<StrictMock<NewServiceMock>>();
    serverNew_ = createServer(
        std::make_shared<ThriftServerAsyncProcessorFactory<NewServiceMock>>(
            handlerNew_),
        portNew_);
  }

  void TearDown() override {
    if (serverOld_) {
      serverOld_->cleanUp();
      serverOld_.reset();
      handlerOld_.reset();
    }
    if (serverNew_) {
      serverNew_->cleanUp();
      serverNew_.reset();
      handlerNew_.reset();
    }
  }

  void connectToOldServer(
      folly::Function<void(std::unique_ptr<OldVersionAsyncClient>)> callMe,
      folly::Function<void()> onDetachable = nullptr) {
    auto channel = connectToServer(portOld_, std::move(onDetachable));
    callMe(std::make_unique<OldVersionAsyncClient>(std::move(channel)));
  }

  void connectToOldServer(
      folly::Function<void(std::unique_ptr<NewVersionAsyncClient>)> callMe,
      folly::Function<void()> onDetachable = nullptr) {
    auto channel = connectToServer(portOld_, std::move(onDetachable));
    callMe(std::make_unique<NewVersionAsyncClient>(std::move(channel)));
  }

  void connectToNewServer(
      folly::Function<void(std::unique_ptr<OldVersionAsyncClient>)> callMe,
      folly::Function<void()> onDetachable = nullptr) {
    auto channel = connectToServer(portNew_, std::move(onDetachable));
    callMe(std::make_unique<OldVersionAsyncClient>(std::move(channel)));
  }

  void connectToNewServer(
      folly::Function<void(std::unique_ptr<NewVersionAsyncClient>)> callMe,
      folly::Function<void()> onDetachable = nullptr) {
    auto channel = connectToServer(portNew_, std::move(onDetachable));
    callMe(std::make_unique<NewVersionAsyncClient>(std::move(channel)));
  }

 protected:
  std::unique_ptr<ThriftServer> serverOld_;
  std::unique_ptr<ThriftServer> serverNew_;
  std::shared_ptr<testing::StrictMock<OldServiceMock>> handlerOld_;
  std::shared_ptr<testing::StrictMock<NewServiceMock>> handlerNew_;
  uint16_t portOld_;
  uint16_t portNew_;

  folly::ScopedEventBaseThread executor_;
};

TEST_F(VersioningTest, SameRequestResponse) {
  connectToOldServer([](std::unique_ptr<OldVersionAsyncClient> client) {
    EXPECT_EQ(6, client->sync_AddOne(5));
  });
  connectToNewServer([](std::unique_ptr<OldVersionAsyncClient> client) {
    EXPECT_EQ(-1, client->sync_AddOne(-2));
  });
  connectToOldServer([](std::unique_ptr<NewVersionAsyncClient> client) {
    EXPECT_EQ(6, client->sync_AddOne(5));
  });
  connectToNewServer([](std::unique_ptr<NewVersionAsyncClient> client) {
    EXPECT_EQ(6, client->sync_AddOne(5));
  });
}

TEST_F(VersioningTest, SameStream) {
  auto oldLambda = [this](std::unique_ptr<OldVersionAsyncClient> client) {
    auto stream = client->sync_Range(5, 5);
    auto subscription =
        std::move(stream)
            .via(&executor_)
            .subscribe(
                [init = 5](auto value) mutable { EXPECT_EQ(init++, value); },
                [](auto) { FAIL(); });
    std::move(subscription).join();
  };

  auto newLambda = [this](std::unique_ptr<NewVersionAsyncClient> client) {
    auto stream = client->sync_Range(5, 5);
    auto subscription =
        std::move(stream)
            .via(&executor_)
            .subscribe(
                [init = 5](auto value) mutable { EXPECT_EQ(init++, value); },
                [](auto) { FAIL(); });
    std::move(subscription).join();
  };

  connectToOldServer(oldLambda);
  connectToNewServer(oldLambda);
  connectToOldServer(newLambda);
  connectToNewServer(newLambda);
}

TEST_F(VersioningTest, SameResponseAndStream) {
  auto oldLambda = [this](std::unique_ptr<OldVersionAsyncClient> client) {
    auto streamAndResponse = client->sync_RangeAndAddOne(5, 5, -2);
    EXPECT_EQ(-1, streamAndResponse.response);
    auto subscription =
        std::move(streamAndResponse.stream)
            .via(&executor_)
            .subscribe(
                [init = 5](auto value) mutable { EXPECT_EQ(init++, value); },
                [](auto) { FAIL(); });
    std::move(subscription).join();
  };

  auto newLambda = [this](std::unique_ptr<NewVersionAsyncClient> client) {
    auto streamAndResponse = client->sync_RangeAndAddOne(5, 5, -2);
    EXPECT_EQ(-1, streamAndResponse.response);
    auto subscription =
        std::move(streamAndResponse.stream)
            .via(&executor_)
            .subscribe(
                [init = 5](auto value) mutable { EXPECT_EQ(init++, value); },
                [](auto) { FAIL(); });
    std::move(subscription).join();
  };

  connectToOldServer(oldLambda);
  connectToNewServer(oldLambda);
  connectToOldServer(newLambda);
  connectToNewServer(newLambda);
}

} // namespace thrift
} // namespace apache
