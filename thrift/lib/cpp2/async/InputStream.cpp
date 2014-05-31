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
#include "thrift/lib/cpp/TApplicationException.h"
#include "thrift/lib/cpp/transport/TTransportException.h"
#include <utility>
#include <unordered_set>

using namespace apache::thrift;
using apache::thrift::protocol::TType;
using apache::thrift::transport::TTransportException;

InputStreamCallbackBase::InputStreamCallbackBase(bool isSync)
    : sink_(nullptr),
      manager_(nullptr),
      source_(nullptr),
      paramId_(0),
      type_(TType::T_STOP),
      isSync_(isSync),
      closed_(false) {
}

StreamManager* InputStreamCallbackBase::getStreamManager() {
  return manager_;
}

bool InputStreamCallbackBase::isSync() {
  return isSync_;
}

void InputStreamCallbackBase::link(StreamSink* sink,
                                   StreamManager* manager,
                                   StreamSource* source,
                                   int16_t paramId) {
  sink_ = sink;
  manager_ = manager;
  source_ = source;
  paramId_ = paramId;
}

void InputStreamCallbackBase::setType(TType type) {
  type_ = type;
}

TType InputStreamCallbackBase::getType() {
  CHECK(type_ != TType::T_STOP);
  return type_;
}

int16_t InputStreamCallbackBase::getParamId() {
  return paramId_;
}

void InputStreamCallbackBase::setBlockedOnItem(bool blockedOnItem) {
  CHECK_NOTNULL(source_);
  source_->setBlockedOnItem(blockedOnItem);
}

void InputStreamCallbackBase::readBuffer() {
  return source_->readBuffer();
}

bool InputStreamCallbackBase::bufferHasUnreadData() {
  return source_->bufferHasUnreadData();
}

void InputStreamCallbackBase::sendAcknowledgement() {
  sink_->sendAcknowledgement();
}

void InputStreamCallbackBase::close() {
  source_->onStreamClose(this);
}

void InputStreamCallbackBase::markAsClosed() {
  closed_ = true;
}

bool InputStreamCallbackBase::isClosed() {
  return closed_;
}

InputStreamCallbackBase::~InputStreamCallbackBase() {
  if (source_ != nullptr) {
    CHECK(isClosed());
  }
}

StreamSource::StreamSource(StreamSink* sink,
                           StreamManager* manager,
                           StreamMap&& streamMap,
                           ExceptionDeserializer exceptionDeserializer)
    : sink_(sink),
      manager_(manager),
      streamMap_(std::move(streamMap)),
      exceptionDeserializer_(exceptionDeserializer),
      isSync_(false),
      numOpenStreams_(0),
      hasReadDescription_(false),
      blockedOnItem_(false),
      hasError_(false) {
  CHECK_NOTNULL(sink_);
  CHECK_NOTNULL(manager_);
  CHECK_NOTNULL(exceptionDeserializer_);

  for (auto& pair : streamMap_) {
    auto paramId = pair.first;
    auto& stream = pair.second;

    stream->link(sink_, manager_, this, paramId);

    // the stream source is sync if any one of the streams is sync
    isSync_ = (isSync_ || stream->isSync());
  }

  // since sync streams are only allowed on the client side as a return value
  // only one sync input stream is allowed
  if (isSync_) {
    CHECK_EQ(streamMap_.size(), 1);
  }
}

void StreamSource::setProtocolType(ProtocolType type) {
  streamReader_.setProtocolType(type);
}

void StreamSource::onCancel() {
  // marking all streams as closed before notifying the steams ensures the
  // onClose() calls cannot call the close() method on any of the streams
  for (auto& pair : streamMap_) {
    pair.second->markAsClosed();
  }

  for (auto& pair : streamMap_) {
    pair.second->notifyFinish();
    pair.second->notifyClose();
  }
}

void StreamSource::setBlockedOnItem(bool blockedOnItem) {
  blockedOnItem_ = blockedOnItem;
}

bool StreamSource::isBlockedOnItem() {
  return blockedOnItem_;
}

bool StreamSource::hasOpenStreams() {
  // when hasReadStreamDescription() == false, then the
  // value for numOpenStreams_ is invalid, so we assume
  // that we have open streams
  return !hasReadStreamDescription() || numOpenStreams_ > 0;
}

