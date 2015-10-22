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
#include <folly/Logging.h>

namespace apache { namespace thrift {

void ProtectionHandler::read(Context* ctx, folly::IOBufQueue& q) {
  if (&inputQueue_ != &q) {
    // If not the same queue
    inputQueue_.append(q);
  }

  auto e = folly::try_and_catch<std::exception>([&]() {
      while (!closing_) {
        if (protectionState_ == ProtectionState::INVALID) {
          throw transport::TTransportException("protection state is invalid");
        }

        if (protectionState_ == ProtectionState::INPROGRESS) {
          // security is still doing stuff, let's return blank.
          break;
        }

        if (protectionState_ != ProtectionState::VALID) {
          // not an encrypted message, so pass-through
          ctx->fireRead(inputQueue_);
          break;
        }

        assert(saslEndpoint_ != nullptr);
        size_t remaining = 0;

        if (!inputQueue_.front() || inputQueue_.front()->empty()) {
          break;
        }

        std::unique_ptr<folly::IOBuf> unwrapped;
        // If this is the first message after protection state was set to valid,
        // allow the ability to fall back to plaintext.
        if (allowFallback_ && protectionState_ == ProtectionState::VALID) {
          // Make a copy of inputQueue_.
          // If decryption fails, we try to read again without decrypting.
          // The copy is necessary since decryption attempt modifies the queue.
          folly::IOBufQueue inputQueueCopy;
          auto copyBuf = inputQueue_.front()->clone();
          copyBuf->unshare();
          inputQueueCopy.append(std::move(copyBuf));

          // decrypt inputQueue_
          auto decryptEx = folly::try_and_catch<std::exception>([&]() {
            unwrapped = saslEndpoint_->unwrap(&inputQueue_, &remaining);
          });

          if (remaining == 0) {
            // If we got an entire frame, make sure we try the fallback
            // only once. If we only got a partial frame, we have to try falling
            // back again until we get the full first frame.
            allowFallback_ = false;
          }

          if (decryptEx) {
            // If decrypt fails, try reading again without decrypting. This
            // allows a fallback to happen if the timeout happened in the last
            // leg of the handshake.
            inputQueue_ = std::move(inputQueueCopy);
            ctx->fireRead(inputQueue_);
            break;
          }
        } else {
          unwrapped = saslEndpoint_->unwrap(&inputQueue_, &remaining);
        }

        assert(bool(unwrapped) ^ (remaining > 0));   // 1 and only 1 should be true
        if (unwrapped) {
          queue_.append(std::move(unwrapped));
          ctx->fireRead(queue_);
        } else {
          break;
        }
      }
    });
  if (e) {
    FB_LOG_EVERY_MS(ERROR, 1000)
        << "Exception in ProtectionHandler::read(): " << e.what();
    ctx->fireReadException(std::move(e));
  }
  // Give ownership back to the main queue if we're not in the inprogress
  // state
  if (&inputQueue_ != &q &&
      protectionState_ != ProtectionState::INPROGRESS) {
    q.append(inputQueue_);
  }
}

folly::Future<folly::Unit> ProtectionHandler::write(
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
  if (getContext() && !inputQueue_.empty() &&
      protectionState_ == ProtectionState::VALID) {
    read(getContext(), inputQueue_);
  }
}

folly::Future<folly::Unit> ProtectionHandler::close(Context* ctx) {
  closing_ = true;
  return ctx->fireClose();
}

}}
