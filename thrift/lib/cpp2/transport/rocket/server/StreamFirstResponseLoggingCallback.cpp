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

#include <thrift/lib/cpp2/transport/rocket/server/StreamFirstResponseLoggingCallback.h>

#include <folly/ScopeGuard.h>

namespace apache::thrift::rocket {

bool StreamFirstResponseLoggingCallback::onFirstResponse(
    FirstResponsePayload&& payload,
    folly::EventBase* evb,
    StreamServerCallback* serverCallback) {
  SCOPE_EXIT {
    delete this;
  };
  CHECK(serverCallback != nullptr)
      << "Stream first response is missing its server callback";

  // resetClientCallback() is documented as non-terminating, so it is safe here,
  // before the first response has been delivered. It must run before the
  // forward below: onFirstResponse() can terminate the stream inline, and the
  // server callback must already be pointing at `inner_` by then.
  serverCallback->resetClientCallback(inner_);

  inner_.setFirstResponseSendCallback(std::move(sendCallback_));

  // Forwarded verbatim. `false` means a terminating method was called inline;
  // reporting `true` instead would let the caller keep using a stream that has
  // already been torn down.
  return inner_.onFirstResponse(std::move(payload), evb, serverCallback);
}

void StreamFirstResponseLoggingCallback::onFirstResponseError(
    folly::exception_wrapper ew) {
  SCOPE_EXIT {
    delete this;
  };

  inner_.setFirstResponseSendCallback(std::move(sendCallback_));
  inner_.onFirstResponseError(std::move(ew));
}

} // namespace apache::thrift::rocket
