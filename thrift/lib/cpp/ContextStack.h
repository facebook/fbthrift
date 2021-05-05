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

#pragma once

#include <folly/ExceptionWrapper.h>

#include <thrift/lib/cpp/SerializedMessage.h>
#include <thrift/lib/cpp/TProcessorEventHandler.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp/transport/THeader.h>

namespace apache {
namespace thrift {

class ContextStack {
  friend class EventHandlerBase;

 public:
  // Note: factory functions return nullptr if handlers is nullptr or empty.
  static std::unique_ptr<ContextStack> create(
      const std::shared_ptr<
          std::vector<std::shared_ptr<TProcessorEventHandler>>>& handlers,
      const char* method,
      TConnectionContext* connectionContext);

  static std::unique_ptr<ContextStack> create(
      const std::shared_ptr<
          std::vector<std::shared_ptr<TProcessorEventHandler>>>& handlers,
      const char* serviceName,
      const char* method,
      TConnectionContext* connectionContext);

  ContextStack(ContextStack&&) = delete;
  ContextStack& operator=(ContextStack&&) = delete;
  ContextStack(const ContextStack&) = delete;
  ContextStack& operator=(const ContextStack&) = delete;

  ~ContextStack();

  void preWrite();

  void onWriteData(const SerializedMessage& msg);

  void postWrite(uint32_t bytes);

  void preRead();

  void onReadData(const SerializedMessage& msg);

  void postRead(apache::thrift::transport::THeader* header, uint32_t bytes);

  void handlerErrorWrapped(const folly::exception_wrapper& ew);
  void userExceptionWrapped(bool declared, const folly::exception_wrapper& ew);

 private:
  std::shared_ptr<std::vector<std::shared_ptr<TProcessorEventHandler>>>
      handlers_;
  const char* const serviceName_;
  const char* const method_;

  friend struct std::default_delete<apache::thrift::ContextStack>;

  ContextStack(
      const std::shared_ptr<
          std::vector<std::shared_ptr<TProcessorEventHandler>>>& handlers,
      const char* method,
      TConnectionContext* connectionContext);

  ContextStack(
      const std::shared_ptr<
          std::vector<std::shared_ptr<TProcessorEventHandler>>>& handlers,
      const char* serviceName,
      const char* method,
      TConnectionContext* connectionContext);

  void*& contextAt(size_t i) {
    void** start = reinterpret_cast<void**>(this + 1);
    return start[i];
  }
};

} // namespace thrift
} // namespace apache

namespace std {
template <>
struct default_delete<apache::thrift::ContextStack> {
  void operator()(apache::thrift::ContextStack* cs) const {
    if (cs) {
      const size_t nbytes = sizeof(apache::thrift::ContextStack) +
          cs->handlers_->size() * sizeof(void*);
      cs->~ContextStack();
      operator delete (
          cs, nbytes, std::align_val_t{alignof(apache::thrift::ContextStack)});
    }
  }
};
} // namespace std
