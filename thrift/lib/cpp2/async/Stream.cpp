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

#include "thrift/lib/cpp2/async/Stream.h"
#include <utility>

namespace apache { namespace thrift {

StreamException::StreamException(const std::string& message)
  : TLibraryException(message) {
}

StreamManager::StreamManager(MockEventBaseCallback* mockEventBase,
                             StreamSource::StreamMap&& sourceMap,
                             StreamSource::ExceptionDeserializer deserializer,
                             StreamSink::StreamMap&& sinkMap,
                             StreamSink::ExceptionSerializer serializer,
                             std::unique_ptr<StreamEndCallback> endCallback)
  : eventBase_(nullptr),
    mockEventBase_(mockEventBase),
    channel_(nullptr),
    source_(&sink_, this, std::move(sourceMap), deserializer),
    sink_(this, std::move(sinkMap), serializer),
    endCallback_(std::move(endCallback)),
    cancelled_(false) {
}

StreamManager::StreamManager(apache::thrift::async::TEventBase* eventBase,
                             StreamSource::StreamMap&& sourceMap,
                             StreamSource::ExceptionDeserializer deserializer,
                             StreamSink::StreamMap&& sinkMap,
                             StreamSink::ExceptionSerializer serializer,
                             std::unique_ptr<StreamEndCallback> endCallback)
  : eventBase_(eventBase),
    mockEventBase_(nullptr),
    channel_(nullptr),
    source_(&sink_, this, std::move(sourceMap), deserializer),
    sink_(this, std::move(sinkMap), serializer),
    endCallback_(std::move(endCallback)),
    cancelled_(false) {
}

void StreamManager::cancel() {
  cancelled_ = true;
  sink_.onCancel();
  source_.onCancel();

  if (endCallback_) {
    endCallback_->onStreamCancel();
    endCallback_.reset();
  }
}

bool StreamManager::isCancelled() {
  return cancelled_;
}

void StreamManager::notifySend() {
  sink_.onSend();
}

void StreamManager::notifyReceive(std::unique_ptr<folly::IOBuf>&& buffer) {
  source_.onReceive(std::move(buffer));
}

bool StreamManager::isDone() {
  return hasError() || isCancelled() ||
         (sink_.hasSentEnd() && source_.hasReceivedEnd());
}

bool StreamManager::hasError() {
  return source_.hasError() || sink_.hasError();
}

void StreamManager::setChannelCallback(StreamChannelCallback* channel) {
  CHECK_NOTNULL(channel);
  channel_ = channel;
}

void StreamManager::setInputProtocol(ProtocolType type) {
  source_.setProtocolType(type);
}

void StreamManager::setOutputProtocol(ProtocolType type) {
  sink_.setProtocolType(type);
}

bool StreamManager::isSendingEnd() {
  return sink_.isSendingEnd();
}

bool StreamManager::needToSendEnd() {
  return (!hasError() &&
          !sink_.hasWrittenEnd() &&
          !sink_.hasOpenStreams() &&
          !source_.hasOpenStreams());
}

bool StreamManager::hasOpenSyncStreams() {
  return (sink_.hasOpenSyncStreams() || source_.hasOpenSyncStreams());
}

// if there is an error, then this object may have been deleted
void StreamManager::startEventBaseLoop() {
  if (eventBase_ != nullptr) {
    eventBase_->loopForever();
  } else {
    mockEventBase_->startEventBaseLoop();
  }
}

void StreamManager::stopEventBaseLoop() {
  if (eventBase_ != nullptr) {
    eventBase_->terminateLoopSoon();
  } else {
    mockEventBase_->stopEventBaseLoop();
  }
}

void StreamManager::sendData(std::unique_ptr<folly::IOBuf>&& data) {
  CHECK_NOTNULL(channel_);
  channel_->onStreamSend(std::move(data));
}

void StreamManager::notifyError(const std::exception_ptr& error) {
  sink_.onError(error);
  source_.onError(error);

  if (endCallback_) {
    endCallback_->onStreamError(error);
    endCallback_.reset();
  }
}

void StreamManager::notifyOutOfLoopError(const std::exception_ptr& error){
  notifyError(error);
  channel_->onOutOfLoopStreamError(error);
}

StreamManager::~StreamManager() {
  CHECK(isDone());

  if (endCallback_) {
    endCallback_->onStreamComplete();
    endCallback_.reset();
  }
}

}} // apache::thrift