bool StreamSource::hasOpenSyncStreams() {
  return (isSync() && hasOpenStreams());
}

bool StreamSource::isSync() {
  return isSync_;
}

bool StreamSource::hasReadStreamDescription() {
  return hasReadDescription_;
}

bool StreamSource::hasReceivedEnd() {
  return streamReader_.hasReadEnd();
}

bool StreamSource::hasError() {
  return hasError_;
}

void StreamSource::onReceive(std::unique_ptr<folly::IOBuf>&& data) {
  CHECK(!hasError());
  CHECK(!hasReceivedEnd());

  streamReader_.insertBuffer(std::move(data));

  if (!isBlockedOnItem()) {
    auto ew = folly::try_and_catch<std::exception, TProtocolException,
        TTransportException, TApplicationException>([&]() {
      readBuffer();
    });
    if (ew) {
      manager_->notifyError(ew);
    }
  } else {
    CHECK(isSync());
    CHECK_EQ(numOpenStreams_, 1);
  }
}

bool StreamSource::bufferHasUnreadData() {
  return streamReader_.hasMoreToRead();
}

void StreamSource::readBuffer() {
  uint32_t oldNumOpenStreams = numOpenStreams_;
  bool anyOutputStreamClosed = false;

  while (bufferHasUnreadData() && !isBlockedOnItem()) {
    CHECK_GE(numOpenStreams_, 0);

    StreamFlag streamFlag = streamReader_.readFlag();
    int16_t paramId;

    switch (streamFlag) {
      case StreamFlag::DESCRIPTION:
        readStreamDescription();
        oldNumOpenStreams = numOpenStreams_;
        break;
      case StreamFlag::ITEM:
        paramId = streamReader_.readId();
        notifyStreamReadItem(paramId);
        break;
      case StreamFlag::ACKNOWLEDGE:
        // we don't do anything right now, but we could send more
        // data like number of items received, and then we can update
        // the count of items received in here
        break;
      case StreamFlag::CLOSE:
        paramId = streamReader_.readId();
        sink_->onStreamCloseFromInput(paramId);
        anyOutputStreamClosed = true;
        break;
      case StreamFlag::FINISH:
        paramId = streamReader_.readId();
        notifyStreamFinished(paramId);
        break;
      case StreamFlag::EXCEPTION:
        paramId = streamReader_.readId();
        notifyStreamReadException(paramId);
        break;
      case StreamFlag::END:
        CHECK(!hasOpenStreams());
        break;
      default:
        using apache::thrift::transport::TTransportException;
        throw TTransportException(TTransportException::CORRUPTED_DATA,
                                  "Unexpected Stream Flag");
    }

    CHECK(hasReadStreamDescription());
  }

  if (!bufferHasUnreadData()) {
    streamReader_.discardBuffer();
  }

  bool anyStreamClosed = (oldNumOpenStreams > numOpenStreams_) ||
                         anyOutputStreamClosed;

  if ((anyStreamClosed || hasReceivedEnd())
      && manager_->needToSendEnd()) {
      sink_->sendEnd();
  }
}

void StreamSource::onError(const folly::exception_wrapper& error) {
  hasError_ = true;

  // marking all streams as closed before notifying the steams ensures the
  // onClose() calls cannot call the close() method on any of the streams
  for (auto& pair : streamMap_) {
    pair.second->markAsClosed();
  }

  // XXX should we call onError of even streams that have been close a long
  // time ago?
  for (auto& pair : streamMap_) {
    pair.second->notifyError(error);
    pair.second->notifyClose();
  }
}

void StreamSource::onStreamClose(InputStreamCallbackBase* stream) {
  CHECK_NOTNULL(stream);

  if (!stream->isClosed()) {
    stream->markAsClosed();
    if (hasReadStreamDescription()) {
      --numOpenStreams_;
      stream->notifyClose();
      sink_->sendClose(stream->getParamId());
    }
  }
}

