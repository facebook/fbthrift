/*
 * Copyright 2017-present Facebook, Inc.
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

#include <folly/io/async/EventBaseManager.h>
#include <rsocket/Payload.h>
#include <rsocket/rsocket.h>
#include <yarpl/Observable.h>
#include <yarpl/Observable/ObservableOperator.h>
#include <yarpl/Single.h>
#include <yarpl/flowable/Flowables.h>

#include <thrift/lib/cpp2/transport/core/ThriftProcessor.h>

namespace apache {
namespace thrift {

class RSResponder : public rsocket::RSocketResponder {
 public:
  RSResponder(ThriftProcessor* processor, folly::EventBase* evb);

  virtual ~RSResponder() = default;

  yarpl::Reference<yarpl::single::Single<rsocket::Payload>>
  handleRequestResponse(rsocket::Payload request, rsocket::StreamId streamId)
      override;

 protected:
  ThriftProcessor* processor_;
  folly::EventBase* evb_;
};
}
}
