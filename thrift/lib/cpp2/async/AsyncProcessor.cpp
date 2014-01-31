/*
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
#include "thrift/lib/cpp2/async/AsyncProcessor.h"

namespace apache { namespace thrift {

__thread Cpp2RequestContext* ServerInterface::reqCtx_;
__thread async::TEventBase* ServerInterface::eb_;

template <>
void HandlerCallback<StreamManager*>::sendReply(folly::IOBufQueue queue,
                                                StreamManager* const& r) {
  std::unique_ptr<StreamManager> stream(r);
  if (getEventBase()->isInEventBaseThread()) {
    req_->sendReplyWithStreams(queue.move(), std::move(stream));
  } else {
    auto req_mw = folly::makeMoveWrapper(std::move(req_));
    auto queue_mw = folly::makeMoveWrapper(std::move(queue));
    auto stream_mw = folly::makeMoveWrapper(std::move(stream));
    getEventBase()->runInEventBaseThread([=]() mutable {
      std::unique_ptr<StreamManager> s(stream_mw->release());
      (*req_mw)->sendReplyWithStreams(queue_mw->move(), std::move(s));
    });
  }
}

}} // apache::thrift
