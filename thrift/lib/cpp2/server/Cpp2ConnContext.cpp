/*
 * Copyright 2004-present Facebook, Inc.
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
#include "thrift/lib/cpp2/server/Cpp2ConnContext.h"

#ifdef __APPLE__
#include <sys/ucred.h> // @manual
#endif

namespace apache {
namespace thrift {

Cpp2ConnContext::PeerCred Cpp2ConnContext::PeerCred::queryFromSocket(
    folly::NetworkSocket socket) {
#if defined(SO_PEERCRED) // Linux
  struct ucred cred = {};
  socklen_t len = sizeof(cred);
  if (getsockopt(socket.toFd(), SOL_SOCKET, SO_PEERCRED, &cred, &len)) {
    return PeerCred{ErrorRetrieving};
  } else {
    return PeerCred{cred.pid, cred.uid};
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
    return PeerCred{ErrorRetrieving};
  } else if (getsockopt(
                 socket.toFd(),
                 SOL_LOCAL,
                 LOCAL_PEEREPID,
                 &epid,
                 &(len = sizeof(epid)))) {
    return PeerCred{ErrorRetrieving};
  } else {
    return PeerCred{epid, cred.cr_uid};
  }
#else
  (void)socket;
  return PeerCred{};
#endif
}

} // namespace thrift
} // namespace apache
