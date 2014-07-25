/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include <thrift/lib/cpp/async/THeaderAsyncChannel.h>

#include <thrift/lib/cpp/protocol/THeaderProtocol.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>

#include <thrift/lib/cpp/async/TStreamAsyncChannel.tcc>
#include <thrift/lib/cpp/async/TBinaryAsyncChannel.h>
#include <thrift/lib/cpp/async/TUnframedAsyncChannel.tcc>

using apache::thrift::protocol::TBinaryProtocol;
using apache::thrift::protocol::THeaderProtocol;
using apache::thrift::transport::TBufferBase;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::transport::TTransportException;

namespace apache { namespace thrift { namespace async {

// Explicit instantiation of THeaderAsyncChannel's parent classes,
// so other users don't have to include TStreamAsyncChannel.tcc
template class TUnframedAsyncChannel<detail::THeaderACProtocolTraits>;
template class TStreamAsyncChannel<
  detail::TUnframedACWriteRequest,
  detail::TUnframedACReadState<detail::THeaderACProtocolTraits> >;
template class detail::TUnframedACReadState<detail::THeaderACProtocolTraits>;

namespace detail {

bool THeaderACProtocolTraits::getMessageLength(uint8_t* buffer,
                                               uint32_t bufferLength,
                                               uint32_t* messageLength) {
  int32_t frameSize;
  if (bufferLength >= sizeof(frameSize)) {
    // We've finished reading the frame size
    // Convert the frame size to host byte order
    memcpy(&frameSize, buffer, sizeof(frameSize));
    frameSize = ntohl(frameSize);
    if ((frameSize & TBinaryProtocol::VERSION_MASK) == TBinaryProtocol::VERSION_1) {
      bool foundCompleteMessage = tryReadUnframed(buffer, bufferLength, messageLength, strictRead_);
      if (foundCompleteMessage && (*messageLength > maxMessageSize_)) {
        throw TTransportException("Frame size exceeded maximum");
      }
      return foundCompleteMessage;
    }
    if (frameSize > maxMessageSize_) {
      throw TTransportException("Frame size exceeded maximum");
    }
    *messageLength = frameSize + sizeof(frameSize); // frame size incl
    return (bufferLength >= *messageLength);
  } else {
    return tryReadUnframed(buffer, bufferLength, messageLength, strictRead_);
  }
  return false;
}

}}}} // apache::thrift::async::detail
