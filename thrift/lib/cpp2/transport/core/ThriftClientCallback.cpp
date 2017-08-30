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

#include <thrift/lib/cpp2/transport/core/ThriftClientCallback.h>

#include <glog/logging.h>

namespace apache {
namespace thrift {

using std::map;
using std::string;
using folly::EventBase;
using folly::IOBuf;

ThriftClientCallback::ThriftClientCallback(
    EventBase* evb,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<ContextStack> ctx,
    bool isSecurityActive,
    uint16_t protoId)
    : evb_(evb),
      cb_(std::move(cb)),
      ctx_(std::move(ctx)),
      isSecurityActive_(isSecurityActive),
      protoId_(protoId) {}

void ThriftClientCallback::onThriftResponse(
    std::unique_ptr<map<string, string>> /*headers*/,
    std::unique_ptr<IOBuf> payload) noexcept {
  cb_->replyReceived(ClientReceiveState(
      protoId_,
      std::move(payload),
      nullptr,
      std::move(ctx_), // move is ok as this method will be called only once
      /* _isSecurityActive = */ false));
}

void ThriftClientCallback::cancel() noexcept {
  // TBD
}

EventBase* ThriftClientCallback::getEventBase() const {
  return evb_;
}

} // namespace thrift
} // namespace apache
