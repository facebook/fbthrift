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

#include <memory>
#include <string>
#include <utility>

#include <gtest/gtest.h>

#include <folly/Function.h>
#include <folly/Range.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/ScopedEventBaseThread.h>

#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/async/RSocketClientChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/async/SemiStream.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <thrift/lib/cpp2/transport/rsocket/test/util/gen-cpp2/StreamService.h>

#include <yarpl/flowable/Flowable.h>
#include <yarpl/flowable/TestSubscriber.h>

namespace apache {
namespace thrift {

using namespace apache::thrift;
using namespace rsocket;
using namespace yarpl::flowable;
using namespace testutil::testservice;

namespace {

using Pair = std::
    pair<Payload, std::shared_ptr<Flowable<std::unique_ptr<folly::IOBuf>>>>;

template <class T>
T makePayload(folly::StringPiece data);
template <>
rsocket::Payload makePayload<rsocket::Payload>(folly::StringPiece data) {
  return rsocket::Payload{data};
}
template <>
rocket::Payload makePayload<rocket::Payload>(folly::StringPiece data) {
  return rocket::Payload::makeFromData(data);
}

template <class PayloadT>
std::string dataToString(PayloadT payload);
template <>
std::string dataToString<rsocket::Payload>(rsocket::Payload payload) {
  return payload.moveDataToString();
}
template <>
std::string dataToString<rocket::Payload>(rocket::Payload payload) {
  return folly::StringPiece(std::move(payload).data()->coalesce()).toString();
}
/// Construct a pipeline with a test subscriber against the supplied
/// flowable.  Return the items that were sent to the subscriber.  If some
/// exception was sent, the exception is thrown.
template <typename T>
std::vector<T> run(
    std::shared_ptr<Flowable<T>> flowable,
    int64_t requestCount = 100) {
  auto subscriber = std::make_shared<TestSubscriber<T>>(requestCount);
  flowable->subscribe(subscriber);
  subscriber->awaitTerminalEvent(std::chrono::seconds(1));
  return std::move(subscriber->values());
}
} // namespace

TEST(YarplStreamImplTest, Basic) {
  folly::ScopedEventBaseThread evbThread;

  auto flowable = Flowable<>::justN({12, 34, 56, 98});
  auto stream = toStream(std::move(flowable), evbThread.getEventBase());
  auto stream2 = std::move(stream).map([&](int x) {
    EXPECT_TRUE(evbThread.getEventBase()->inRunningEventBaseThread());
    return x * 2;
  });

  auto flowable2 = toFlowable(std::move(stream2));

  EXPECT_EQ(run(flowable2), std::vector<int>({12 * 2, 34 * 2, 56 * 2, 98 * 2}));
}

TEST(YarplStreamImplTest, SemiStream) {
  folly::ScopedEventBaseThread evbThread;
  folly::ScopedEventBaseThread evbThread2;

  auto flowable = Flowable<>::justN({12, 34, 56, 98});
  auto stream = toStream(std::move(flowable), evbThread.getEventBase());
  SemiStream<int> stream2 = std::move(stream).map([&](int x) {
    EXPECT_TRUE(evbThread.getEventBase()->inRunningEventBaseThread());
    return x * 2;
  });
  auto streamString = std::move(stream2).map([&](int x) {
    EXPECT_TRUE(evbThread2.getEventBase()->inRunningEventBaseThread());
    return folly::to<std::string>(x);
  });
  auto flowableString =
      toFlowable(std::move(streamString).via(evbThread2.getEventBase()));

  EXPECT_EQ(
      run(flowableString),
      std::vector<std::string>({"24", "68", "112", "196"}));
}

TEST(YarplStreamImplTest, EncodeDecode) {
  folly::ScopedEventBaseThread evbThread;

  Message mIn;
  mIn.set_message("Test Message");
  mIn.set_timestamp(2015);

  auto flowable = Flowable<>::justN({mIn});
  auto inStream = toStream(std::move(flowable), evbThread.getEventBase());

  using PResult =
      ThriftPresult<true, FieldData<0, protocol::T_STRUCT, Message*>>;

  // Encode in the thread of the flowable, namely at the user's thread
  auto encodedStream =
      detail::ap::encode_stream<CompactProtocolWriter, PResult>(
          std::move(inStream),
          [](PResult&, folly::exception_wrapper&) { return false; })
          .map([](folly::IOBufQueue&& in) mutable { return in.move(); });

  // No event base is involved, as this is a defered decoding
  auto decodedStream =
      detail::ap::decode_stream<CompactProtocolReader, PResult, Message>(
          SemiStream<std::unique_ptr<folly::IOBuf>>(std::move(encodedStream)));

  // Actual decode operation happens at the given user thread
  auto flowableOut =
      toFlowable(std::move(decodedStream).via(evbThread.getEventBase()));

  auto subscriber = std::make_shared<TestSubscriber<Message>>(1);
  flowableOut->subscribe(subscriber);

  subscriber->awaitTerminalEvent(std::chrono::seconds(1));

  Message mOut = std::move(subscriber->values()[0]);
  ASSERT_STREQ(mIn.get_message().c_str(), mOut.get_message().c_str());
  ASSERT_EQ(mIn.get_timestamp(), mOut.get_timestamp());
}

template <class P>
class TakeFirstTest;

template <>
class TakeFirstTest<rsocket::Payload> : public testing::Test {
 public:
  static std::shared_ptr<detail::TakeFirst> makeTakeFirst(
      folly::Function<void(
          rsocket::Payload&& first,
          std::shared_ptr<yarpl::flowable::Flowable<
              std::unique_ptr<folly::IOBuf>>> tail)> onNormalFirstResponse,
      folly::Function<void(folly::exception_wrapper ew)> onErrorFirstResponse,
      folly::Function<void()> onStreamTerminated) {
    return std::make_shared<detail::TakeFirst>(
        nullptr,
        [onNormalFirstResponse =
             std::move(onNormalFirstResponse)](auto&& pair) mutable {
          onNormalFirstResponse(std::move(pair.first), std::move(pair.second));
        },
        std::move(onErrorFirstResponse),
        std::move(onStreamTerminated));
  }
};

template <>
class TakeFirstTest<rocket::Payload> : public testing::Test {
 public:
  class RocketTestTakeFirst : public RocketClientChannel::TakeFirst {
   public:
    RocketTestTakeFirst(
        folly::Function<void(
            rocket::Payload&& first,
            std::shared_ptr<yarpl::flowable::Flowable<
                std::unique_ptr<folly::IOBuf>>> tail)> onNormalFirstResponse,
        folly::Function<void(folly::exception_wrapper ew)> onErrorFirstResponse,
        folly::Function<void()> onStreamTerminated)
        : RocketClientChannel::TakeFirst(
              dummyEvb_,
              nullptr /* callback */,
              std::chrono::milliseconds::zero()),
          onNormalFirstResponse_(std::move(onNormalFirstResponse)),
          onErrorFirstResponse_(std::move(onErrorFirstResponse)),
          onStreamTerminated_(std::move(onStreamTerminated)) {}

