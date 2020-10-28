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

#include <thrift/lib/cpp2/security/SSLUtil.h>
#include <folly/io/async/ssl/BasicTransportCertificate.h>

namespace apache {
namespace thrift {

folly::AsyncSocket::UniquePtr moveToPlaintext(folly::AsyncSocket* sock) {
  // Grab certs + fd from sock
  auto selfCert =
      folly::ssl::BasicTransportCertificate::create(sock->getSelfCertificate());
  auto peerCert =
      folly::ssl::BasicTransportCertificate::create(sock->getPeerCertificate());
  auto eb = sock->getEventBase();
  auto fd = sock->detachNetworkSocket();
  auto zcId = sock->getZeroCopyBufId();

  // create new socket
  auto plaintextTransport =
      folly::AsyncSocket::UniquePtr(new folly::AsyncSocket(eb, fd, zcId));
  plaintextTransport->setSelfCertificate(std::move(selfCert));
  plaintextTransport->setPeerCertificate(std::move(peerCert));
  return plaintextTransport;
}

} // namespace thrift
} // namespace apache
