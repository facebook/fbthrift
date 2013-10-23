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

#ifndef _THRIFT_TRANSPORT_THDFSFILETRANSPORT_H_
#define _THRIFT_TRANSPORT_THDFSFILETRANSPORT_H_

#include "thrift/lib/cpp/transport/TTransport.h"
#include "thrift/lib/cpp/transport/TVirtualTransport.h"
#include "HDFS.h"
#include <string>

#include <boost/scoped_ptr.hpp>
#include <memory>

namespace apache { namespace thrift { namespace transport {
/**
 * Dead-simple wrapper around libhdfs.
 * THDFSFileTransport only takes care of read/write,
 * and leaves allocation/release to HDFS and HDFSFile.
 * @author Li Zhang <lzhang@facebook.com>
 */
class THDFSFileTransport : public TVirtualTransport<THDFSFileTransport> {
 public:

  THDFSFileTransport(std::shared_ptr<HDFSFile> hdfsFile) : hdfsFile_(hdfsFile) {
  }

  ~THDFSFileTransport() {
  }

  bool isOpen() {
    return hdfsFile_->isOpen();
  }

  void open();

  void close();

  uint32_t read(uint8_t* buf, uint32_t len);

  void write(const uint8_t* buf, uint32_t len);

 protected:
  std::shared_ptr<HDFSFile> hdfsFile_;
};

}}} // apache::thrift::transport

#endif //  _THRIFT_TRANSPORT_THDFSFILETRANSPORT_H_
