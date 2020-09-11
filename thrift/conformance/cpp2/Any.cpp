/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <thrift/conformance/cpp2/Any.h>

#include <thrift/conformance/cpp2/Protocol.h>

namespace apache::thrift::conformance {

Protocol getProtocol(const Any& any) noexcept {
  if (any.protocol_ref() != StandardProtocol::None) {
    return Protocol(*any.protocol_ref());
  }
  if (any.customProtocol_ref()) {
    return Protocol(any.customProtocol_ref().value_unchecked());
  }
  return {};
}

bool hasProtocol(const Any& any, const Protocol& protocol) noexcept {
  if (any.protocol_ref() != StandardProtocol::None) {
    // Fast path for the standard case.
    return protocol.standard() == any.protocol_ref();
  }
  if (any.customProtocol_ref()) {
    // Might not be normalized, so pass through the Protocol constructor.
    return Protocol(any.customProtocol_ref().value_unchecked()) == protocol;
  }
  // We have no protocol.
  return protocol.isNone();
}

void normalizeProtocol(Any& any) noexcept {
  if (!any.customProtocol_ref()) {
    // Nothing to normalize
    return;
  }

  if (any.protocol_ref() != StandardProtocol::None) {
    // We have junk in the custom protocol field.
    // This 'should' never happen.
    any.customProtocol_ref().reset();
    return;
  }

  // See if this should be normalized to a stnadard protocol.
  if (auto standard =
          getStandardProtocol(any.customProtocol_ref().value_unchecked())) {
    any.customProtocol_ref().reset();
    any.protocol_ref() = *standard;
  }
}

} // namespace apache::thrift::conformance
