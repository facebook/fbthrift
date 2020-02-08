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

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/async/StreamGenerator.h>

#include <folly/Conv.h>
#include <folly/io/async/ScopedEventBaseThread.h>

namespace apache {
namespace thrift {

TEST(StreamGeneratorFutureTest, Basic) {
  folly::ScopedEventBaseThread th;
  int length = 5;
  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::Optional<int> {
        if (i < length) {
          return i++;
        }

        return folly::none;
      });

  int expected_i = 0;
  std::move(stream)
      .subscribe(
          [&](int i) { EXPECT_EQ(expected_i++, i); },
          [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
          [&] { EXPECT_EQ(expected_i, length); },
          2)
      .futureJoin()
      .get();
}

TEST(StreamGeneratorFutureTest, FutureBasic) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 5;
  std::vector<folly::Promise<folly::Optional<int>>> vp(length);

  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::SemiFuture<folly::Optional<int>> {
        if (i < length) {
          return vp[i++].getSemiFuture();
        }

        return makeSemiFuture(folly::Optional<int>(folly::none));
      });

  // intentionally let promised fullfilled in reverse order, but the result
  // should come back to stream in order
  for (int i = length - 1; i >= 0; i--) {
    pth.add([&vp, i]() { vp[i].setValue(i); });
  }

  int expected_i = 0;
  std::move(stream)
      .subscribe(
          [&](int i) { EXPECT_EQ(expected_i++, i); },
          [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
          [&] { EXPECT_EQ(expected_i, length); },
          2)
      .futureJoin()
      .get();
}

TEST(StreamGeneratorFutureTest, FutureLongStreamSmallFanout) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 100;
  std::vector<folly::Promise<folly::Optional<int>>> vp(length);

  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::SemiFuture<folly::Optional<int>> {
        if (i < length) {
          return vp[i++].getSemiFuture();
        }

        return makeSemiFuture(folly::Optional<int>(folly::none));
      },
      10);

  // future fullfilled with intervals
  for (int i = 0; i < length; i++) {
    pth.add([&, i]() {
      pth.getEventBase()->schedule(
          [&, i]() mutable { vp[i].setValue(i); },
          std::chrono::milliseconds(i));
    });
  }

  int expected_i = 0;
  std::move(stream)
      .subscribe(
          [&](int i) { EXPECT_EQ(expected_i++, i); },
          [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
          [&] { EXPECT_EQ(expected_i, length); },
          50)
      .futureJoin()
      .get();
  EXPECT_EQ(expected_i, length);
}

TEST(StreamGeneratorFutureTest, FutureThrow) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 10;
  std::vector<folly::Promise<folly::Optional<int>>> vp(length);

  int i = 0;
  std::string errorMsg = "error message";
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::SemiFuture<folly::Optional<int>> {
        if (i < length) {
          return vp[i++].getSemiFuture();
        }
        throw std::runtime_error(errorMsg);
      });

  for (int i = 0; i < length; i++) {
    pth.add([&vp, i]() { vp[i].setValue(i); });
  }

  int expected_i = 0;
  bool onError = false;
  std::move(stream)
      .subscribe(
          [&](int i) { EXPECT_EQ(expected_i++, i); },
          [&](folly::exception_wrapper ew) {
            onError = true;
            EXPECT_EQ(errorMsg, ew.get_exception()->what());
          },
          [] {},
          3)
      .futureJoin()
      .get();

  EXPECT_TRUE(onError);
}

TEST(StreamGeneratorFutureTest, FutureNoFlowControl) {
  folly::ScopedEventBaseThread th;
  folly::ScopedEventBaseThread pth;
  int length = 5;
  std::vector<folly::Promise<folly::Optional<int>>> vp(length);

  int i = 0;
  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [&]() -> folly::SemiFuture<folly::Optional<int>> {
        if (i < length) {
          return vp[i++].getSemiFuture();
        }

        return makeSemiFuture(folly::Optional<int>(folly::none));
      });

  for (int i = 0; i < length; i++) {
    pth.add([&vp, i]() { vp[i].setValue(i); });
  }

  int expected_i = 0;
  std::move(stream)
      .subscribe(
          [&](int i) { EXPECT_EQ(expected_i++, i); },
          [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
          [&] { EXPECT_EQ(expected_i, length); },
          Stream<int>::kNoFlowControl)
      .futureJoin()
      .get();
}

// rely on client's cancel signal to stop the stream
TEST(StreamGeneratorFutureTest, FutureNoFlowControlNoEnd) {
  folly::ScopedEventBaseThread th;

  Stream<int> stream = StreamGenerator::create(
      folly::getKeepAliveToken(th.getEventBase()),
      [i = 0]() mutable -> folly::SemiFuture<folly::Optional<int>> {
        return makeSemiFuture(folly::Optional<int>(i++));
      },
      10);

  int expected_i = 0;
  int target = 101;
  folly::Baton<> b;
  auto subscription = std::move(stream).subscribe(
      [&](int i) {
        if (expected_i == target) {
          b.post();
        }
        EXPECT_EQ(expected_i++, i);
      },
      [](folly::exception_wrapper) { CHECK(false) << "on Error"; },
      [&] {},
      Stream<int>::kNoFlowControl);
  b.wait();
  subscription.cancel();
  std::move(subscription).join();
  EXPECT_GE(expected_i, target);
}

} // namespace thrift
} // namespace apache