    ~RocketTestTakeFirst() override = default;

   private:
    folly::Function<void(
        rocket::Payload&& first,
        std::shared_ptr<
            yarpl::flowable::Flowable<std::unique_ptr<folly::IOBuf>>> tail)>
        onNormalFirstResponse_;
    folly::Function<void(folly::exception_wrapper ew)> onErrorFirstResponse_;
    folly::Function<void()> onStreamTerminated_;

    void onNormalFirstResponse(
        rocket::Payload&& first,
        std::shared_ptr<
            yarpl::flowable::Flowable<std::unique_ptr<folly::IOBuf>>> tail)
        final {
      onNormalFirstResponse_(std::move(first), std::move(tail));
    }

    void onErrorFirstResponse(folly::exception_wrapper ew) final {
      onErrorFirstResponse_(std::move(ew));
    }

    void onStreamTerminated() final {
      onStreamTerminated_();
    }
  };

  static std::shared_ptr<RocketTestTakeFirst> makeTakeFirst(
      folly::Function<void(
          rocket::Payload&& first,
          std::shared_ptr<yarpl::flowable::Flowable<
              std::unique_ptr<folly::IOBuf>>> tail)> onNormalFirstResponse,
      folly::Function<void(folly::exception_wrapper ew)> onErrorFirstResponse,
      folly::Function<void()> onStreamTerminated) {
    return std::make_shared<RocketTestTakeFirst>(
        std::move(onNormalFirstResponse),
        std::move(onErrorFirstResponse),
        std::move(onStreamTerminated));
  }

