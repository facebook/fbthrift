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
  explicit ContextStack(const char* method)
      : serviceName_(""), method_(method) {}

  ContextStack(
      const std::shared_ptr<
          std::vector<std::shared_ptr<TProcessorEventHandler>>>& handlers,
      const char* serviceName,
      const char* method,
      TConnectionContext* connectionContext)
      : serviceName_(serviceName), method_(method) {
    if (handlers && !handlers->empty()) {
      handlers_ = handlers;
      ctxs_.reserve(handlers->size());
      for (const auto& handler : *handlers) {
        ctxs_.push_back(handler->getServiceContext(
            serviceName_, method_, connectionContext));
      }
    }
  }

  ContextStack(
      const std::shared_ptr<
          std::vector<std::shared_ptr<TProcessorEventHandler>>>& handlers,
      const char* method,
      TConnectionContext* connectionContext)
      : serviceName_(""), method_(method) {
    if (handlers && !handlers->empty()) {
      handlers_ = handlers;
      ctxs_.reserve(handlers->size());
      for (const auto& handler : *handlers) {
        ctxs_.push_back(handler->getContext(method_, connectionContext));
      }
    }
  }

  ContextStack(ContextStack&&) = delete;
  ContextStack& operator=(ContextStack&&) = delete;
  ContextStack(const ContextStack&) = delete;
  ContextStack& operator=(const ContextStack&) = delete;

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

  void handlerErrorWrapped(const folly::exception_wrapper& ew);
  void userExceptionWrapped(bool declared, const folly::exception_wrapper& ew);

  void asyncComplete();

  const char* getServiceName() const {
    return serviceName_;
  }

  const char* getMethod() const {
    return method_;
  }

 private:
  std::vector<void*> ctxs_;
  std::shared_ptr<std::vector<std::shared_ptr<TProcessorEventHandler>>>
      handlers_;
  const char* const serviceName_;
  const char* const method_;
};

} // namespace thrift
} // namespace apache
