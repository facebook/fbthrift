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
#include "thrift/lib/cpp/test/SocketPair.h"

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "thrift/lib/cpp/transport/TTransportException.h"

using apache::thrift::transport::TTransportException;

namespace apache { namespace thrift { namespace test {

SocketPair::SocketPair(Mode mode) {
  if (socketpair(PF_UNIX, SOCK_STREAM, 0, fds_) != 0) {
    throw TTransportException(TTransportException::INTERNAL_ERROR,
                              "test::SocketPair: failed create socket pair",
                              errno);
  }

  if (mode == NONBLOCKING) {
    if (fcntl(fds_[0], F_SETFL, O_NONBLOCK) != 0) {
      throw TTransportException(TTransportException::INTERNAL_ERROR,
                                "test::SocketPair: failed to set non-blocking "
                                "read mode", errno);
    }
    if (fcntl(fds_[1], F_SETFL, O_NONBLOCK) != 0) {
      throw TTransportException(TTransportException::INTERNAL_ERROR,
                                "test::SocketPair: failed to set non-blocking "
                                "write mode", errno);
    }
  }
}

SocketPair::~SocketPair() {
  closeFD0();
  closeFD1();
}

void SocketPair::closeFD0() {
  if (fds_[0] >= 0) {
    close(fds_[0]);
    fds_[0] = -1;
  }
}

void SocketPair::closeFD1() {
  if (fds_[1] >= 0) {
    close(fds_[1]);
    fds_[1] = -1;
  }
}

}}} // apache::thrift::test
