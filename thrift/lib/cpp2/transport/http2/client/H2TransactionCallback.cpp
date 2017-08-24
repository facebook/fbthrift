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

#include <thrift/lib/cpp2/transport/http2/client/H2TransactionCallback.h>

#include <glog/logging.h>

namespace apache {
namespace thrift {

H2TransactionCallback::~H2TransactionCallback() {
  if (txn_) {
    txn_->setHandler(nullptr);
    txn_->setTransportCallback(nullptr);
  }
}

void H2TransactionCallback::setChannel(std::shared_ptr<H2ChannelIf> channel) {
  channel_ = channel;
}

// proxygen::HTTPTransactionHandler methods

void H2TransactionCallback::setTransaction(
    proxygen::HTTPTransaction* txn) noexcept {
  txn_ = txn;
  // If Transaction is created through HTTPSession::newTransaction,
  // handler is already set, thus no need for txn_->setHandler(this);
  txn_->setTransportCallback(this);
}

void H2TransactionCallback::detachTransaction() noexcept {
  VLOG(5) << "HTTPTransaction on memory " << this << " is detached.";
  delete this;
}

void H2TransactionCallback::onHeadersComplete(
    std::unique_ptr<proxygen::HTTPMessage> msg) noexcept {
  DCHECK(channel_);
  channel_->onH2StreamBegin(std::move(msg));
}

void H2TransactionCallback::onBody(
    std::unique_ptr<folly::IOBuf> body) noexcept {
  DCHECK(channel_);
  channel_->onH2BodyFrame(std::move(body));
}

void H2TransactionCallback::onEOM() noexcept {
  DCHECK(channel_);
  channel_->onH2StreamEnd();
}

void H2TransactionCallback::onError(
    const proxygen::HTTPException& /*error*/) noexcept {
  // TODO
}

// end proxygen::HTTPTransactionHandler methods

// proxygen::HTTPTransaction::TransportCallback methods

void H2TransactionCallback::lastByteFlushed() noexcept {
  // TODO: Does this belong here or in channel?
}

// end proxygen::HTTPTransaction::TransportCallback methods

} // namespace thrift
} // namespace apache
