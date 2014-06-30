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

#include <thrift/lib/cpp2/async/Stream.h>
#include <vector>

using namespace apache::thrift;
using apache::thrift::protocol::TType;

OutputStreamCallbackBase::OutputStreamCallbackBase(bool isSync)
    : manager_(nullptr),
      sink_(nullptr),
      writer_(nullptr),
      type_(TType::T_STOP),
      isSync_(isSync),
      paramId_(0),
      sendState_(SendState::INIT),
      closed_(false) {
}

StreamManager* OutputStreamCallbackBase::getStreamManager() {
  return manager_;
}

StreamSink* OutputStreamCallbackBase::getStreamSink() {
  return sink_;
}

StreamWriter* OutputStreamCallbackBase::getStreamWriter() {
  return writer_;
}

bool OutputStreamCallbackBase::isSync() {
  return isSync_;
}

void OutputStreamCallbackBase::link(StreamManager* manager,
                                    StreamSink* sink,
                                    StreamWriter* writer,
                                    int16_t paramId) {
  CHECK_NOTNULL(manager);
  CHECK_NOTNULL(sink);
  CHECK_NOTNULL(writer);

  manager_ = manager;
  sink_ = sink;
  writer_ = writer;
  paramId_ = paramId;
}

int16_t OutputStreamCallbackBase::getParamId() {
  return paramId_;
}

void OutputStreamCallbackBase::setSendState(SendState state) {
  sendState_ = state;
}

OutputStreamCallbackBase::SendState OutputStreamCallbackBase::getSendState() {
  return sendState_;
}

void OutputStreamCallbackBase::setType(TType type) {
  type_ = type;
}

TType OutputStreamCallbackBase::getType() {
  CHECK(type_ != TType::T_STOP);
  return type_;
}

void OutputStreamCallbackBase::close() {
  sink_->onStreamClose(this);
}

void OutputStreamCallbackBase::markAsClosed() {
  closed_ = true;
}

bool OutputStreamCallbackBase::isClosed() {
  return closed_;
}

bool OutputStreamCallbackBase::isLinked() {
  return manager_ != nullptr &&
         sink_ != nullptr &&
         writer_ != nullptr;
}

OutputStreamCallbackBase::~OutputStreamCallbackBase() {
  CHECK((sendState_ == SendState::INIT) ||
        (sendState_ == SendState::NONE));
  CHECK(closed_);
}

StreamSink::StreamSink(StreamManager* manager,
                       StreamMap&& streamMap,
                       ExceptionSerializer exceptionSerializer)
    : manager_(manager),
      streamMap_(std::move(streamMap)),
      exceptionSerializer_(exceptionSerializer),
      hasOutstandingSend_(true),
      numOpenStreams_(streamMap_.size()),
      numOpenSyncStreams_(0),
      hasError_(false) {
  CHECK_NOTNULL(manager_);
  CHECK_NOTNULL(exceptionSerializer_);

  for (auto& pair : streamMap_) {
    auto paramId = pair.first;
    auto& stream = pair.second;

    stream->link(manager_, this, &streamWriter_, paramId);

    // either all the streams are sync streams or
    // all the streams are async streams
    if (stream->isSync()) {
      ++numOpenSyncStreams_;
    }
  }
}

void StreamSink::setProtocolType(ProtocolType type) {
  streamWriter_.setProtocolType(type);
  // we write the description immediately after we have set the protocol
  // type to ensure that the description will always be read first
  writeDescription();
}

void StreamSink::onCancel() {
  numOpenStreams_ = 0;
  numOpenSyncStreams_ = 0;

  // marking all streams as closed before notifying the steams ensures the
  // onClose() calls cannot call the put() method on any of the streams
  for (auto& pair : streamMap_) {
    pair.second->markAsClosed();
    pair.second->setSendState(SendState::NONE);
  }

  for (auto& pair : streamMap_) {
    pair.second->notifyClose();
  }
}

void StreamSink::writeDescription() {
  streamWriter_.writeNumDescriptions(streamMap_.size());
  for (auto& pair : streamMap_) {
    int16_t paramId = pair.first;
    auto& stream = pair.second;

    streamWriter_.writeDescription(paramId, stream->getType());
  }

  sendDataIfNecessary();
}

void StreamSink::onStreamPut(OutputStreamCallbackBase* stream) {
  CHECK_NOTNULL(stream);
  // if we are in sync mode, then we will only ever put stuff on the
  // stream when the sendState is NONE
  CHECK(!stream->isSync() || (stream->getSendState() == SendState::NONE));

  stream->setSendState(SendState::PENDING);
  sendDataIfNecessary();
}

void StreamSink::onStreamClose(OutputStreamCallbackBase* stream) {
  CHECK_NOTNULL(stream);
  CHECK(!stream->isSync() || (stream->getSendState() == SendState::NONE));

  if (!stream->isClosed()) {
    --numOpenStreams_;
    if (stream->isSync()) {
      --numOpenSyncStreams_;
    }

    stream->markAsClosed();

    streamWriter_.writeFinish(stream->getParamId());

    stream->setSendState(SendState::PENDING);
    sendDataIfNecessary();
  }
}

