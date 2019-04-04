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

#include <memory>

#include <thrift/lib/cpp2/transport/rocket/Types.h>

#include <folly/ExceptionWrapper.h>

#include <yarpl/flowable/Subscriber.h>

namespace yarpl {
namespace flowable {
class Subscription;
} // namespace flowable
} // namespace yarpl

namespace apache {
namespace thrift {
namespace rocket {

class RocketServerFrameContext;
class RocketServerConnection;

class RocketServerStreamSubscriber
    : public yarpl::flowable::Subscriber<Payload> {
 public:
  RocketServerStreamSubscriber(
      RocketServerFrameContext&& context,
      uint32_t initialRequestN);

  ~RocketServerStreamSubscriber() final;

  // Subscriber implementation
  void onSubscribe(
      std::shared_ptr<yarpl::flowable::Subscription> subscription) final;
  void onNext(Payload payload) final;
  void onComplete() final;
  void onError(folly::exception_wrapper) final;

  void request(uint32_t n);
  void cancel();

 private:
  std::unique_ptr<RocketServerFrameContext> context_;
  std::shared_ptr<yarpl::flowable::Subscription> subscription_;
  uint32_t initialRequestN_{0};
  bool canceledBeforeSubscribed_{false};
};

} // namespace rocket
} // namespace thrift
} // namespace apache
