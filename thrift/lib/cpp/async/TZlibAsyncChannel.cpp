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
#include <thrift/lib/cpp/async/TZlibAsyncChannel.h>

using apache::thrift::transport::TMemoryBuffer;
using std::shared_ptr;


namespace apache { namespace thrift { namespace async {

TZlibAsyncChannel::SendRequest::SendRequest()
  : compressedBuffer_(new TMemoryBuffer)
  , zlibTransport_(compressedBuffer_)
  , sendSuccess_(std::bind(&SendRequest::sendSuccess, this))
  , sendError_(std::bind(&SendRequest::sendError, this))
  , callback_()
  , errorCallback_() {
}

void TZlibAsyncChannel::SendRequest::set(
    const VoidCallback& callback,
    const VoidCallback& errorCallback,
    TMemoryBuffer* message) {
  assert(!isSet());

  uint32_t len = message->available_read();
  const uint8_t* buf = message->borrow(nullptr, &len);

  zlibTransport_.write(buf, len);
  zlibTransport_.flush();

  message->consume(len);

  callback_ = callback;
  errorCallback_ = errorCallback;
}

void TZlibAsyncChannel::SendRequest::send(TAsyncEventChannel* channel) {
  channel->sendMessage(sendSuccess_, sendError_, compressedBuffer_.get());
}

void TZlibAsyncChannel::SendRequest::invokeCallback(VoidCallback callback) {
  // Note: it is important that this function accept an actual callback object,
  // and not a reference to the callback.  The code will reset the callback_
  // and errorCallback_ member variables before invoking the callback, so it is
  // important to make a copy of the callback that will be invoked.

  callback_ = VoidCallback();
  errorCallback_ = VoidCallback();
  compressedBuffer_->resetBuffer(0);

  callback();
}

void TZlibAsyncChannel::SendRequest::sendSuccess() {
  invokeCallback(callback_);
}

void TZlibAsyncChannel::SendRequest::sendError() {
  invokeCallback(errorCallback_);
}

TZlibAsyncChannel::RecvRequest::RecvRequest()
  : compressedBuffer_(new TMemoryBuffer)
  , zlibTransport_(compressedBuffer_)
  , recvSuccess_(std::bind(&RecvRequest::recvSuccess, this))
  , recvError_(std::bind(&RecvRequest::recvError, this))
  , callback_()
  , errorCallback_() {
}

void TZlibAsyncChannel::RecvRequest::set(
    const VoidCallback& callback,
    const VoidCallback& errorCallback,
    TMemoryBuffer* message) {
  assert(!isSet());

  callback_ = callback;
  errorCallback_ = errorCallback;
  callbackBuffer_ = message;
}

void TZlibAsyncChannel::RecvRequest::recv(TAsyncEventChannel* channel) {
  channel->recvMessage(recvSuccess_, recvError_, compressedBuffer_.get());
}

void TZlibAsyncChannel::RecvRequest::invokeCallback(VoidCallback callback) {
  // Note: it is important that this function accept an actual callback object,
  // and not a reference to the callback.  The code will reset the callback_
  // and errorCallback_ member variables before invoking the callback, so it is
  // important to make a copy of the callback that will be invoked.

  callback_ = VoidCallback();
  errorCallback_ = VoidCallback();
  callbackBuffer_ = nullptr;
  compressedBuffer_->resetBuffer(0);

  callback();
}

void TZlibAsyncChannel::RecvRequest::recvSuccess() {
  // Uncompress the buffer
  try {
    // Process in 64kb blocks
    const uint32_t kUncompressBlock = 1 << 16;
    while (true) {
      uint8_t* writePtr = callbackBuffer_->getWritePtr(kUncompressBlock);
      uint32_t readBytes = zlibTransport_.read(writePtr, kUncompressBlock);
      if (readBytes <= 0) {
        break;
      } else {
        callbackBuffer_->wroteBytes(readBytes);
      }
    }
  } catch (const std::exception& ex) {
    T_ERROR("zlib channel: error uncompressing data: %s", ex.what());
    recvError();
    return;
  }

  invokeCallback(callback_);
}

void TZlibAsyncChannel::RecvRequest::recvError() {
  invokeCallback(errorCallback_);
}

TZlibAsyncChannel::TZlibAsyncChannel(
    const shared_ptr<TAsyncEventChannel>& channel)
  : channel_(channel)
  , sendRequest_()
  , recvRequest_() {
}

void TZlibAsyncChannel::sendMessage(const VoidCallback& callback,
                                    const VoidCallback& errorCallback,
                                    TMemoryBuffer* message) {
  assert(message);
  DestructorGuard dg(this);

  if (!good()) {
    T_DEBUG_T("zlib channel: attempted to send on non-good channel");
    return errorCallback();
  }

  if (sendRequest_.isSet()) {
    T_ERROR("zlib async channel currently does not support multiple "
            "outstanding send requests");
    return errorCallback();
  }

  try {
    sendRequest_.set(callback, errorCallback, message);
  } catch (const std::exception& ex) {
    T_ERROR("zlib async channel: error initializing send: %s", ex.what());
    return errorCallback();
  }

  sendRequest_.send(channel_.get());
}

void TZlibAsyncChannel::recvMessage(const VoidCallback& callback,
                                    const VoidCallback& errorCallback,
                                    TMemoryBuffer* message) {
  assert(message);
  DestructorGuard dg(this);

  if (!good()) {
    T_DEBUG_T("zlib channel: attempted to read on non-good channel");
    return errorCallback();
  }

  if (recvRequest_.isSet()) {
    T_ERROR("zlib async channel is already reading");
    return errorCallback();
  }

  try {
    recvRequest_.set(callback, errorCallback, message);
  } catch (const std::exception& ex) {
    T_ERROR("zlib async channel: error initializing receive: %s", ex.what());
    return errorCallback();
  }

  recvRequest_.recv(channel_.get());
}

void TZlibAsyncChannel::sendAndRecvMessage(const VoidCallback& callback,
                                           const VoidCallback& errorCallback,
                                           TMemoryBuffer* sendBuf,
                                           TMemoryBuffer* recvBuf) {
  const VoidCallback& sendSucceeded =
    std::bind(&TZlibAsyncChannel::recvMessage, this, callback, errorCallback,
              recvBuf);

  return sendMessage(sendSucceeded, errorCallback, sendBuf);
}

}}} // apache::thrift::async
