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
  class NameWrapper;

  explicit ContextStack(NameWrapper&& method) : method_(std::move(method)) {}

  ContextStack(
      const std::shared_ptr<
          std::vector<std::shared_ptr<TProcessorEventHandler>>>& handlers,
      NameWrapper&& serviceName,
      NameWrapper&& method,
      TConnectionContext* connectionContext)
      : handlers_(handlers),
        serviceName_(std::move(serviceName)),
        method_(std::move(method)) {
    ctxs_.reserve(handlers->size());
    for (const auto& handler : *handlers) {
      ctxs_.push_back(handler->getServiceContext(
          serviceName_.name(), method_.name(), connectionContext));
    }
  }

  ContextStack(
      const std::shared_ptr<
          std::vector<std::shared_ptr<TProcessorEventHandler>>>& handlers,
      NameWrapper&& method,
      TConnectionContext* connectionContext)
      : handlers_(handlers), method_(std::move(method)) {
    ctxs_.reserve(handlers->size());
    for (const auto& handler : *handlers) {
      ctxs_.push_back(handler->getContext(method_.name(), connectionContext));
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

  // NameWrapper allows us to hold either a static cstr without
  // copying (e.g. a compiler const) or a cstr copied to std::string.
  // This way, we can avoid copying in the common case in generated code.
  class NameWrapper {
    enum StaticOp { STATIC };

   public:
    NameWrapper() : cstr_("") {}
    /* implicit */
    NameWrapper(const char* n) : storage_(n), cstr_(storage_.c_str()) {}
    static NameWrapper makeFromStatic(const char* n) {
      return NameWrapper(STATIC, n);
    }
    const char* name() {
      return cstr_;
    }
    NameWrapper(const NameWrapper&) = delete;
    NameWrapper& operator=(const NameWrapper&) = delete;
    NameWrapper(NameWrapper&& other) noexcept {
      if (other.storage_.empty()) {
        cstr_ = other.cstr_;
      } else {
        storage_ = other.storage_;
        cstr_ = storage_.c_str();
      }
    }
    NameWrapper& operator=(NameWrapper&& other) noexcept {
      if (&other != this) {
        if (other.storage_.empty()) {
          storage_.clear();
          cstr_ = other.cstr_;
        } else {
          storage_ = other.storage_;
          cstr_ = storage_.c_str();
        }
      }
      return *this;
    }

   private:
    NameWrapper(StaticOp, const char* n) {
      cstr_ = n;
    }
    std::string storage_;
    const char* cstr_;
  };

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
    return serviceName_.name();
  }

  const char* getMethod() {
    return method_.name();
  }

 private:
  std::vector<void*> ctxs_;
  std::shared_ptr<std::vector<std::shared_ptr<TProcessorEventHandler>>>
      handlers_;
  NameWrapper serviceName_;
  NameWrapper method_;
};

} // namespace thrift
} // namespace apache
