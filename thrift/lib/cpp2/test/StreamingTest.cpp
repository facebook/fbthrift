/*
 * Copyright 2015 Facebook, Inc.
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
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/TestServer.h>

using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;
using namespace apache::thrift::util;
using namespace apache::thrift::async;
using apache::thrift::transport::TTransportException;
using folly::wangle::Observer;
using folly::wangle::ObservablePtr;
using folly::wangle::Error;

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

apache::thrift::TestThriftServerFactory<StreamingServiceInterface> factory;
ScopedServerThread sst(factory.create());

std::shared_ptr<StreamingServiceAsyncClient> getClient(folly::EventBase& eb) {
  auto socket = TAsyncSocket::newSocket(&eb, *sst.getAddress());
  auto channel = HeaderClientChannel::newChannel(socket);
  auto client = std::make_shared<StreamingServiceAsyncClient>(std::move(channel));
  return client;
}

TEST(Streaming, Callback) {
  folly::EventBase eb;
  auto client = getClient(eb);
  const int COUNT = 5;

  int n = 0;
  client->streamingMethod([&n](ClientReceiveState&& state) mutable {
      if (n < COUNT) {
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
  }, COUNT);

  eb.loop();
  EXPECT_EQ(n, COUNT);
}

TEST(Streaming, Observable) {
  folly::EventBase eb;
  auto client = getClient(eb);
  const int COUNT = 5;

  int n = 0;
  ObservablePtr<int> s = client->observable_streamingMethod(COUNT);
  s->observe(Observer<int>::create(
      [&n](int x) mutable {
        // onNext
        EXPECT_LT(n, COUNT);
        EXPECT_EQ(x, n);
        n++;
      },
      [&n](Error e) {
        // onError
        FAIL();
      },
      [&n]() {
        // onCompleted
        EXPECT_EQ(n, COUNT);
      }
  ));

  eb.loop();
  EXPECT_EQ(n, COUNT);
}

TEST(Streaming, Exception) {
  folly::EventBase eb;
  auto client = getClient(eb);
  const int COUNT = 5;

  int n = 0;
  bool error = false;
  ObservablePtr<int> s = client->observable_streamingException(COUNT);
  s->observe(Observer<int>::create(
      [&n](int x) mutable {
        // onNext
        EXPECT_LT(n, COUNT);
        EXPECT_EQ(x, n);
        n++;
      },
      [&error](Error e) {
        // onError
        EXPECT_TRUE(e.is_compatible_with<StreamingException>());
        error = true;
      },
      [&n]() {
        // onCompleted
        FAIL();
      }
  ));

  eb.loop();
  EXPECT_EQ(n, COUNT);
  EXPECT_TRUE(error);
}

TEST(Streaming, GlobalTimeout) {
  folly::EventBase eb;
  auto client = getClient(eb);
  const int COUNT = 5;

  int n = 0;
  bool error = false;
  ObservablePtr<int> s = client->observable_streamingMethod(
      RpcOptions().setTimeout(std::chrono::milliseconds(10)),
      COUNT);
  s->observe(Observer<int>::create(
      [&n](int x) mutable {
        // onNext
        EXPECT_LT(n, COUNT);
        EXPECT_EQ(x, n);
        n++;
      },
      [&error](Error e) {
        // onError
        EXPECT_TRUE(e.with_exception<TTransportException>([](TTransportException& ex) {
          EXPECT_EQ(ex.getType(), TTransportException::TIMED_OUT);
        }));
        error = true;
      },
      [&n]() {
        // onCompleted
        FAIL();
      }
  ));

  eb.loop();
  EXPECT_LT(n, COUNT);
  EXPECT_TRUE(error);
}

TEST(Streaming, ChunkTimeout) {
  folly::EventBase eb;
  auto client = getClient(eb);
  const int COUNT = 5;

  int n = 0;
  bool error = false;
  ObservablePtr<int> s = client->observable_streamingMethod(
      RpcOptions().setChunkTimeout(std::chrono::milliseconds(5)),
      COUNT);
  s->observe(Observer<int>::create(
      [&n](int x) mutable {
        // onNext
        EXPECT_LT(n, COUNT);
        EXPECT_EQ(x, n);
        n++;
      },
      [&error](Error e) {
        // onError
        EXPECT_TRUE(e.with_exception<TTransportException>([](TTransportException& ex) {
          EXPECT_EQ(ex.getType(), TTransportException::TIMED_OUT);
        }));
        error = true;
      },
      [&n]() {
        // onCompleted
        FAIL();
      }
  ));

  eb.loop();
  EXPECT_LT(n, COUNT);
  EXPECT_TRUE(error);
  // n should have been incremented at least once because the delay on
  // the initial chunks is smaller
  EXPECT_GT(n, 0);
}