void StreamSource::readStreamDescription() {
  CHECK(!hasReadStreamDescription());
  CHECK(numOpenStreams_ == 0);
  CHECK(unexpectedStreamMap_.empty());

  std::unordered_set<int16_t> notDescribedStreams;
  for (auto& pair : streamMap_) {
    auto inserted = notDescribedStreams.insert(pair.first);
    CHECK(inserted.second);
  }

  uint32_t numDescriptions = streamReader_.readNumDescriptions();
  for (uint32_t i = 0; i < numDescriptions; ++i) {
    int16_t paramId;
    TType type;
    streamReader_.readDescription(paramId, type);

    auto ite = streamMap_.find(paramId);
    if (ite != streamMap_.end() && type == ite->second->getType()) {
      // we have a stream that matches with the other stream

      // In sync mode it is possible to have closed the streams first, and
      // then run the event base loop to reach this function. So here we
      // check to see if the stream has already been closed first.
      auto& stream = ite->second;
      if (!stream->isClosed()) {
        ++numOpenStreams_;
      } else {
        stream->notifyClose();
        sink_->sendClose(paramId);
      }

      uint32_t numErased = notDescribedStreams.erase(paramId);
      if (numErased == 0) {
        VLOG(5) << "The same stream was described multiple times.";
      }

    } else {
      // the stream id and type of the other stream does not match with any
      // stream that we have

      sink_->sendClose(paramId);

      // even though we have closed the incoming streams, we still need to store
      // their types in case we need to skip over items from those streams
      auto ins = unexpectedStreamMap_.insert(std::make_pair(paramId, type));
      if (!ins.second) {
        VLOG(5) << "An unexpected stream was described multiple times.";
      }
    }
  }

  // notDescribedStreams contains the ids of the input streams that we have
  // prepared for, but did not receive a description for from the other side
  for (auto& paramId : notDescribedStreams) {
    auto ite = streamMap_.find(paramId);
    CHECK(ite != streamMap_.end());

    auto& stream = ite->second;

    if (!stream->isClosed()) {
      stream->markAsClosed();
      stream->notifyFinish();
    }

    stream->notifyClose();

    // we do not delete the callback, because there might be an error that
    // occurs and we need to call the onError callback for the stream
  }

  if (isSync()) {
    // Since we an only have sync streams on the client side and there can be
    // at most one return stream, there should have at most one stream in sync
    // mode.
    CHECK_LE(numOpenStreams_, 1);
  }

  hasReadDescription_ = true;
}

void StreamSource::notifyStreamReadItem(int16_t paramId) {
  auto stream = getStream(paramId);
  if (stream) {
    if (!stream->isClosed()) {
      stream->readItem(streamReader_);
    } else {
      TType type = stream->getType();
      streamReader_.skipItem(type);
    }
  } else {
    TType type = getUnexpectedStreamType(paramId);
    streamReader_.skipItem(type);
  }
}

void StreamSource::notifyStreamFinished(int16_t paramId) {
  auto stream = getStream(paramId);
  if ((stream != nullptr) && !stream->isClosed()) {
    --numOpenStreams_;
    stream->markAsClosed();
    stream->notifyFinish();
    stream->notifyClose();
  }
}

void StreamSource::notifyStreamReadException(int16_t paramId) {
  auto stream = getStream(paramId);
  if ((stream != nullptr) && !stream->isClosed()) {
    --numOpenStreams_;
    stream->markAsClosed();
    stream->notifyException(exceptionDeserializer_(streamReader_));
    stream->notifyClose();
  } else {
    streamReader_.skipException();
  }
}

InputStreamCallbackBase* StreamSource::getStream(int16_t paramId) {
  auto ite = streamMap_.find(paramId);
  if (ite != streamMap_.end()) {
    return ite->second.get();
  } else {
    return nullptr;
  }
}

TType StreamSource::getUnexpectedStreamType(int16_t paramId) {
  auto ite = unexpectedStreamMap_.find(paramId);
  if (ite != unexpectedStreamMap_.end()) {
    return ite->second;
  } else {
    using apache::thrift::transport::TTransportException;
    throw TTransportException(TTransportException::CORRUPTED_DATA,
                              "Unexpected Stream Flag");
  }
}

StreamSource::~StreamSource() {
  // !hasReadStreamDescription() if it was cancalled
  // !hasOpenStreams() if completed successfully
  // hasError() if has an error
  CHECK(!hasReadStreamDescription() || !hasOpenStreams() || hasError());
}

