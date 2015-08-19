/*
 * Copyright 2014 Facebook, Inc.
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

#include <thrift/lib/cpp/transport/THeaderTransport.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <folly/io/IOBuf.h>

#include <algorithm>
#include <bitset>
#include <cassert>
#include <string>

using std::map;
using std::shared_ptr;
using std::string;
using std::unique_ptr;
using std::vector;
using std::pair;

namespace apache { namespace thrift { namespace transport {

using namespace apache::thrift::protocol;
using namespace folly;
using apache::thrift::protocol::TBinaryProtocol;

void THeaderTransport::resetProtocol() {
  THeader::resetProtocol();

  // Read the header and decide which protocol to go with
  readFrame(0);
}

void THeaderTransport::checkSupportedClient(CLIENT_TYPE ct) {
  if (!isSupportedClient(ct)) {
    throw TApplicationException(
        TApplicationException::UNSUPPORTED_CLIENT_TYPE,
        "Transport does not support this client type");
  }
}

void THeaderTransport::setClientType(CLIENT_TYPE ct) {
  checkSupportedClient(ct);
  setClientTypeNoCheck(ct);
}

void THeaderTransport::setSupportedClients(
    const std::bitset<CLIENT_TYPES_LEN>* ct) {
  if (!ct || ct->none()) {
    std::bitset<CLIENT_TYPES_LEN> clients;

    clients[THRIFT_UNFRAMED_DEPRECATED] = true;
    clients[THRIFT_FRAMED_DEPRECATED] = true;
    clients[THRIFT_HTTP_SERVER_TYPE] = true;
    clients[THRIFT_HTTP_CLIENT_TYPE] = true;
    clients[THRIFT_HEADER_CLIENT_TYPE] = true;
    clients[THRIFT_FRAMED_COMPACT] = true;

    supported_clients = clients;
  } else if (ct->test(THRIFT_HEADER_SASL_CLIENT_TYPE)) {
    throw TApplicationException(
        TApplicationException::UNSUPPORTED_CLIENT_TYPE,
        "Thrift 1 does not support SASL client type");
  } else {
    supported_clients = *ct;
  }
}

uint32_t THeaderTransport::readAll(uint8_t* buf, uint32_t len) {
  if (clientType == THRIFT_HTTP_SERVER_TYPE) {
    return httpTransport_->read(buf, len);
  }

  // We want to call TBufferBase's version here, because
  // TFramedTransport would try and call its own readFrame function
  return TBufferBase::readAll(buf, len);
}

uint32_t THeaderTransport::readSlow(uint8_t* buf, uint32_t len) {

  if (clientType == THRIFT_HTTP_SERVER_TYPE) {
    return httpTransport_->read(buf, len);
  }

  if ((clientType == THRIFT_UNFRAMED_DEPRECATED) ||
      (clientType == THRIFT_FRAMED_COMPACT)) {
    return transport_->read(buf, len);
  }

  return TFramedTransport::readSlow(buf, len);
}

void THeaderTransport::allocateReadBuffer(uint32_t sz) {
  if (sz > rBufSize_) {
    rBuf_.reset(new uint8_t[sz]);
    rBufSize_ = sz;
  }
}

bool THeaderTransport::readFrame(uint32_t minFrameSize) {
  const size_t allocSize = 200; // Pick a useful size > 4.

  pair<void*, uint32_t> framing = queue_->preallocate(4, allocSize);
  uint8_t frameSize = 0;

  while (frameSize < 4) {
    uint8_t* szp = (uint8_t*)framing.first + frameSize;
    uint32_t bytes_read = transport_->read(szp, 4 - frameSize);
    if (bytes_read == 0) {
      if (frameSize == 0) {
        // EOF before any data was read.
        return false;
      } else {
        // EOF after a partial frame header.  Raise an exception.
        throw TTransportException(TTransportException::END_OF_FILE,
                                  "No more data to read after "
                                  "partial frame header.");
      }
    }
    frameSize += bytes_read;
  }

  queue_->postallocate(frameSize);
  size_t needed = 0;
  readBuf_ = nullptr;

  while (true) {
    readBuf_ = removeHeader(queue_.get(), needed, persistentReadHeaders_);
    checkSupportedClient(getClientType());
    if (!readBuf_) {
      pair<void*, uint32_t> data = queue_->preallocate(needed,
                                                      needed);
      transport_->readAll((uint8_t*)data.first, needed);
      queue_->postallocate(needed);
    } else {
      break;
    }
  }

  if (clientType == THRIFT_HTTP_SERVER_TYPE) {
    // TODO: Update to use THttpParser directly instead of wrapping
    // in a THttpTransport.
    readBuf_->coalesce();
    shared_ptr<TBufferedTransport> bufferedTrans(
      new TBufferedTransport(transport_));
    bufferedTrans->putBack(readBuf_->writableData(), readBuf_->length());
    httpTransport_ = shared_ptr<TTransport>(new THttpServer(bufferedTrans));
  } else if (readBuf_) {
    readBuf_->coalesce(); // Necessary for backwards compatibility
    setReadBuffer(readBuf_->writableData(), readBuf_->length());
  }

  return true;
}

shared_ptr<TTransport> THeaderTransport::getUnderlyingInputTransport() {
  return transport_;
}

shared_ptr<TTransport> THeaderTransport::getUnderlyingOutputTransport() {
  if (clientType == THRIFT_HTTP_SERVER_TYPE) {
    return httpTransport_;
  } else {
    return outTransport_;
  }
}

void THeaderTransport::flushUnderlyingTransport(bool oneway) {
  if (oneway) {
    getUnderlyingOutputTransport()->onewayFlush();
  } else {
    getUnderlyingOutputTransport()->flush();
  }
  shrinkWriteBuffer();
}

void THeaderTransport::flush()  {
  flushImpl(false);
}

void THeaderTransport::onewayFlush()  {
  flushImpl(true);
}

void THeaderTransport::flushImpl(bool oneway)  {
  ptrdiff_t writableBytes = wBase_ - wBuf_.get();

  if (writableBytes < 0) {
    throw TTransportException(TTransportException::INVALID_STATE,
                              "Negative number of writable bytes");
  }

  if (writableBytes == 0) {
    flushUnderlyingTransport(oneway);
    return;
  }

  // Wrap the old interface to the new addHeader interface.
  unique_ptr<IOBuf> buf = IOBuf::wrapBuffer(wBuf_.get(),
                                            wBase_ - wBuf_.get());

  // Note that we reset wBase_ prior to the underlying write
  // to ensure we're in a sane state (i.e. internal buffer cleaned)
  // if the underlying write throws up an exception
  wBase_ = wBuf_.get();

  buf = addHeader(std::move(buf), persistentWriteHeaders_);

  // And then write back to underlying transport.

  do {
    getUnderlyingOutputTransport()->write(buf->data(), buf->length());
  } while (nullptr != (buf = buf->pop()));

  flushUnderlyingTransport(oneway);
}

}}} // apache::thrift::transport
