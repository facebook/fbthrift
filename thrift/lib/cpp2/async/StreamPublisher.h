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

#pragma once

#include <thrift/lib/cpp2/async/Stream.h>

namespace apache {
namespace thrift {
namespace detail {
template <typename T>
class StreamPublisherState;
}

template <typename T>
class StreamPublisher {
 public:
  static constexpr size_t kNoLimit = 0;

  ~StreamPublisher();

  StreamPublisher(const StreamPublisher&) = delete;
  StreamPublisher(StreamPublisher&&) = default;

  static std::pair<Stream<T>, StreamPublisher<T>> create(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
      folly::Function<void()> onCompleteOrCanceled,
      size_t bufferSizeLimit = kNoLimit);

  void next(const T&) const;
  void next(T&&) const;
  // Completes the stream. This functions blocks until onCompleteOrCanceled
  // callback is executed.
  void complete(folly::exception_wrapper) &&;
  void complete() &&;

 private:
  explicit StreamPublisher(
      std::shared_ptr<detail::StreamPublisherState<T>> sharedState);

  std::shared_ptr<detail::StreamPublisherState<T>> sharedState_;
};

} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/async/StreamPublisher-inl.h>
