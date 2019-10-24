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

#include "thrift/lib/cpp2/server/Cpp2ConnContext.h"

#include <folly/String.h>

#ifdef __APPLE__
#include <sys/ucred.h> // @manual
#endif

namespace {

#ifndef _WIN32
uid_t errnoToUid(int no) {
  static_assert(
      sizeof(int) <= sizeof(uid_t), "We want to stash errno in a uid_t");
  if (no < 0) {
    // Make up an invalid errno value - negative shouldn't happen.
    no = std::numeric_limits<int>::max();
  }
  return static_cast<uid_t>(no);
}
#endif

} // namespace

namespace apache {
namespace thrift {

Cpp2ConnContext::PeerCred Cpp2ConnContext::PeerCred::queryFromSocket(
    folly::NetworkSocket socket) {
#if defined(SO_PEERCRED) // Linux
  struct ucred cred = {};
  socklen_t len = sizeof(cred);
  if (getsockopt(socket.toFd(), SOL_SOCKET, SO_PEERCRED, &cred, &len)) {
    return PeerCred{ErrorRetrieving, errnoToUid(errno), 0};
  } else {
    return PeerCred{cred.pid, cred.uid, cred.gid};
  }
#elif defined(LOCAL_PEERCRED) // macOS
  struct xucred cred = {};
  pid_t epid = 0;
  socklen_t len;
  if (getsockopt(
          socket.toFd(),
          SOL_LOCAL,
          LOCAL_PEERCRED,
          &cred,
          &(len = sizeof(cred)))) {
    return PeerCred{ErrorRetrieving, errnoToUid(errno), 0};
  } else if (getsockopt(
                 socket.toFd(),
                 SOL_LOCAL,
                 LOCAL_PEEREPID,
                 &epid,
                 &(len = sizeof(epid)))) {
    return PeerCred{ErrorRetrieving, errnoToUid(errno), 0};
  } else {
    return PeerCred{epid, cred.cr_uid, cred.cr_gid};
  }
#else
  (void)socket;
  return PeerCred{UnsupportedPlatform};
#endif
}

folly::Optional<std::string> Cpp2ConnContext::PeerCred::getError() const {
  if (UnsupportedPlatform == pid_) {
    return folly::make_optional<std::string>("unsupported platform");
#ifndef _WIN32
  } else if (ErrorRetrieving == pid_) {
    return folly::to<std::string>("getsockopt failed: ", folly::errnoStr(uid_));
#endif
  } else {
    return folly::none;
  }
}

} // namespace thrift
} // namespace apache
