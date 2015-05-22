/*
 * Copyright 2014 Facebook, Inc.
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

#ifndef _THRIFT_TRANSPORT_TCALLBACKTRANSPORT_H_
#define _THRIFT_TRANSPORT_TCALLBACKTRANSPORT_H_ 1

#include <thrift/lib/cpp/transport/TTransport.h>
#include <thrift/lib/cpp/transport/TVirtualTransport.h>

namespace apache { namespace thrift { namespace transport {

typedef uint32_t (*WriteCallback)(void* userdata, const uint8_t* buf, uint32_t len);
typedef uint32_t (*ReadCallback)(void* userdata, uint8_t* buf, uint32_t len);

/**
 * Call some callbacks to do transport
 *
 */
class TCallbackTransport : public TVirtualTransport<TCallbackTransport> {
 public:

  TCallbackTransport() {}

  ~TCallbackTransport() override {}

  bool isOpen() override { return true; }

  void open() override {}

  void close() override {}

  uint32_t read(uint8_t* buf, uint32_t len);

  void write(const uint8_t* buf, uint32_t len);

  void setWriteCallback(WriteCallback callback, void* userdata) {
      write_callback = callback;
      userData = userdata;
  }
  void setReadCallback(ReadCallback callback, void* userdata) {
      read_callback = callback;
      userData = userdata;
  }

 protected:
  void* userData;
  WriteCallback write_callback;
  ReadCallback read_callback;
};

}}} // apache::thrift::transport

#endif // #ifndef _THRIFT_TRANSPORT_TCALLBACKTRANSPORT_H_
