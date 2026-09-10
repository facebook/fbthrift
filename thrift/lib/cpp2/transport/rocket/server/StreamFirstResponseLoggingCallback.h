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

#pragma once

#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/transport/rocket/server/RocketStreamClientCallback.h>

namespace apache::thrift::rocket {

/**
 * Interposes on a stream's initial response so that the write timings of that
 * response are recorded, then takes itself back out of the stream.
 *
 * `thrift_request_events` samples are emitted by the destructor of the send
 * callback that ThriftRequestCore::createRequestLoggingCallback() builds. The
 * unary path hands that callback to the transport, so the transport sets
 * writeBegin/writeEnd on it and destroys it once the write settles. The stream
 * path had nowhere to hand it — sendStreamThriftResponse() takes no send
 * callback at all — so streaming methods produced no sample whatsoever. This
 * wrapper is that place: it attaches the callback to the initial-response write
 * via RocketStreamClientCallback::setFirstResponseSendCallback().
 *
 * Scope is deliberately the initial response only. The wrapper is destroyed as
 * soon as onFirstResponse()/onFirstResponseError() has been forwarded — the
 * earliest the StreamClientCallback contract allows — so per-item stream
 * traffic goes straight to the wrapped callback with no extra hop.
 *
 * Self-deletion is what makes re-pointing mandatory: whoever invoked us stored
 * `this` as its client callback, so onFirstResponse() calls
 * resetClientCallback() on the server callback before returning. Skipping that
 * would leave a dangling client callback for the rest of the stream.
 */
class StreamFirstResponseLoggingCallback final : public StreamClientCallback {
 public:
  StreamFirstResponseLoggingCallback(
      RocketStreamClientCallback& inner,
      apache::thrift::MessageChannel::SendCallbackPtr sendCallback)
      : inner_(inner), sendCallback_(std::move(sendCallback)) {}

  bool onFirstResponse(
      FirstResponsePayload&& payload,
      folly::EventBase* evb,
      StreamServerCallback* serverCallback) override;

  void onFirstResponseError(folly::exception_wrapper ew) override;

  // The remaining StreamClientCallback methods are unreachable: this object is
  // gone once the first response has been forwarded, and it re-points the
  // server callback at `inner_` on the way out. They forward rather than abort
  // so that a mistake in that reasoning degrades into missing timings instead
  // of a crashed server.
  bool onStreamNext(StreamPayload&& payload) override {
    return inner_.onStreamNext(std::move(payload));
  }

  void onStreamError(folly::exception_wrapper ew) override {
    inner_.onStreamError(std::move(ew));
  }

  void onStreamComplete() override { inner_.onStreamComplete(); }

  bool onStreamHeaders(HeadersPayload&& payload) override {
    return inner_.onStreamHeaders(std::move(payload));
  }

  void resetServerCallback(StreamServerCallback& serverCallback) override {
    inner_.resetServerCallback(serverCallback);
  }

 private:
  RocketStreamClientCallback& inner_;
  apache::thrift::MessageChannel::SendCallbackPtr sendCallback_;
};

} // namespace apache::thrift::rocket
