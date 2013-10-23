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
#include "thrift/lib/cpp/async/THttpAsyncChannel.h"

#include "thrift/lib/cpp/async/TStreamAsyncChannel.tcc"

using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::transport::TTransportException;
using apache::thrift::util::THttpParser;

namespace apache { namespace thrift { namespace async {

// Explicit instantiation of THttpAsyncChannel's parent class,
// so other users don't have to include TStreamAsyncChannel.tcc
template class TStreamAsyncChannel<detail::THttpACWriteRequest,
                                   detail::THttpACReadState>;

namespace detail {

THttpACWriteRequest::THttpACWriteRequest(const VoidCallback& callback,
                                         const VoidCallback& errorCallback,
                                         TMemoryBuffer* message,
                                         TAsyncEventChannel* channel)
  : TAsyncChannelWriteRequestBase(callback, errorCallback, message)  {
  channel_ = static_cast<THttpAsyncChannel*>(channel);
}

void THttpACWriteRequest::write(
    TAsyncTransport* transport,
    TAsyncTransport::WriteCallback* callback) noexcept {
  uint32_t len = buffer_.available_read();
  const int kNOps = 10;
  iovec ops[kNOps];
  int opsLen = channel_->constructHeader(ops, kNOps - 1, len, lengthBuf_);
  assert(opsLen < kNOps);
  ops[opsLen].iov_base = const_cast<uint8_t*>(buffer_.borrow(nullptr, &len));
  ops[opsLen].iov_len = len;
  opsLen++;
  transport->writev(callback, ops, opsLen);
}

void THttpACWriteRequest::writeSuccess() noexcept {
  buffer_.consume(buffer_.available_read());
  invokeCallback();
}

void THttpACWriteRequest::writeError(
    size_t bytesWritten,
    const TTransportException& ex) noexcept {
  T_ERROR("THttpAC: write failed after writing %zu bytes: %s",
          bytesWritten, ex.what());
  invokeErrorCallback();
}

void THttpACReadState::getReadBuffer(void** bufReturn, size_t* lenReturn) {
  parser_->getReadBuffer(bufReturn, lenReturn);
}

bool THttpACReadState::readDataAvailable(size_t len) {
  return parser_->readDataAvailable(len);
}

} // detail

}}}  // apache::thrift::async
