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

#include <thrift/lib/cpp2/test/gen-cpp2/StreamingService.h>
#include <gtest/gtest.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using apache::thrift::transport::TTransportException;
using wangle::Observer;
using wangle::ObservablePtr;
using wangle::Error;

namespace {

class StreamingServiceInterface : public StreamingServiceSvIf {
 public:
  void async_tm_streamingMethod(
      std::unique_ptr<StreamingHandlerCallback<int32_t>> callback,
      int32_t count) override {
    for (int i = 0; i < count; i++) {
      callback->write(i);
      /* sleep override */ usleep((i+1) * 5*1000);
    }
    //callback->exception(folly::make_exception_wrapper<TApplicationException>("qwerty"));
    callback->done();
  }
  void async_tm_streamingException(
      std::unique_ptr<StreamingHandlerCallback<int32_t>> callback,
      int32_t count) override {
    for (int i = 0; i < count; i++) {
      callback->write(i);
      /* sleep override */ usleep((i+1) * 5*1000);
    }
    callback->exception(folly::make_exception_wrapper<StreamingException>());
  }
};

static constexpr size_t kCount = 5;

class StreamingTest : public testing::Test {
 public:
  std::shared_ptr<StreamingServiceInterface> handler {
    std::make_shared<StreamingServiceInterface>() };
  apache::thrift::ScopedServerInterfaceThread runner { handler };
  folly::EventBase eb;

  std::unique_ptr<StreamingServiceAsyncClient> newClient() {
    return runner.newClient<StreamingServiceAsyncClient>(&eb);
  }
};

}

TEST_F(StreamingTest, Callback) {
  auto client = newClient();

  int n = 0;
  client->streamingMethod([&n](ClientReceiveState&& state) mutable {
      if (n < kCount) {
        EXPECT_FALSE(state.isStreamEnd());
      } else {
        EXPECT_TRUE(state.isStreamEnd());
      }
      // call recv_ before checking for isStreamEnd to distinguish exceptions
      // from normal stream end
      int x = StreamingServiceAsyncClient::recv_streamingMethod(state);
      if (state.isStreamEnd()) {
        return;
      }
      EXPECT_EQ(x, n);
      n++;
  }, kCount);

  eb.loop();
  EXPECT_EQ(n, kCount);
}

TEST_F(StreamingTest, Observable) {
  auto client = newClient();

  int n = 0;
  ObservablePtr<int> s = client->observable_streamingMethod(kCount);
  s->observe(Observer<int>::create(
      [&n](int x) mutable {
        // onNext
        EXPECT_LT(n, kCount);
        EXPECT_EQ(x, n);
        n++;
      },
      [](Error) {
        // onError
        FAIL();
      },
      [&n]() {
        // onCompleted
        EXPECT_EQ(n, kCount);
      }));

  eb.loop();
  EXPECT_EQ(n, kCount);
}

TEST_F(StreamingTest, Exception) {
  auto client = newClient();

  int n = 0;
  bool error = false;
  ObservablePtr<int> s = client->observable_streamingException(kCount);
  s->observe(Observer<int>::create(
      [&n](int x) mutable {
        // onNext
        EXPECT_LT(n, kCount);
        EXPECT_EQ(x, n);
        n++;
      },
      [&error](Error e) {
        // onError
        EXPECT_TRUE(e.is_compatible_with<StreamingException>());
        error = true;
      },
      []() {
        // onCompleted
        FAIL();
      }
  ));

  eb.loop();
  EXPECT_EQ(n, kCount);
  EXPECT_TRUE(error);
}

TEST_F(StreamingTest, GlobalTimeout) {
  auto client = newClient();

  int n = 0;
  bool error = false;
  ObservablePtr<int> s = client->observable_streamingMethod(
      RpcOptions().setTimeout(std::chrono::milliseconds(10)),
      kCount);
  s->observe(Observer<int>::create(
      [&n](int x) mutable {
        // onNext
        EXPECT_LT(n, kCount);
        EXPECT_EQ(x, n);
        n++;
      },
      [&error](Error e) {
        // onError
        EXPECT_TRUE(e.with_exception([](TTransportException& ex) {
          EXPECT_EQ(ex.getType(), TTransportException::TIMED_OUT);
        }));
        error = true;
      },
      []() {
        // onCompleted
        FAIL();
      }
  ));

  eb.loop();
  EXPECT_LT(n, kCount);
  EXPECT_TRUE(error);
}

TEST_F(StreamingTest, ChunkTimeout) {
  auto client = newClient();

  int n = 0;
  bool error = false;
  ObservablePtr<int> s = client->observable_streamingMethod(
      RpcOptions().setChunkTimeout(std::chrono::milliseconds(5)),
      kCount);
  s->observe(Observer<int>::create(
      [&n](int x) mutable {
        // onNext
        EXPECT_LT(n, kCount);
        EXPECT_EQ(x, n);
        n++;
      },
      [&error](Error e) {
        // onError
        EXPECT_TRUE(e.with_exception([](TTransportException& ex) {
          EXPECT_EQ(ex.getType(), TTransportException::TIMED_OUT);
        }));
        error = true;
      },
      []() {
        // onCompleted
        FAIL();
      }
  ));

  eb.loop();
  EXPECT_LT(n, kCount);
  EXPECT_TRUE(error);
  // n should have been incremented at least once because the delay on
  // the initial chunks is smaller
  EXPECT_GT(n, 0);
}
