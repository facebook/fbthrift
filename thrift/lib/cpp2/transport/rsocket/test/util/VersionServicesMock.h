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

#pragma once

#include <folly/portability/GMock.h>

#include <folly/io/async/ScopedEventBaseThread.h>
#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>
#include <thrift/lib/cpp2/transport/rsocket/test/util/gen-cpp2/NewVersion.h>
#include <thrift/lib/cpp2/transport/rsocket/test/util/gen-cpp2/OldVersion.h>

namespace testutil {
namespace testservice {

class OldServiceMock : public OldVersionSvIf {
 public:
  OldServiceMock() {}

  int32_t AddOne(int32_t i) override {
    return i + 1;
  }

  void DeletedMethod() {}

  apache::thrift::Stream<Message> DeletedStreamMethod() {
    return createStreamGenerator(
        []() -> folly::Optional<Message> { return folly::none; });
  }

  apache::thrift::ResponseAndStream<Message, Message>
  DeletedResponseAndStreamMethod() {
    return {{}, createStreamGenerator([]() -> folly::Optional<Message> {
              return folly::none;
            })};
  }

  apache::thrift::Stream<int32_t> Range(int32_t from, int32_t length) override {
    return createStreamGenerator(
        [first = from,
         last = from + length]() mutable -> folly::Optional<int32_t> {
          if (first >= last) {
            return folly::none;
          }
          return first++;
        });
  }

  apache::thrift::ResponseAndStream<int32_t, int32_t>
  RangeAndAddOne(int32_t from, int32_t length, int32_t number) override {
    return {number + 1, Range(from, length)};
  }

  apache::thrift::Stream<Message> StreamToRequestResponse() override {
    return createStreamGenerator(
        []() -> folly::Optional<Message> { return folly::none; });
  }

  apache::thrift::ResponseAndStream<Message, Message>
  ResponseandStreamToRequestResponse() override {
    Message response;
    response.message = "Message";
    response.__isset.message = true;
    return {std::move(response),
            createStreamGenerator(
                []() -> folly::Optional<Message> { return folly::none; })};
  }

  void RequestResponseToStream(Message& response) override {
    response.message = "Message";
    response.__isset.message = true;
  }

  void RequestResponseToResponseandStream(Message& response) override {
    response.message = "Message";
    response.__isset.message = true;
  }

 protected:
  folly::ScopedEventBaseThread executor_;
};

class NewServiceMock : public NewVersionSvIf {
 public:
  NewServiceMock() {}

  int32_t AddOne(int32_t i) override {
    return i + 1;
  }

  apache::thrift::Stream<int32_t> Range(int32_t from, int32_t length) override {
    return createStreamGenerator(
        [first = from,
         last = from + length]() mutable -> folly::Optional<int32_t> {
          if (first >= last) {
            return folly::none;
          }
          return first++;
        });
  }

  apache::thrift::ResponseAndStream<int32_t, int32_t>
  RangeAndAddOne(int32_t from, int32_t length, int32_t number) override {
    return {number + 1, Range(from, length)};
  }

 protected:
  void StreamToRequestResponse() {
    LOG(DFATAL) << "StreamToRequestResponse should not be executed";
  }

  void ResponseandStreamToRequestResponse() {
    LOG(DFATAL) << "ResponseandStreamToRequestResponse should not be executed";
  }

  apache::thrift::Stream<Message> RequestResponseToStream() override {
    LOG(DFATAL) << "RequestResponseToStream should not be executed";

    return createStreamGenerator(
        []() -> folly::Optional<Message> { return folly::none; });
  }

  apache::thrift::ResponseAndStream<Message, Message>
  RequestResponseToResponseandStream() override {
    LOG(DFATAL) << "RequestResponseToStream should not be executed";

    return {Message{}, createStreamGenerator([]() -> folly::Optional<Message> {
              return folly::none;
            })};
  }

 protected:
  folly::ScopedEventBaseThread executor_;
};
} // namespace testservice
} // namespace testutil
