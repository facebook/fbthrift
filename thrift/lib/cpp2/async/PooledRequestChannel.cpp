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
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>

namespace apache {
namespace thrift {

uint16_t PooledRequestChannel::getProtocolId() {
  folly::call_once(protocolIdInitFlag_, [&] {
    auto evb = executor_->getEventBase();
    evb->runInEventBaseThreadAndWait(
        [&] { protocolId_ = impl(*evb).getProtocolId(); });
  });

  return protocolId_;
}

uint32_t PooledRequestChannel::sendRequest(
    RpcOptions& options,
    std::unique_ptr<RequestCallback> cob,
    std::unique_ptr<ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<transport::THeader> header) {
  auto evb = executor_->getEventBase();

  evb->runInEventBaseThread([this,
                             evb,
                             keepAlive = evb->getKeepAliveToken(),
                             options = std::move(options),
                             cob = std::move(cob),
                             ctx = std::move(ctx),
                             buf = std::move(buf),
                             header = std::move(header)]() mutable {
    impl(*evb).sendRequest(
        options,
        std::move(cob),
        std::move(ctx),
        std::move(buf),
        std::move(header));
  });
  return 0;
}

PooledRequestChannel::Impl& PooledRequestChannel::impl(folly::EventBase& evb) {
  DCHECK(evb.inRunningEventBaseThread());

  auto creator = [&evb, &implCreator = implCreator_] {
    return implCreator(evb);
  };
  return impl_.getOrCreateFn(evb, creator);
}
} // namespace thrift
} // namespace apache
