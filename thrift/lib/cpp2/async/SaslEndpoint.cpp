/*
 * Copyright 2014 Facebook, Inc.
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

#include <thrift/lib/cpp2/async/SaslEndpoint.h>

#include <memory>

#include <folly/io/Cursor.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>

using folly::IOBuf;
using folly::IOBufQueue;

using namespace std;

namespace apache {
namespace thrift {

std::unique_ptr<IOBuf> SaslEndpoint::wrap(std::unique_ptr<IOBuf>&& buf) {
  // Note that this forced copy is necessary because we need to make sure
  // the original buffer is not modified. Code higher-up in the stack
  // assumes the buffer contents don't change. (The 'buf' wrapper doesn't
  // actually own the underlying data).
  if (buf->isChained()) {
    buf->coalesce();
  } else {
    buf = IOBuf::copyBuffer(buf->data(), buf->length());
  }
  std::unique_ptr<IOBuf> wrapped = encrypt(std::move(buf));

  uint32_t wraplen = wrapped->computeChainDataLength();
  std::unique_ptr<IOBuf> framing = IOBuf::create(sizeof(wraplen));

  framing->append(sizeof(wraplen));
  framing->appendChain(std::move(wrapped));

  folly::io::RWPrivateCursor c(framing.get());
  c.writeBE<uint32_t>(wraplen);
  return framing;
}

std::unique_ptr<IOBuf> SaslEndpoint::unwrap(IOBufQueue* q, size_t* remaining) {
  folly::io::Cursor c(q->front());
  size_t chainSize = q->chainLength();
  uint32_t wraplen = 0;

  if (chainSize < sizeof(wraplen)) {
    *remaining = sizeof(wraplen) - chainSize;
    return nullptr;
  }

  wraplen = c.readBE<uint32_t>();

  if (chainSize < sizeof(wraplen) + wraplen) {
    *remaining = sizeof(wraplen) + wraplen - chainSize;
    return nullptr;
  }

  // unwrap the data
  q->trimStart(sizeof(wraplen));
  std::unique_ptr<IOBuf> input = q->split(wraplen);
  std::unique_ptr<IOBuf> output = decrypt(std::move(input));
  *remaining = 0;
  return output;
}

} // namespace thrift
} // namespace apache
