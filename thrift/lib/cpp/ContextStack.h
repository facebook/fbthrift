/*
 * Copyright 2018-present Facebook, Inc.
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
  explicit ContextStack(const char* method)
      : ctxs_(), handlers_(), method_(method) {}

  ContextStack(
      const std::shared_ptr<
          std::vector<std::shared_ptr<TProcessorEventHandler>>>& handlers,
      const char* serviceName,
      const char* method,
      TConnectionContext* connectionContext)
      : ctxs_(),
        handlers_(handlers),
        serviceName_(serviceName),
        method_(method) {
    ctxs_.reserve(handlers->size());
    for (auto handler : *handlers) {
      ctxs_.push_back(
          handler->getServiceContext(serviceName, method, connectionContext));
    }
  }

  ContextStack(
      const std::shared_ptr<
          std::vector<std::shared_ptr<TProcessorEventHandler>>>& handlers,
      const char* method,
      TConnectionContext* connectionContext)
      : ctxs_(), handlers_(handlers), method_(method) {
    for (auto handler : *handlers) {
      ctxs_.push_back(handler->getContext(method, connectionContext));
    }
  }

  ~ContextStack() {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->freeContext(ctxs_[i], getMethod());
      }
    }
  }

  void preWrite();

  void onWriteData(const SerializedMessage& msg);

  void postWrite(uint32_t bytes);

  void preRead();

  void onReadData(const SerializedMessage& msg);

  void postRead(apache::thrift::transport::THeader* header, uint32_t bytes);

  void handlerError();

  void handlerErrorWrapped(const folly::exception_wrapper& ew);

  void userException(const std::string& ex, const std::string& ex_what);

  void userExceptionWrapped(bool declared, const folly::exception_wrapper& ew);

  void asyncComplete();

  const char* getServiceName() {
    return serviceName_.c_str();
  }

  const char* getMethod() {
    return method_.c_str();
  }

 private:
  std::vector<void*> ctxs_;
  std::shared_ptr<std::vector<std::shared_ptr<TProcessorEventHandler>>>
      handlers_;
  std::string serviceName_;
  std::string method_;
};

} // namespace thrift
} // namespace apache