bool StreamSink::hasOpenStreams() {
  return numOpenStreams_ > 0;
}

bool StreamSink::hasOpenSyncStreams() {
  return numOpenSyncStreams_ > 0;
}

bool StreamSink::hasWrittenEnd() {
  return streamWriter_.hasWrittenEnd();
}

bool StreamSink::hasSentEnd() {
  return hasWrittenEnd() &&
         streamWriter_.getNumBytesWritten() == 0 &&
         !hasOutstandingSend_;
}

bool StreamSink::hasError() {
  return hasError_;
}

void StreamSink::onSend() {
  CHECK(!hasError());

  std::vector<OutputStreamCallbackBase*> sendStreams;
  std::vector<OutputStreamCallbackBase*> closedStreams;

  for (auto& pair : streamMap_) {
    OutputStreamCallbackBase* stream = pair.second.get();

    SendState sendState = stream->getSendState();

    if (sendState == SendState::INIT) {
      stream->setSendState(SendState::NONE);
      sendStreams.push_back(stream);

    } else if (sendState == SendState::SENT) {
      stream->setSendState(SendState::NONE);
      if (stream->isClosed()) {
        closedStreams.push_back(stream);
      } else {
        sendStreams.push_back(stream);
      }
    }
  }

  for (auto stream : closedStreams) {
    stream->notifyClose();
  }

  for (auto stream : sendStreams) {
    stream->notifySend();
  }

  hasOutstandingSend_ = false;
  sendDataIfNecessary();
}

void StreamSink::onStreamException(OutputStreamCallbackBase* stream,
                                   const folly::exception_wrapper& exception) {
  CHECK_NOTNULL(stream);
  CHECK(!stream->isSync() || (stream->getSendState() == SendState::NONE));
  CHECK(exception);

  if (!stream->isClosed()) {
    --numOpenStreams_;
    if (stream->isSync()) {
      --numOpenSyncStreams_;
    }

    stream->markAsClosed();

    streamWriter_.writeException(stream->getParamId());
    // this function will rethrow the exception if the exception cannot
    // be serialized
    exceptionSerializer_(streamWriter_, exception);

    stream->setSendState(SendState::PENDING);
    sendDataIfNecessary();

  } else {
    throw StreamException("Cannot send exception after stream is closed.");
  }
}

void StreamSink::onError(const folly::exception_wrapper& error) {
  hasError_ = true;

  // marking all streams as closed before notifying the steams ensures the
  // onClose() calls cannot call the put() methods on any of the streams
  for (auto& pair : streamMap_) {
    pair.second->setSendState(SendState::NONE);
    pair.second->markAsClosed();
  }

  // XXX should we call onError of even streams that have been close a lont
  // time ago?
  for (auto& pair : streamMap_) {
    pair.second->notifyError(error);
    pair.second->notifyClose();
  }
}

void StreamSink::sendEnd() {
  CHECK(!hasOpenStreams());
  streamWriter_.writeEnd();
  sendDataIfNecessary();
}

void StreamSink::sendDataIfNecessary() {
  if (shouldSendData()) {
    sendData();
  }
}

bool StreamSink::shouldSendData() {
  return !hasError() &&
         !hasOutstandingSend_ &&
         streamWriter_.getNumBytesWritten() > 0;
}

void StreamSink::sendData() {
  manager_->sendData(streamWriter_.extractBuffer());

  for (auto& pair : streamMap_) {
    auto stream = pair.second.get();
    if (stream->getSendState() == SendState::PENDING) {
      stream->setSendState(SendState::SENT);
    }
  }

  hasOutstandingSend_ = true;
}

bool StreamSink::isSendingEnd() {
  CHECK(!hasOutstandingSend_);
  return hasWrittenEnd() && streamWriter_.getNumBytesWritten() == 0;
}

void StreamSink::sendAcknowledgement() {
  streamWriter_.writeAcknowledgement();
  sendDataIfNecessary();
}

void StreamSink::sendClose(int16_t paramId) {
  streamWriter_.writeClose(paramId);
  sendDataIfNecessary();
}

void StreamSink::onStreamCloseFromInput(int16_t paramId) {
  auto stream = getStream(paramId);
  CHECK_NOTNULL(stream);

  if (!stream->isClosed()) {
    --numOpenStreams_;
    if (stream->isSync()) {
      --numOpenSyncStreams_;
    }

    stream->markAsClosed();
    stream->setSendState(SendState::NONE);
    stream->notifyClose();
  }
}

OutputStreamCallbackBase* StreamSink::getStream(int16_t paramId) {
  auto ite = streamMap_.find(paramId);
  if (ite != streamMap_.end()) {
    return ite->second.get();
  } else {
    return nullptr;
  }
}

StreamSink::~StreamSink() {
  CHECK(!hasOpenStreams() || hasError());
}

