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

#ifndef _THRIFT_TRANSPORT_HDFS_H
#define _THRIFT_TRANSPORT_HDFS_H

#include <string>
#include <memory>

/**
 * Dead-simple wrappers around hdfs and hdfsFile descriptors.
 * The wrappers take responsibility of descriptor allocation/release in ctor/dtor.
 *
 * @author Li Zhang <lzhang@facebook.com>
 */

class HDFS {
 public:
  HDFS();
  HDFS(const std::string& host, uint16_t port);
  bool disconnect();
  ~HDFS();
  void* getHandle() const {
    return hdfs_;
  }
  bool isConnected() const {
    return hdfs_ != nullptr;
  }
 protected:
  void* hdfs_;
};

class HDFSFile {
 public:
  enum AccessPolicy {
    OPEN_FOR_READ = 0,
    CREATE_FOR_WRITE = 1,
    OPEN_FOR_APPEND = 2,
  };
  HDFSFile(std::shared_ptr<HDFS> hdfs, const std::string& path, AccessPolicy ap,
            int bufferSize = 0, short replication = 0, int32_t blocksize = 0);
  bool close();
  void* getHandle() const {
    return file_;
  }
  std::shared_ptr<HDFS> getFS() const {
    return hdfs_;
  }
  bool isOpen() const {
    return file_ != nullptr;
  }
  ~HDFSFile();
 protected:
  std::shared_ptr<HDFS> hdfs_;
  void* file_;
};

#endif
