/*
 * Copyright 2015 Facebook, Inc.
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
#include <thrift/lib/cpp2/async/ProtectionHandler.h>
#include <thrift/lib/cpp/transport/TTransportException.h>

namespace apache { namespace thrift {

void ProtectionHandler::read(Context* ctx, folly::IOBufQueue& q) {
  ctx_ = ctx;

  if (&inputQueue_ != &q) {
    // If not the same queue
    inputQueue_.append(q);
  }

  auto e = folly::try_and_catch<std::exception>([&]() {
      while (true) {
        if (protectionState_ == ProtectionState::INVALID) {
          throw transport::TTransportException("protection state is invalid");
        }

        if (protectionState_ == ProtectionState::INPROGRESS) {
          // security is still doing stuff, let's return blank.
          return;
        }

        if (protectionState_ != ProtectionState::VALID) {
          // not an encrypted message, so pass-through
          ctx->fireRead(inputQueue_);
          return;
        }

        assert(saslEndpoint_ != nullptr);
        size_t remaining = 0;

        if (!inputQueue_.front() || inputQueue_.front()->empty()) {
          return;
        }

        // decrypt
        std::unique_ptr<folly::IOBuf> unwrapped = saslEndpoint_->unwrap(&inputQueue_, &remaining);
        assert(bool(unwrapped) ^ (remaining > 0));   // 1 and only 1 should be true
        if (unwrapped) {
          queue_.append(std::move(unwrapped));

          ctx->fireRead(queue_);
        } else {
          return;
        }
      }
    });
  if (e) {
    VLOG(5) << "Exception in ProtectionHandler::read(): " << e.what();
    ctx->fireReadException(std::move(e));
  }
}

folly::Future<void> ProtectionHandler::write(
  Context* ctx,
  std::unique_ptr<folly::IOBuf> buf) {
  if (protectionState_ == ProtectionState::VALID) {
    assert(saslEndpoint_);
    buf = saslEndpoint_->wrap(std::move(buf));
  }
  return ctx->fireWrite(std::move(buf));
}


void ProtectionHandler::protectionStateChanged() {
  // We only want to do this callback in the case where we're switching
  // to a valid protection state.
  if (ctx_ && !inputQueue_.empty() &&
      protectionState_ == ProtectionState::VALID) {
    read(ctx_, inputQueue_);
  }
}

}}
