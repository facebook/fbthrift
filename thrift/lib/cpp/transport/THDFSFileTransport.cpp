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
#include <hdfs.h>
#include "THDFSFileTransport.h"

using namespace std;
using std::shared_ptr;

namespace apache { namespace thrift { namespace transport {

void THDFSFileTransport::open() {
  // THDFSFileTransport does not handle resource alloc/release.
  // An open HDFSFile handle must exist by now
  if (!isOpen()) {
    throw TTransportException(TTransportException::NOT_OPEN,
                              "THDFSFileTransport::open()");
  }
}

void THDFSFileTransport::close() {
  // THDFSFileTransport does not handle resource alloc/release
  return;
}

uint32_t THDFSFileTransport::read(uint8_t* buf, uint32_t len) {
  tSize rv = hdfsRead(hdfsFile_->getFS()->getHandle(), (hdfsFile)hdfsFile_->getHandle(), buf, len);
  if (rv < 0) {
    int errno_copy = errno;
    throw TTransportException(TTransportException::UNKNOWN,
                              "THDFSFileTransport::read()",
                              errno_copy);
  } else if (rv == 0) {
    throw TTransportException(TTransportException::END_OF_FILE,
                              "THDFSFileTransport::read()");
  }
  return rv;
}

void THDFSFileTransport::write(const uint8_t* buf, uint32_t len) {
  tSize rv = hdfsWrite(hdfsFile_->getFS()->getHandle(), (hdfsFile)hdfsFile_->getHandle(), buf, len);
  if (rv < 0) {
    int errno_copy = errno;
    throw TTransportException(TTransportException::UNKNOWN,
                              "THDFSFileTransport::write()",
                              errno_copy);
  } else if (rv != len) {
      throw TTransportException(TTransportException::INTERRUPTED,
                                "THDFSFileTransport::write()");
  }
}

}}} // apache::thrift::transport