 private:
  static folly::EventBase dummyEvb_;
};

folly::EventBase TakeFirstTest<rocket::Payload>::dummyEvb_;

using PayloadTypes = testing::Types<
    rsocket::Payload /* to test RSocketClientChannel's TakeFirst */,
    rocket::Payload /* to test RocketClientChannel's TakeFirst */>;

TYPED_TEST_CASE(TakeFirstTest, PayloadTypes);

TYPED_TEST(TakeFirstTest, TakeFirstNormal) {
  using PayloadT = TypeParam;

  auto a = Flowable<PayloadT>::justOnce(makePayload<PayloadT>("Hello"));
  auto b = Flowable<PayloadT>::justOnce(makePayload<PayloadT>("World"));
  auto combined = a->concatWith(b);

  folly::Baton<> baton;
  std::shared_ptr<Flowable<std::string>> flowable;
  bool completed = false;
  auto takeFirst = this->makeTakeFirst(
      [&baton, &flowable](auto&& first, auto tail) {
        EXPECT_STREQ("Hello", dataToString(std::move(first)).c_str());
        flowable = tail->map(
            [](auto iobuf) { return iobuf->moveToFbString().toStdString(); });
        baton.post();
      },
      [](auto ex) { FAIL() << "no error was expected: " << ex.what(); },
      [&completed]() { completed = true; });

  combined->subscribe(takeFirst);

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));
  auto values = run(flowable);
  EXPECT_EQ(1, values.size());
  EXPECT_STREQ("World", values[0].c_str());
  EXPECT_TRUE(completed);
}

TYPED_TEST(TakeFirstTest, TakeFirstDontSubscribe) {
  using PayloadT = TypeParam;

  auto a = Flowable<PayloadT>::justOnce(makePayload<PayloadT>("Hello"));
  auto b = Flowable<PayloadT>::justOnce(makePayload<PayloadT>("World"));
  auto combined = a->concatWith(b);

  folly::Baton<> baton;
  bool completed = false;
  auto takeFirst = this->makeTakeFirst(
      [&baton](auto&& first, auto /* tail */) {
        EXPECT_STREQ("Hello", dataToString(std::move(first)).c_str());
        // Do not subscribe to the `result.second`
        baton.post();
      },
      [](auto ex) { FAIL() << "no error was expected: " << ex.what(); },
      [&completed]() { completed = true; });

  combined->subscribe(std::move(takeFirst));

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));
  EXPECT_TRUE(completed);
}

TYPED_TEST(TakeFirstTest, TakeFirstNoResponse) {
  using PayloadT = TypeParam;

  folly::ScopedEventBaseThread evbThread;
  folly::Baton<> baton;
  bool timedOut = false;
  auto takeFirst = this->makeTakeFirst(
      [&baton](auto&& /* first */, auto /* tail */) { baton.post(); },
      [&timedOut, &baton](folly::exception_wrapper) {
        timedOut = true;
        baton.post();
      },
      []() { FAIL() << "onTerminal should not be called"; });

  Flowable<PayloadT>::never()->subscribe(takeFirst);

  ASSERT_FALSE(baton.timed_wait(std::chrono::seconds(1)));

  // Timed out, so cancel the TakeFirst
  baton.reset();
  takeFirst->cancel();

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));
  EXPECT_TRUE(timedOut);
}

