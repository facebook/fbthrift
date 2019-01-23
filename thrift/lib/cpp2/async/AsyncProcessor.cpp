/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <thrift/lib/cpp2/async/AsyncProcessor.h>

namespace apache {
namespace thrift {

constexpr std::chrono::seconds ServerInterface::BlockingThreadManager::kTimeout;
thread_local RequestParams ServerInterface::requestParams_;

void HandlerCallbackBase::sendReply(
    folly::IOBufQueue queue,
    apache::thrift::Stream<folly::IOBufQueue>&& stream) {
  transform(queue);
  auto stream_ =
      std::move(stream).map([](auto&& value) mutable { return value.move(); });
  if (getEventBase()->isInEventBaseThread()) {
    req_->sendStreamReply({queue.move(), std::move(stream_)});
  } else {
    getEventBase()->runInEventBaseThread(
        [req = std::move(req_),
         queue = std::move(queue),
         stream = std::move(stream_)]() mutable {
          req->sendStreamReply({queue.move(), std::move(stream)});
        });
  }
}

} // namespace thrift
} // namespace apache
