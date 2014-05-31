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

#ifndef THRIFT_ASYNC_INPUTSTREAM_TCC_
#define THRIFT_ASYNC_INPUTSTREAM_TCC_ 1

#include "thrift/lib/cpp2/async/Stream.h"
#include <memory>

namespace apache { namespace thrift {

template <typename T>
InputStreamControllable<T>::~InputStreamControllable() {
}

template <typename T>
InputStreamCallback<T>::InputStreamCallback() : controllable_(nullptr) {
}

template <typename T>
bool InputStreamCallback<T>::isClosed() {
  CHECK_NOTNULL(controllable_);
  return controllable_->isClosed();
}

template <typename T>
void InputStreamCallback<T>::close() {
  CHECK_NOTNULL(controllable_);
  controllable_->close();
}

template <typename T>
void InputStreamCallback<T>::setControllable(
    InputStreamControllable<T>* controllable) {
  CHECK_NOTNULL(controllable);
  CHECK_NULL(controllable_);
  controllable_ = controllable;
}

template <typename T>
InputStreamCallback<T>::~InputStreamCallback() {
}

template <typename T>
InputStreamController<T>::~InputStreamController() {
}

template <typename T>
InputStreamControllerImpl<T>::InputStreamControllerImpl()
  : controllable_(nullptr),
    closed_(false) {
}

template <typename T>
bool InputStreamControllerImpl<T>::hasControllable() {
  return controllable_ != nullptr;
}

template <typename T>
void InputStreamControllerImpl<T>::setControllable(
    InputStreamControllable<T>* controllable) {
  controllable_ = controllable;
}

template <typename T>
bool InputStreamControllerImpl<T>::isClosed() {
  return closed_;
}

template <typename T>
void InputStreamControllerImpl<T>::close() {
  if (isClosed()) {
    return;
  } else if (!hasControllable()) {
    throw StreamException("Stream has not been used in a call.");
  }

  markAsClosed();
  controllable_->close();
}

template <typename T>
void InputStreamControllerImpl<T>::markAsClosed() {
  closed_ = true;
}

template <typename Serializer, typename T>
InputStreamCallbackDeserializer<Serializer, T>::InputStreamCallbackDeserializer(
    std::unique_ptr<InputStreamCallback<T>>&& callback,
    const std::shared_ptr<InputStreamControllerImpl<T>>& controller,
    bool isSync)
  : InputStreamCallbackBase(isSync),
    callback_(std::move(callback)),
    controller_(controller),
    hasCalledOnClose_(false) {

  if (controller_) {
    controller_->setControllable(this);
  }

  if (hasCallback()) {
    callback_->setControllable(this);
  }
}

template <typename Serializer, typename T>
void InputStreamCallbackDeserializer<Serializer, T>::notifyException(
    const folly::exception_wrapper& ex) {
  CHECK(isClosed());
  if (hasCallback()) {
    callback_->onException(ex);
  }
}

template <typename Serializer, typename T>
void InputStreamCallbackDeserializer<Serializer, T>::notifyFinish() {
  CHECK(isClosed());
  if (hasCallback()) {
    callback_->onFinish();
  }
}

template <typename Serializer, typename T>
void InputStreamCallbackDeserializer<Serializer, T>::notifyError(
    const folly::exception_wrapper& error) {
  CHECK(isClosed());
  if (hasCallback()) {
    callback_->onError(error);
  }
}

template <typename Serializer, typename T>
void InputStreamCallbackDeserializer<Serializer, T>::notifyClose() {
  CHECK(isClosed());
  if (!hasCalledOnClose_) {
    hasCalledOnClose_ = true;
    if (hasController()) {
      controller_->markAsClosed();
    }
    if (hasCallback()) {
      callback_->onClose();
    }
  }
}

template <typename Serializer, typename T>
bool InputStreamCallbackDeserializer<Serializer, T>::isClosed() {
  return InputStreamCallbackBase::isClosed();
}

template <typename Serializer, typename T>
void InputStreamCallbackDeserializer<Serializer, T>::close() {
  InputStreamCallbackBase::close();
}

template <typename Deserializer, typename T>
void InputStreamCallbackDeserializer<Deserializer, T>::readItem(
    StreamReader& reader) {
  CHECK(!isClosed());
  CHECK(hasCallback());

  T value;
  reader.readItem<Deserializer>(value);
  callback_->onReceive(value);
  InputStreamCallbackBase::sendAcknowledgement();
}

template <typename Serializer, typename T>
bool InputStreamCallbackDeserializer<Serializer, T>::hasCallback() {
  return bool(callback_);
}

template <typename Serializer, typename T>
bool InputStreamCallbackDeserializer<Serializer, T>::hasController() {
  return bool(controller_);
}

template <typename Serializer, typename T>
bool InputStreamCallbackDeserializer<Serializer, T>::hasCalledOnClose() {
  return hasCalledOnClose_;
}

template <typename Serializer, typename T>
InputStreamCallbackDeserializer<Serializer, T>::
    ~InputStreamCallbackDeserializer(){
  CHECK(hasCalledOnClose_);
  if (hasController()) {
    controller_->setControllable(nullptr);
  }
}

template <typename T>
AsyncInputStream<T>::AsyncInputStream()
  : controller_(std::make_shared<InputStreamControllerImpl<T>>()),
    hasMadeHandler_(false) {
}

template <typename T>
AsyncInputStream<T>::AsyncInputStream(
    std::unique_ptr<InputStreamCallback<T>>&& callback)
  : callback_(std::move(callback)),
    controller_(std::make_shared<InputStreamControllerImpl<T>>()),
    hasMadeHandler_(false) {
}

template <typename T>
void AsyncInputStream<T>::setCallback(
    std::unique_ptr<InputStreamCallback<T>>&& callback) {
  if (hasMadeHandler_) {
    throw StreamException("Stream has already been used in a call.");
  }

  callback_ = std::move(callback);
}

template <typename T>
std::shared_ptr<InputStreamController<T>>
AsyncInputStream<T>::makeController() {
  if (hasMadeHandler_) {
    throw StreamException("Stream has already been used in a call.");
  }

  return controller_;
}

template <typename T>
template <typename Deserializer>
std::unique_ptr<InputStreamCallbackBase> AsyncInputStream<T>::makeHandler() {
  // This method is usually called by the generated code, inside the call
  // to the server or after the server side handler finishes.

  typedef InputStreamCallbackDeserializer<Deserializer, T> Handler;

  if (hasMadeHandler_) {
    throw StreamException("Handler had been created earlier.");
  }

  hasMadeHandler_ = true;

  // If we don't have a callback and nobody else owns another reference to the
  // controller, then we just return a nullptr to indicate that this stream
  // will be simply ignored.
  // As a result, this requires that any controllers be created beforehand.
  if (!callback_ && controller_.use_count() == 1) {
    return nullptr;
  }

  std::unique_ptr<Handler> handler(new Handler(std::move(callback_),
                                               controller_,
                                               false));
  return std::move(handler);
};

template <typename T>
SyncInputStreamControllable<T>::~SyncInputStreamControllable() {
}

template <typename Deserializer, typename T>
SyncInputStreamCallback<Deserializer, T>::SyncInputStreamCallback(
    SyncInputStream<T>* stream)
  : InputStreamCallbackDeserializer<Deserializer, T>(
      nullptr, std::shared_ptr<InputStreamControllerImpl<T>>(), true),
    stream_(stream),
    thisStartedTheEventBaseLoop_(false),
    errorPtr_(nullptr),
    deletedFlagPtr_(nullptr),
    reader_(nullptr),
    hasGottenItem_(false) {
  CHECK_NOTNULL(stream_);
  CHECK(!this->hasCallback());
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::readItem(StreamReader& reader) {
  CHECK_NOTNULL(stream_);
  reader_ = &reader;
  this->setBlockedOnItem(true);

  if (this->isClosed()) {
    skipItem();
  }

  StreamManager* manager = this->getStreamManager();
  if (thisStartedTheEventBaseLoop_) {
    manager->stopEventBaseLoop();
  }
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::notifyFinish() {
  CHECK_NOTNULL(stream_);
  stream_->markAsFinished();
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::notifyException(
    const folly::exception_wrapper& exception) {
  CHECK_NOTNULL(stream_);
  stream_->setException(exception);
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::notifyError(
    const folly::exception_wrapper& error) {
  if (thisStartedTheEventBaseLoop_) {
    CHECK_NOTNULL(errorPtr_);
    *errorPtr_ = error;
  }
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::notifyClose() {
  CHECK_NOTNULL(stream_);
  stream_->markAsClosed();

  bool firstTime = !this->hasCalledOnClose();
  InputStreamCallbackDeserializer<Deserializer, T>::notifyClose();

  if (firstTime) {
    sendAcknowledgementIfGottenItems();

    StreamManager* manager = this->getStreamManager();
    if (thisStartedTheEventBaseLoop_ && manager->hasOpenSyncStreams()) {
      manager->stopEventBaseLoop();
    }
  }
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::waitForItem() {
  CHECK(!this->isClosed());
  CHECK_NOTNULL(stream_);

  if (hasItem()) {
    return;
  }

  try {
    this->readBuffer();

    if (hasItem()) {
      return;
    } else if (this->isClosed() && !needToRunEventBaseLoop()) {
      return;
    } else if (!this->isClosed()) {
      sendAcknowledgementIfGottenItems();
    }
  } catch (...) {
    notifyOutOfLoopError(std::current_exception());
    throw;
  }

  runEventBaseLoop();
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::get(T& value) {
  CHECK(!this->isClosed());
  CHECK_NOTNULL(stream_);

  if (!hasItem()) {
    throw StreamException("No item available.");
  }

  try {
    readItem(value);
    hasGottenItem_ = true;
  } catch (...) {
    notifyOutOfLoopError(std::current_exception());
    throw;
  }
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::close() {
  CHECK(!this->isClosed());
  CHECK_NOTNULL(stream_);

  try {
    if (hasItem()) {
      skipItem();
    }

    while (this->bufferHasUnreadData()) {
      this->readBuffer();

      if (hasItem()) {
        skipItem();
      }
    }

    InputStreamCallbackBase::close();
  } catch (...) {
    notifyOutOfLoopError(std::current_exception());
    throw;
  }

  runEventBaseLoop();
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::
    sendAcknowledgementIfGottenItems() {
  if (hasGottenItem_) {
    hasGottenItem_ = false;
    this->sendAcknowledgement();
  }
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::setStream(
    SyncInputStream<T>* stream) {
  CHECK_NOTNULL(stream);
  stream_ = stream;
}

// this object may be deleted after this method finishes
template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::runEventBaseLoop() {
  folly::exception_wrapper error;
  errorPtr_ = &error;

  bool isDeleted = false;
  deletedFlagPtr_ = &isDeleted;

  thisStartedTheEventBaseLoop_ = true;
  do {
    this->getStreamManager()->startEventBaseLoop();
  } while (!isDeleted && needToRunEventBaseLoop());

  if (error) {
    CHECK(isDeleted);
    error.throwException();
  } else if (!isDeleted) {
    thisStartedTheEventBaseLoop_ = false;
    errorPtr_ = nullptr;
    deletedFlagPtr_ = nullptr;
  }
}

template <typename Deserializer, typename T>
bool SyncInputStreamCallback<Deserializer, T>::needToRunEventBaseLoop() {
  StreamManager* manager = this->getStreamManager();
  return !manager->hasOpenSyncStreams();
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::notifyOutOfLoopError(
    const folly::exception_wrapper& error) {
  StreamManager* manager = this->getStreamManager();
  manager->notifyOutOfLoopError(error);
}

template <typename Deserializer, typename T>
bool SyncInputStreamCallback<Deserializer, T>::hasItem() {
  return reader_ != nullptr;
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::readItem(T& value) {
  reader_->readItem<Deserializer>(value);
  reader_ = nullptr;
  this->setBlockedOnItem(false);
}

template <typename Deserializer, typename T>
void SyncInputStreamCallback<Deserializer, T>::skipItem() {
  reader_->skipItem(this->getType());
  reader_ = nullptr;
  this->setBlockedOnItem(false);
}


template <typename Deserializer, typename T>
SyncInputStreamCallback<Deserializer, T>::~SyncInputStreamCallback() {
  CHECK_NOTNULL(stream_);
  stream_->clearControllable();
  stream_ = nullptr;

  StreamManager* manager = this->getStreamManager();
  if (thisStartedTheEventBaseLoop_) {
    manager->stopEventBaseLoop();
  }

  if (deletedFlagPtr_ != nullptr) {
    *deletedFlagPtr_ = true;
  }
}

template <typename T>
SyncInputStream<T>::SyncInputStream()
  : controllable_(nullptr),
    finished_(false),
    closed_(false) {
}

template <typename T>
SyncInputStream<T>::SyncInputStream(SyncInputStream<T>&& another) noexcept
  : controllable_(another.controllable_),
    finished_(another.finished_),
    closed_(another.closed_),
    exception_(another.exception_) {

  another.controllable_ = nullptr;
  another.finished_ = false;
  another.closed_ = false;
  another.exception_ = nullptr;

  if (controllable_ != nullptr) {
    controllable_->setStream(this);
  }
}

template <typename T>
SyncInputStream<T>& SyncInputStream<T>::operator=(
    SyncInputStream<T>&& another) noexcept {
  std::swap(controllable_, another.controllable_);
  std::swap(finished_, another.finished_);
  std::swap(closed_, another.closed_);
  std::swap(exception_, another.exception_);

  if (controllable_ != nullptr) {
    controllable_->setStream(this);
  }

  if (another.controllable_ != nullptr) {
    another.controllable_->setStream(&another);
  }

  return *this;
}

template <typename T>
bool SyncInputStream<T>::isDone() {
  if (hasException()) {
    return false;
  } else if (hasBeenClosed()) {
    return true;
  } else if (!hasControllable()) {
    throw StreamException("Stream has not been used in a call.");
  }

  // If the stream finished or the stream errored, then source_ will
  // call markAsFinished() or markAsClosed() respectively, and isClosed()
  // will return true after the call.
  //
  // If the stream encodes an exception, then setException(...) will be
  // set with the appropriate exception, hasBeenClosed() will be true and
  // hasException() will also be true, so isClosed() will be false.
  controllable_->waitForItem();

  return isClosed();
}

template <typename T>
void SyncInputStream<T>::get(T& value) {
  if (hasException()) {
    rethrowException();
  } else if (hasBeenClosed()) {
    throw StreamException("Stream has already been closed.");
  } else if (!hasControllable()) {
    throw StreamException("Stream has not been used in a call.");
  }

  controllable_->get(value);
}

template <typename T>
void SyncInputStream<T>::close() {
  if (hasException()) {
    CHECK(hasBeenClosed());
    clearException();
    return;
  } else if (hasBeenClosed()) {
    return;
  } else if (!hasControllable()) {
    throw StreamException("Stream has not been used in a call.");
  }

  controllable_->close();
}

template <typename T>
bool SyncInputStream<T>::isFinished() {
  return finished_;
}

template <typename T>
bool SyncInputStream<T>::isClosed() {
  return hasBeenClosed() && !hasException();
}

template <typename T>
bool SyncInputStream<T>::hasBeenClosed() {
  return closed_;
}

template <typename T>
bool SyncInputStream<T>::hasControllable() {
  return controllable_ != nullptr;
}

template <typename T>
bool SyncInputStream<T>::hasException() {
  return (bool) exception_;
}

template <typename T>
void SyncInputStream<T>::setException(
    const folly::exception_wrapper& exception) {
  exception_ = exception;
}

template <typename T>
void SyncInputStream<T>::rethrowException() {
  CHECK(hasException());
  folly::exception_wrapper copy = exception_;
  clearException();
  copy.throwException();
}

template <typename T>
void SyncInputStream<T>::clearException() {
  exception_ = folly::exception_wrapper();
}

template <typename T>
void SyncInputStream<T>::markAsFinished() {
  finished_ = true;
}

template <typename T>
void SyncInputStream<T>::markAsClosed() {
  closed_ = true;
}

template <typename T>
void SyncInputStream<T>::clearControllable() {
  controllable_ = nullptr;
}

template <typename T>
template <typename Deserializer>
std::unique_ptr<InputStreamCallbackBase> SyncInputStream<T>::makeHandler() {
  auto callback = new SyncInputStreamCallback<Deserializer, T>(this);
  controllable_ = callback;
  return std::unique_ptr<InputStreamCallbackBase>(callback);
}

template <typename T>
SyncInputStream<T>::~SyncInputStream() {
  if (hasControllable()) {
    close();
  }
}

template <typename T>
StreamSingleton<T>::StreamSingleton() {
}

template <typename T>
void StreamSingleton<T>::get(T& value) {
  if (stream_.isDone()) {
    throw StreamException("No item available.");
  }

  stream_.get(value);
  stream_.close();
}

template <typename T>
template <typename Deserializer>
std::unique_ptr<InputStreamCallbackBase> StreamSingleton<T>::makeHandler() {
  return stream_.template makeHandler<Deserializer>();
}

}} // apache::thrift

#endif // THRIFT_ASYNC_INPUTSTREAM_TCC_
