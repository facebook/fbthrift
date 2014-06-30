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

#ifndef THRIFT_ASYNC_OUTPUTSTREAM_TCC_
#define THRIFT_ASYNC_OUTPUTSTREAM_TCC_ 1

#include <thrift/lib/cpp2/async/Stream.h>
#include <utility>
#include <memory>

namespace apache { namespace thrift {

template <typename T>
OutputStreamControllable<T>::~OutputStreamControllable() {
}

template <typename T>
OutputStreamCallback<T>::OutputStreamCallback() : controllable_(nullptr) {
}

template <typename T>
bool OutputStreamCallback<T>::isClosed() {
  CHECK_NOTNULL(controllable_);
  return controllable_->isClosed();
}

template <typename T>
void OutputStreamCallback<T>::close() {
  CHECK_NOTNULL(controllable_);
  controllable_->close();
}

template <typename T>
void OutputStreamCallback<T>::put(const T& data) {
  CHECK_NOTNULL(controllable_);
  controllable_->put(data);
}

template <typename T>
void OutputStreamCallback<T>::putException(
    const folly::exception_wrapper& error) {
  CHECK_NOTNULL(controllable_);
  controllable_->putException(error);
}

template <typename T>
void OutputStreamCallback<T>::setControllable(
    OutputStreamControllable<T>* controllable) {
  CHECK_NOTNULL(controllable);
  CHECK_NULL(controllable_);
  controllable_ = controllable;
}

template <typename T>
OutputStreamCallback<T>::~OutputStreamCallback() {
}

template <typename T>
OutputStreamController<T>::~OutputStreamController() {
}

template <typename T>
OutputStreamControllerImpl<T>::OutputStreamControllerImpl()
  : controllable_(nullptr),
    closed_(false) {
}

template <typename T>
void OutputStreamControllerImpl<T>::setControllable(
    OutputStreamControllable<T>* controllable) {
  controllable_ = controllable;
}

template <typename T>
bool OutputStreamControllerImpl<T>::isClosed() {
  return closed_;
}

template <typename T>
void OutputStreamControllerImpl<T>::close() {
  if (isClosed()) {
    return;
  } else if (!hasControllable()) {
    throw StreamException("Stream has not been used in a call.");
  }

  markAsClosed();
  controllable_->close();
}

template <typename T>
void OutputStreamControllerImpl<T>::put(const T& data) {
  if (isClosed()) {
    throw StreamException("Stream has already been closed.");
  } else if (!hasControllable()) {
    throw StreamException("Stream has not been used in a call.");
  }

  controllable_->put(data);
}

template <typename T>
void OutputStreamControllerImpl<T>::putException(
    const folly::exception_wrapper& exception) {
  if (isClosed()) {
    throw StreamException("Stream has already been closed.");
  } else if (!hasControllable()) {
    throw StreamException("Stream has not been used in a call.");
  }

  controllable_->putException(exception);
}

template <typename T>
bool OutputStreamControllerImpl<T>::hasControllable() {
  return controllable_ != nullptr;
}

template <typename T>
void OutputStreamControllerImpl<T>::markAsClosed() {
  closed_ = true;
}

template <typename Serializer, typename T>
OutputStreamCallbackSerializer<Serializer, T>::OutputStreamCallbackSerializer(
    std::unique_ptr<OutputStreamCallback<T>>&& callback,
    const std::shared_ptr<OutputStreamControllerImpl<T>>&  controller,
    bool isSync)
  : OutputStreamCallbackBase(isSync),
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
void OutputStreamCallbackSerializer<Serializer, T>::notifySend() {
  CHECK(!isClosed());
  if (hasCallback()) {
    callback_->onSend();
  }
}

template <typename Serializer, typename T>
void OutputStreamCallbackSerializer<Serializer, T>::notifyError(
    const folly::exception_wrapper& error) {
  CHECK(isClosed());
  if (hasCallback()) {
    callback_->onError(error);
  }
}

template <typename Serializer, typename T>
void OutputStreamCallbackSerializer<Serializer, T>::notifyClose() {
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
bool OutputStreamCallbackSerializer<Serializer, T>::isClosed() {
  return OutputStreamCallbackBase::isClosed();
}

template <typename Serializer, typename T>
void OutputStreamCallbackSerializer<Serializer, T>::close() {
  OutputStreamCallbackBase::close();
}

template <typename Serializer, typename T>
void OutputStreamCallbackSerializer<Serializer, T>::put(const T& value) {
  CHECK(isLinked());

  if (isClosed()) {
    throw StreamException("Stream has already been closed.");
  }

  StreamWriter* writer = getStreamWriter();
  writer->writeItem<Serializer>(getParamId(), value);
  getStreamSink()->onStreamPut(this);
}

template <typename Serializer, typename T>
void OutputStreamCallbackSerializer<Serializer, T>::putException(
    const folly::exception_wrapper& exception){
  CHECK(isLinked());

  if (isClosed()) {
    throw StreamException("Stream has already been closed.");
  }

  getStreamSink()->onStreamException(this, exception);
}

template <typename Serializer, typename T>
bool OutputStreamCallbackSerializer<Serializer, T>::hasCallback() {
  return (bool) callback_;
}

template <typename Serializer, typename T>
bool OutputStreamCallbackSerializer<Serializer, T>::hasController() {
  return controller_ != nullptr;
}

template <typename Serializer, typename T>
bool OutputStreamCallbackSerializer<Serializer, T>::hasCalledOnClose() {
  return hasCalledOnClose_;
}

template <typename Serializer, typename T>
OutputStreamCallbackSerializer<Serializer, T>::
    ~OutputStreamCallbackSerializer(){
  if (hasController()) {
    controller_->setControllable(nullptr);
  }
}

template <typename T>
AsyncOutputStream<T>::AsyncOutputStream()
  : controller_(std::make_shared<OutputStreamControllerImpl<T>>()),
    hasMadeHandler_(false) {
}

template <typename T>
AsyncOutputStream<T>::AsyncOutputStream(
    std::unique_ptr<OutputStreamCallback<T>>&& callback)
  : callback_(std::move(callback)),
    controller_(std::make_shared<OutputStreamControllerImpl<T>>()),
    hasMadeHandler_(false) {
}

template <typename T>
void AsyncOutputStream<T>::setCallback(
    std::unique_ptr<OutputStreamCallback<T>>&& callback) {
  if (hasMadeHandler_) {
    throw StreamException("Stream has already been used in a call.");
  }

  callback_ = std::move(callback);
}

template <typename T>
std::shared_ptr<OutputStreamController<T>>
AsyncOutputStream<T>::makeController() {
  if (hasMadeHandler_) {
    throw StreamException("Stream has already been used in a call.");
  }

  return controller_;
}

template <typename T>
template <typename Serializer>
std::unique_ptr<OutputStreamCallbackBase> AsyncOutputStream<T>::makeHandler() {
  // This method is usually called by the generated code, inside the call
  // to the server or after the server side handler finishes.

  typedef OutputStreamCallbackSerializer<Serializer, T> Handler;

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
SyncOutputStreamControllable<T>::~SyncOutputStreamControllable() {
}

template <typename Serializer, typename T>
SyncOutputStreamCallback<Serializer, T>::SyncOutputStreamCallback(
    SyncOutputStream<T>* stream,
    const std::shared_ptr<OutputStreamControllerImpl<T>>& controller)
  : OutputStreamCallbackSerializer<Serializer, T>(nullptr, controller, true),
    stream_(stream),
    thisStartedTheEventBaseLoop_(false),
    errorPtr_(nullptr),
    deletedFlagPtr_(nullptr) {
  CHECK_NOTNULL(stream_);
  CHECK(!this->hasCallback());
}

template <typename Serializer, typename T>
void SyncOutputStreamCallback<Serializer, T>::setStream(
    SyncOutputStream<T>* stream) {
  CHECK_NOTNULL(stream);
  stream_ = stream;
}

template <typename Serializer, typename T>
void SyncOutputStreamCallback<Serializer, T>::notifySend() {
  CHECK_NOTNULL(stream_);
  StreamManager* manager = this->getStreamManager();
  if (thisStartedTheEventBaseLoop_) {
    manager->stopEventBaseLoop();
  }
}

template <typename Serializer, typename T>
void SyncOutputStreamCallback<Serializer, T>::notifyError(
    const folly::exception_wrapper& error) {
  if (thisStartedTheEventBaseLoop_) {
    CHECK_NOTNULL(errorPtr_);
    *errorPtr_ = error;
  }
}

template <typename Serializer, typename T>
void SyncOutputStreamCallback<Serializer, T>::notifyClose() {
  bool firstTime = !this->hasCalledOnClose();
  OutputStreamCallbackSerializer<Serializer, T>::notifyClose();

  if (firstTime) {
    StreamManager* manager = this->getStreamManager();
    if (thisStartedTheEventBaseLoop_ &&
        manager->hasOpenSyncStreams()) {
      manager->stopEventBaseLoop();
    }
  }
}

// this object may be deleted after this method finishes
template <typename Serializer, typename T>
void SyncOutputStreamCallback<Serializer, T>::runEventBaseLoop() {
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

template <typename Serializer, typename T>
bool SyncOutputStreamCallback<Serializer, T>::needToRunEventBaseLoop() {
  StreamManager* manager = this->getStreamManager();
  return !manager->hasOpenSyncStreams();
}

template <typename Serializer, typename T>
void SyncOutputStreamCallback<Serializer, T>::notifyOutOfLoopError(
    const folly::exception_wrapper& error) {
  StreamManager* manager = this->getStreamManager();
  manager->notifyOutOfLoopError(error);
}

template <typename Serializer, typename T>
SyncOutputStreamCallback<Serializer, T>::~SyncOutputStreamCallback() {
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
SyncOutputStream<T>::SyncOutputStream()
  : controllable_(nullptr),
    controller_(std::make_shared<OutputStreamControllerImpl<T>>()) {
}

template <typename T>
SyncOutputStream<T>::SyncOutputStream(SyncOutputStream<T>&& another) noexcept
  : controllable_(another.controllable_),
    controller_(std::move(another.controller_)) {

  another.controllable_ = nullptr;

  if (controllable_ != nullptr) {
    controllable_->setStream(this);
  }
}

template <typename T>
SyncOutputStream<T>& SyncOutputStream<T>::operator=(
    SyncOutputStream<T>&& another) noexcept {
  std::swap(controllable_, another.controllable_);
  std::swap(controller_, another.controller_);

  if (controllable_ != nullptr) {
    controllable_->setStream(this);
  }

  if (another.controllable_!= nullptr) {
    another.controllable_->setStream(&another);
  }

  return *this;
}

template <typename T>
void SyncOutputStream<T>::put(const T& value) {
  try {
    controller_->put(value);

  } catch(const StreamException& e) {
    throw;

  } catch(...) {
    CHECK(hasControllable());
    controllable_->notifyOutOfLoopError(std::current_exception());
    throw;
  }

  CHECK_NOTNULL(controllable_);
  controllable_->runEventBaseLoop();
}

template <typename T>
void SyncOutputStream<T>::putException(
    const folly::exception_wrapper& exception) {
  try {
    controller_->putException(exception);

  } catch(const StreamException& e) {
    throw;

  } catch(...) {
    CHECK(hasControllable());
    controllable_->notifyOutOfLoopError(std::current_exception());
    throw;
  }

  CHECK_NOTNULL(controllable_);
  controllable_->runEventBaseLoop();
}

template <typename T>
void SyncOutputStream<T>::close() {
  if (isClosed()) {
    return;
  }

  try {
    controller_->close();

  } catch(const StreamException& e) {
    throw;

  } catch(...) {
    CHECK(hasControllable());
    controllable_->notifyOutOfLoopError(std::current_exception());
    throw;
  }

  CHECK_NOTNULL(controllable_);
  controllable_->runEventBaseLoop();
}

template <typename T>
bool SyncOutputStream<T>::isClosed() {
  return controller_->isClosed();
}

template <typename T>
template <typename Serializer>
std::unique_ptr<OutputStreamCallbackBase> SyncOutputStream<T>::makeHandler() {
  auto callback = new SyncOutputStreamCallback<Serializer, T>(this,
                                                              controller_);
  controllable_ = callback;
  return std::unique_ptr<OutputStreamCallbackBase>(callback);
}

template <typename T>
bool SyncOutputStream<T>::hasControllable() {
  return controllable_;
}

template <typename T>
void SyncOutputStream<T>::clearControllable() {
  controllable_ = nullptr;
}

template <typename T>
SyncOutputStream<T>::~SyncOutputStream() {
  if (hasControllable()) {
    close();
  }
}

}} // apache::thrift

#endif // THRIFT_ASYNC_OUTPUTSTREAM_TCC_
