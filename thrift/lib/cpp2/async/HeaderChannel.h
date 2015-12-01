/*
 * Copyright 2015 Facebook, Inc.
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
#pragma once

#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>

namespace apache {
namespace thrift {

/**
 * HeaderChannel manages persistent headers.
 */
class HeaderChannel {
 public:
  HeaderChannel() {}

  void addRpcOptionHeaders(apache::thrift::transport::THeader* header,
                           RpcOptions& rpcOptions);

  void setPersistentHeader(const std::string& key, const std::string& value) {
    persistentWriteHeaders_[key] = value;
  }

  transport::THeader::StringToStringMap& getPersistentReadHeaders() {
    return persistentReadHeaders_;
  }

  transport::THeader::StringToStringMap& getPersistentWriteHeaders() {
    return persistentWriteHeaders_;
  }

 protected:
  virtual ~HeaderChannel() {}

  virtual bool clientSupportHeader() { return true; }

 private:
  // Map to use for persistent headers
  transport::THeader::StringToStringMap persistentReadHeaders_;
  transport::THeader::StringToStringMap persistentWriteHeaders_;
};
}
} // apache::thrift
