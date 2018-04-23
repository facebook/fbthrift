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

#include <thrift/lib/cpp/transport/HDFS.h>

#include <hdfs.h>
#include <thrift/lib/cpp/transport/TTransportException.h>

using namespace std;
using std::shared_ptr;
using apache::thrift::transport::TTransportException;

HDFS::HDFS(const std::string& host, uint16_t port) {
  if (host=="localfile") {
    hdfs_ = hdfsConnect(nullptr, 0);
  } else {
    hdfs_ = hdfsConnect(host.c_str(), port);
  }
  if (hdfs_ == nullptr) {
    throw TTransportException(TTransportException::UNKNOWN,
            "Error in hdfsConnect()", errno);
  }
}

bool HDFS::disconnect() {
  int ret = 0;
  if (hdfs_ != nullptr) {
    ret = hdfsDisconnect(hdfs_);
    if (ret != 0) {
      throw TTransportException(TTransportException::UNKNOWN,
                                "Error in hdfsDisconnect()", errno);
    }
    hdfs_ = nullptr;
  }
  return ret == 0;
}

HDFS::~HDFS() {
  disconnect();
}

HDFSFile::HDFSFile(std::shared_ptr<HDFS> hdfs,
                   const std::string& path,
                   AccessPolicy ap,
                   int bufferSize, short replication, int32_t blocksize) {
  int flags;
  switch (ap) {
    case OPEN_FOR_READ:
      flags = O_RDONLY;
      break;
    case CREATE_FOR_WRITE:
      flags = O_WRONLY;// | O_CREAT;
      break;
    case OPEN_FOR_APPEND:
      flags = O_WRONLY|O_APPEND;
      break;
    default:
    file_ = nullptr;
    throw TTransportException(TTransportException::BAD_ARGS,
                              "Invalid HDFSFile::AccessPolicy");
    return;
  }
  hdfs_ = hdfs;
  file_ = hdfsOpenFile(hdfs_->getHandle(), path.c_str(), flags,
                       bufferSize, replication, blocksize);
  if (file_ == nullptr) {
    throw TTransportException(TTransportException::UNKNOWN,
            "Error in hdfsOpenFile()", errno);
  }
}

bool HDFSFile::close() {
  int rv = 0;
  if (file_ != nullptr) {
    rv = hdfsCloseFile(hdfs_->getHandle(), (hdfsFile)file_);
    if (rv != 0) {
      throw TTransportException(TTransportException::UNKNOWN,
                                "Error closing HDFS file.",errno);
    }
    file_ = nullptr;
  }
  return rv == 0;
}

HDFSFile::~HDFSFile() {
  close();
}
