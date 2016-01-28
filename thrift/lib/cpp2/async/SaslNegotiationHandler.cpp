/*
 * Copyright 2016 Facebook, Inc.
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
#include <thrift/lib/cpp2/async/SaslNegotiationHandler.h>

namespace apache { namespace thrift {

using ProtectionState = apache::thrift::ProtectionHandler::ProtectionState;

void SaslNegotiationHandler::read(Context* ctx, BufAndHeader bufAndHeader) {
  if (protectionHandler_->getProtectionState() == ProtectionState::NONE ||
      protectionHandler_->getProtectionState() == ProtectionState::VALID) {
    // This handler should be removed from the pipeline after sasl
    // negotiation is completed. If it is still installed, it should
    // do nothing.
    ctx->fireRead(std::move(bufAndHeader));
  } else {
    if (handleSecurityMessage(std::move(bufAndHeader.first),
                              std::move(bufAndHeader.second))) {
      ctx->fireRead(std::move(bufAndHeader));
    }
  }
}

}} // namespace
