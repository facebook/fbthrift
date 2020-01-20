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

namespace apache {
namespace thrift {
namespace async {

class TAsyncFizzClient : public virtual fizz::client::AsyncFizzClient {
 public:
  using UniquePtr =
      std::unique_ptr<TAsyncFizzClient, folly::DelayedDestruction::Destructor>;

  using fizz::client::AsyncFizzClient::AsyncFizzClient;

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
