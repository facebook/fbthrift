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
      : ctxs_(), handlers_(handlers), method_(method) {
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

  void preWrite() {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->preWrite(ctxs_[i], getMethod());
      }
    }
  }

  void onWriteData(const SerializedMessage& msg) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->onWriteData(ctxs_[i], getMethod(), msg);
      }
    }
  }

  void postWrite(uint32_t bytes) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->postWrite(ctxs_[i], getMethod(), bytes);
      }
    }
  }

  void preRead() {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->preRead(ctxs_[i], getMethod());
      }
    }
  }

  void onReadData(const SerializedMessage& msg) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->onReadData(ctxs_[i], getMethod(), msg);
      }
    }
  }

  void postRead(apache::thrift::transport::THeader* header, uint32_t bytes) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->postRead(ctxs_[i], getMethod(), header, bytes);
      }
    }
  }

  void handlerError() {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->handlerError(ctxs_[i], getMethod());
      }
    }
  }

  void handlerErrorWrapped(const folly::exception_wrapper& ew) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->handlerErrorWrapped(ctxs_[i], getMethod(), ew);
      }
    }
  }

  void userException(const std::string& ex, const std::string& ex_what) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->userException(ctxs_[i], getMethod(), ex, ex_what);
      }
    }
  }

  void userExceptionWrapped(bool declared, const folly::exception_wrapper& ew) {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->userExceptionWrapped(
            ctxs_[i], getMethod(), declared, ew);
      }
    }
  }

  void asyncComplete() {
    if (handlers_) {
      for (size_t i = 0; i < handlers_->size(); i++) {
        (*handlers_)[i]->asyncComplete(ctxs_[i], getMethod());
      }
    }
  }

  const char* getMethod() {
    return method_.c_str();
  }

 private:
  std::vector<void*> ctxs_;
  std::shared_ptr<std::vector<std::shared_ptr<TProcessorEventHandler>>>
      handlers_;
  std::string method_;
};

} // namespace thrift
} // namespace apache