TYPED_TEST(TakeFirstTest, TakeFirstErrorResponse) {
  using PayloadT = TypeParam;

  folly::ScopedEventBaseThread evbThread;
  folly::Baton<> baton;
  auto takeFirst = this->makeTakeFirst(
      [](auto&& /* first */, auto /* tail */) { ASSERT_TRUE(false); },
      [&baton](folly::exception_wrapper ew) {
        ASSERT_STREQ(ew.what().c_str(), "std::runtime_error: error");
        baton.post();
      },
      []() { FAIL() << "onTerminal should not be called"; });

  Flowable<PayloadT>::error(std::runtime_error("error"))->subscribe(takeFirst);

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));
}

TYPED_TEST(TakeFirstTest, TakeFirstStreamError) {
  using PayloadT = TypeParam;

  folly::ScopedEventBaseThread evbThread;
  folly::Baton<> baton;

  auto first = Flowable<PayloadT>::justOnce(makePayload<PayloadT>("Hello"));
  auto error = Flowable<PayloadT>::error(std::runtime_error("error"));
  auto combined = first->concatWith(error);

  std::shared_ptr<Flowable<int>> flowable;
  bool completed = false;
  auto takeFirst = this->makeTakeFirst(
      [&baton, &flowable](auto&& /* first */, auto tail) {
        flowable = tail->map([](auto) { return 1; });
        baton.post();
      },
      [](auto ex) { FAIL() << "no error was expected: " << ex.what(); },
      [&completed]() { completed = true; });

  combined->subscribe(takeFirst);

  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));

  auto subscriber = std::make_shared<TestSubscriber<int32_t>>(1);
  flowable->subscribe(subscriber);

  EXPECT_TRUE(subscriber->isError());
  EXPECT_EQ(subscriber->getErrorMsg(), "error");
  EXPECT_TRUE(completed);
}

TYPED_TEST(TakeFirstTest, TakeFirstMultiSubscribeInner) {
  using PayloadT = TypeParam;

  folly::ScopedEventBaseThread evbThread;

  auto a = Flowable<PayloadT>::fromGenerator(
               []() { return makePayload<PayloadT>("Hello"); })
               ->take(1);
  auto b = Flowable<PayloadT>::fromGenerator(
               []() { return makePayload<PayloadT>("World"); })
               ->take(1);
  auto combined = a->concatWith(b);

  // First subscribe
  folly::Baton<> baton;
  std::shared_ptr<Flowable<std::unique_ptr<folly::IOBuf>>> flowable;
  bool completed = false;
  auto takeFirst = this->makeTakeFirst(
      [&baton, &flowable](auto&& first, auto tail) {
        EXPECT_STREQ("Hello", dataToString(std::move(first)).c_str());
        flowable = std::move(tail);
        baton.post();
      },
      [](auto ex) { FAIL() << "no error was expected: " << ex.what(); },
      [&completed]() { completed = true; });

  combined->subscribe(takeFirst);
  ASSERT_TRUE(baton.timed_wait(std::chrono::seconds(1)));

  auto flowable1 = flowable->map(
      [](auto iobuf) { return iobuf->moveToFbString().toStdString(); });
  auto values = run(flowable1);
  EXPECT_EQ(1, values.size());
  EXPECT_STREQ("World", values[0].c_str());

  // Second subscribe
  auto flowable2 = flowable->map([](auto) { return 1; });
  auto subscriber = std::make_shared<TestSubscriber<int32_t>>(10);
  EXPECT_THROW(flowable2->subscribe(subscriber), std::logic_error);
  EXPECT_TRUE(completed);
}

} // namespace thrift
} // namespace apache
