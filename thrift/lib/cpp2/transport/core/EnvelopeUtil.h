/*
 * Copyright 2017-present Facebook, Inc.
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

#pragma once

#include <folly/io/IOBuf.h>
#include <glog/logging.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {

class EnvelopeUtil {
 public:
  // Strips the envelope out of the payload and moves the information
  // contained in it into metadata.  This function is required to
  // maintain legacy compatibility.  Eventually, the envelope should
  // only be used with the header transport (i.e., not even created in
  // the first place), at which point this function will be
  // deprecated.
  static bool stripEnvelope(
      RequestRpcMetadata* metadata,
      std::unique_ptr<folly::IOBuf>& payload) noexcept {
    MessageType mtype;
    uint64_t sz;
    while (payload->length() == 0) {
      if (payload->next() != payload.get()) {
        payload = payload->pop();
      } else {
        LOG(ERROR) << "Payload is empty";
        return false;
      }
    }
    try {
      // Sequence id is always 0 in the envelope.  So we ignore it.
      int32_t seqId;
      auto protByte = payload->data()[0];
      switch (protByte) {
        case 0x80: {
          BinaryProtocolReader reader;
          metadata->protocol = ProtocolId::BINARY;
          reader.setInput(payload.get());
          sz = reader.readMessageBegin(metadata->name, mtype, seqId);
        } break;
        case 0x82: {
          metadata->protocol = ProtocolId::COMPACT;
          CompactProtocolReader reader;
          reader.setInput(payload.get());
          sz = reader.readMessageBegin(metadata->name, mtype, seqId);
        } break;
        // TODO: Add Frozen2 case.
        default:
          LOG(ERROR) << "Unknown protocol: " << protByte;
          return false;
      }
    } catch (const TException& ex) {
      LOG(ERROR) << "Invalid envelope: " << ex.what();
      return false;
    }
    // Remove the envelope.
    while (payload->length() < sz) {
      sz -= payload->length();
      payload = payload->pop();
    }
    payload->trimStart(sz);
    metadata->__isset.protocol = true;
    metadata->__isset.name = true;
    return true;
  }
};

} // namespace thrift
} // namespace apache
