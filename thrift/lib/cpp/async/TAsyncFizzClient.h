/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <fizz/client/AsyncFizzClient.h>
#include <thrift/lib/cpp/async/TAsyncTransport.h>

namespace apache {
namespace thrift {
namespace async {

class TAsyncFizzClient : public virtual fizz::client::AsyncFizzClient,
                         public TAsyncTransport {
 public:
  using UniquePtr =
      std::unique_ptr<TAsyncFizzClient, folly::DelayedDestruction::Destructor>;
  TAsyncFizzClient(
      folly::AsyncTransportWrapper::UniquePtr sock,
      std::shared_ptr<const fizz::client::FizzClientContext> ctx,
      const std::shared_ptr<fizz::ClientExtensions>& ext = nullptr)
      : fizz::client::AsyncFizzClient(std::move(sock), std::move(ctx), ext) {}

  TAsyncFizzClient(
      folly::EventBase* evb,
      std::shared_ptr<const fizz::client::FizzClientContext> ctx,
      const std::shared_ptr<fizz::ClientExtensions>& ext = nullptr)
      : fizz::client::AsyncFizzClient(evb, std::move(ctx), ext) {}

  void setReadCB(AsyncTransportWrapper::ReadCallback* callback) override {
    fizz::client::AsyncFizzClient::setReadCB(callback);
  }

  folly::AsyncTransportWrapper::ReadCallback* getReadCallback() const override {
    return fizz::client::AsyncFizzClient::getReadCallback();
  }

  void write(
      AsyncTransportWrapper::WriteCallback* callback,
      const void* buf,
      size_t bytes,
      folly::WriteFlags flags = folly::WriteFlags::NONE) override {
    fizz::client::AsyncFizzClient::write(callback, buf, bytes, flags);
  }
  void writev(
      AsyncTransportWrapper::WriteCallback* callback,
      const iovec* vec,
      size_t count,
      folly::WriteFlags flags = folly::WriteFlags::NONE) override {
    fizz::client::AsyncFizzClient::writev(callback, vec, count, flags);
  }
  void writeChain(
      AsyncTransportWrapper::WriteCallback* callback,
      std::unique_ptr<folly::IOBuf>&& buf,
      folly::WriteFlags flags = folly::WriteFlags::NONE) override {
    fizz::client::AsyncFizzClient::writeChain(callback, std::move(buf), flags);
  }
  const AsyncTransportWrapper* getWrappedTransport() const override {
    return fizz::client::AsyncFizzClient::getWrappedTransport();
  }

  void setPeerCertificate(
      std::unique_ptr<const folly::AsyncTransportCertificate> cert) {
    peerCertData_ = std::move(cert);
  }

  const folly::AsyncTransportCertificate* getPeerCertificate() const override {
    return peerCertData_ ? peerCertData_.get()
                         : fizz::client::AsyncFizzClient::getPeerCertificate();
  }

 private:
  std::unique_ptr<const folly::AsyncTransportCertificate> peerCertData_{
      nullptr};
};
} // namespace async
} // namespace thrift
} // namespace apache
