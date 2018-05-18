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

#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>

#include <yarpl/Flowable.h>

namespace apache {
namespace thrift {

template <typename Generator, typename Element>
Stream<Element> StreamGenerator::create(
    folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
    Generator&& generator) {
  auto flowable = yarpl::flowable::Flowable<Element>::create(
      [generator = std::forward<Generator>(generator)](
          yarpl::flowable::Subscriber<Element>& subscriber,
          int64_t requested) mutable {
        try {
          while (requested-- > 0) {
            if (auto value = generator()) {
              subscriber.onNext(std::move(*value));
            } else {
              subscriber.onComplete();
              break;
            }
          }
        } catch (const std::exception& ex) {
          subscriber.onError(
              folly::exception_wrapper(std::current_exception(), ex));
        } catch (...) {
          subscriber.onError(
              folly::exception_wrapper(std::current_exception()));
        }
      });
  return toStream(std::move(flowable), std::move(executor));
}
} // namespace thrift
} // namespace apache
