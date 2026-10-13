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

#include <thrift/lib/cpp2/fast_thrift/thrift/stream/common/Messages.h>

#include <cstdint>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

#include <folly/io/IOBuf.h>

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/TypeErasedBox.h>

namespace apache::thrift::fast_thrift::thrift::stream {

namespace {

using channel_pipeline::erase_and_box;
using channel_pipeline::TypeErasedBox;

// The two sets partition the five frame kinds with no overlap: Control carries
// exactly the consumer -> producer frames, Data exactly the producer ->
// consumer frames. Asserting the arm membership documents the partition and
// guards against a frame kind drifting into the wrong set.
TEST(MessagesTest, ControlSetHoldsDemandAndConsumerTeardown) {
  ControlMessage requestN{.payload = RequestN{.n = 7}};
  EXPECT_TRUE(requestN.payload.is<RequestN>());
  EXPECT_EQ(requestN.payload.get<RequestN>().n, 7u);

  ControlMessage cancel{.payload = Cancel{}};
  EXPECT_TRUE(cancel.payload.is<Cancel>());
}

TEST(MessagesTest, DataSetHoldsPayloadsAndProducerTerminals) {
  DataMessage payload{.payload = Payload{.data = folly::IOBuf::create(0)}};
  EXPECT_TRUE(payload.payload.is<Payload>());

  DataMessage complete{.payload = Complete{}};
  EXPECT_TRUE(complete.payload.is<Complete>());

  DataMessage error{
      .payload = Error{
          .ex = folly::make_exception_wrapper<std::runtime_error>("boom")}};
  EXPECT_TRUE(error.payload.is<Error>());
}

// Each set must survive being erased into a pipeline box and pulled back out,
// since that is exactly how a handler receives it on the read/write path.
TEST(MessagesTest, ControlMessageRoundTripsThroughTypeErasedBox) {
  TypeErasedBox box =
      erase_and_box(ControlMessage{.payload = RequestN{.n = 3}});
  ControlMessage taken = box.take<ControlMessage>();
  EXPECT_EQ(taken.payload.get<RequestN>().n, 3u);
}

TEST(MessagesTest, DataMessageRoundTripsThroughTypeErasedBox) {
  TypeErasedBox box = erase_and_box(DataMessage{.payload = Complete{}});
  DataMessage taken = box.take<DataMessage>();
  EXPECT_TRUE(taken.payload.is<Complete>());
}

// Narrowing the 5-way message into two smaller sets must not cost a heap
// allocation on the hot path: both sets have to stay inline in TypeErasedBox.
static_assert(TypeErasedBox::fits_inline<ControlMessage>());
static_assert(TypeErasedBox::fits_inline<DataMessage>());

} // namespace

} // namespace apache::thrift::fast_thrift::thrift::stream
