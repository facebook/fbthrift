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

#include <thrift/lib/cpp2/transport/rocket/framing/Serializer.h>

#include <thrift/lib/cpp2/transport/rocket/Types.h>

namespace apache {
namespace thrift {
namespace rocket {

size_t Serializer::writePayload(Payload&& p) {
  size_t nwritten = 0;
  if (p.hasNonemptyMetadata()) {
    const size_t metadataSize = p.metadata()->computeChainDataLength();
    nwritten += writeFrameOrMetadataSize(metadataSize);
    nwritten += write(std::move(p.metadata()));
  }
  if (!p.data()->empty()) {
    nwritten += write(std::move(p).data());
  }
  return nwritten;
}

} // namespace rocket
} // namespace thrift
} // namespace apache
