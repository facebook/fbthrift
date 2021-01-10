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

#include <thrift/lib/cpp2/async/ClientBufferedStream.h>

#include <folly/io/async/ScopedEventBaseThread.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/async/ServerStream.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/Baton.h>
#endif // FOLLY_HAS_COROUTINES

namespace apache {
namespace thrift {

using namespace ::testing;

// Serializes i as int and pads by i bytes.
auto encode(folly::Try<int>&& i) -> folly::Try<StreamPayload> {
  if (i.hasValue()) {
    folly::IOBufQueue buf;
    CompactSerializer::serialize(*i, &buf);
    buf.allocate(*i);
    return folly::Try<StreamPayload>({buf.move(), {}});
  } else if (i.hasException()) {
    return folly::Try<StreamPayload>(i.exception());
  } else {
    return folly::Try<StreamPayload>();
  }
}
// Retrieves i and drops the padding.
auto decode(folly::Try<StreamPayload>&& i) -> folly::Try<int> {
  if (i.hasValue()) {
    int out;
    CompactSerializer::deserialize<int>(i.value().payload.get(), out);
    return folly::Try<int>(out);
  } else if (i.hasException()) {
    return folly::Try<int>(i.exception());
  } else {
    return folly::Try<int>();
  }
}

struct ServerCallback : StreamServerCallback {
  bool onStreamRequestN(uint64_t) override {
    requested.post();
    return true;
  }
  void onStreamCancel() override {
    std::terminate();
  }
  void resetClientCallback(StreamClientCallback&) override {
    std::terminate();
  }

  folly::coro::Baton requested;
};

struct FirstResponseCb : detail::ClientStreamBridge::FirstResponseCallback {
  void onFirstResponse(
      FirstResponsePayload&&,
      detail::ClientStreamBridge::ClientPtr clientStreamBridge) override {
    ptr = std::move(clientStreamBridge);
  }
  void onFirstResponseError(folly::exception_wrapper) override {
    std::terminate();
  }
  detail::ClientStreamBridge::ClientPtr ptr;
};

struct ClientBufferedStreamTest : public Test {
  folly::ScopedEventBaseThread ebt;
  FirstResponseCb firstResponseCb;
  ServerCallback serverCb;
  StreamClientCallback* client;
  void SetUp() override {
    client = detail::ClientStreamBridge::create(&firstResponseCb);
    std::ignore =
        client->onFirstResponse({nullptr, {}}, ebt.getEventBase(), &serverCb);
  }
};

TEST_F(ClientBufferedStreamTest, Inline) {
  ClientBufferedStream<int> stream(std::move(firstResponseCb.ptr), decode, 2);
  ebt.getEventBase()->runInEventBaseThreadAndWait([&] {
    for (int i = 1; i <= 10; ++i) {
      std::ignore = client->onStreamNext(*encode(folly::Try(i)));
    }
    client->onStreamComplete();
  });

  int i = 0;
  std::move(stream).subscribeInline([&](auto val) {
    if (val.hasValue()) {
      EXPECT_EQ(*val, ++i);
    }
  });
  EXPECT_EQ(i, 10);
}

TEST_F(ClientBufferedStreamTest, InlineCancel) {
  ClientBufferedStream<int> stream(std::move(firstResponseCb.ptr), decode, 2);
  ebt.getEventBase()->runInEventBaseThreadAndWait([&] {
    for (int i = 1; i <= 10; ++i) {
      std::ignore = client->onStreamNext(*encode(folly::Try(i)));
    }
    client->onStreamComplete();
  });

  int i = 0;
  std::move(stream).subscribeInline([&](auto val) {
    if (val.hasValue()) {
      EXPECT_EQ(*val, ++i);
    }
    return i != 6;
  });
  EXPECT_EQ(i, 6);
}

} // namespace thrift
} // namespace apache
