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

#include "thrift/lib/cpp2/async/FramingHandler.h"

namespace apache { namespace thrift {

void FramingHandler::read(Context* ctx, folly::IOBufQueue& q) {
  // Remaining for this packet.  Will update the class member
  // variable below for the next call to getReadBuffer
  size_t remaining = 0;

  // Loop as long as there are deframed messages to read.
  // Partial frames are stored inside the handlers between calls

  // On the last iteration, remaining_ is updated to the anticipated remaining
  // frame length (if we're in the middle of a frame) or to readBufferSize_
  // (if we are exactly between frames)
  while (true) {
    DCHECK(protectionHandler_);
    if (protectionHandler_->getProtectionState() ==
        ProtectionHandler::ProtectionState::INPROGRESS) {
      return;
    }
    std::unique_ptr<folly::IOBuf> unframed;
    auto ex = folly::try_and_catch<std::exception>([&]() {

        // got a decrypted message
        std::tie(unframed, remaining) = removeFrame(&q);
      });

    if (ex) {
      VLOG(5) << "Failed to read a message header";
      ctx->fireReadException(std::move(ex));
      ctx->fireClose();
      return;
    }

    if (!unframed) {
      ctx->setReadBufferSettings(
        readBufferSize_, remaining ? remaining : readBufferSize_);
      return;
    } else {
      folly::IOBufQueue q2;
      q2.append(std::move(unframed));
      ctx->fireRead(q2);
    }
  }
}

folly::Future<void> FramingHandler::write(
  Context* ctx,
  std::unique_ptr<folly::IOBuf> buf) {
  return ctx->fireWrite(addFrame(std::move(buf)));
}


}} // namespace
