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

#include <thrift/lib/cpp/SerializedMessage.h>

#include <utility>

#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/IOBufChain.h>

namespace apache::thrift {

size_t SerializedMessage::computeChainDataLength() const noexcept {
  if (buffer != nullptr) {
    return buffer->computeChainDataLength();
  }
  return chainBuffer != nullptr ? chainBuffer->chainLength() : 0;
}

std::unique_ptr<folly::IOBuf> SerializedMessage::cloneBuffer() const {
  if (buffer != nullptr) {
    return buffer->clone();
  }
  if (chainBuffer != nullptr) {
    return chainBuffer->empty() ? std::make_unique<folly::IOBuf>()
                                : std::move(chainBuffer->clone()).moveToIOBuf();
  }
  return nullptr;
}

} // namespace apache::thrift
