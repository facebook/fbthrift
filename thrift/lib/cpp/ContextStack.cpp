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

#include <thrift/lib/cpp/ContextStack.h>
#include <folly/tracing/StaticTracepoint.h>

namespace apache {
namespace thrift {

void ContextStack::preWrite() {
  FOLLY_SDT(
      thrift, thrift_context_stack_pre_write, getServiceName(), getMethod());

  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->preWrite(ctxs_[i], getMethod());
    }
  }
}

void ContextStack::onWriteData(const SerializedMessage& msg) {
  FOLLY_SDT(
      thrift,
      thrift_context_stack_on_write_data,
      getServiceName(),
      getMethod());

  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->onWriteData(ctxs_[i], getMethod(), msg);
    }
  }
}

void ContextStack::postWrite(uint32_t bytes) {
  FOLLY_SDT(
      thrift,
      thrift_context_stack_post_write,
      getServiceName(),
      getMethod(),
      bytes);

  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->postWrite(ctxs_[i], getMethod(), bytes);
    }
  }
}

void ContextStack::preRead() {
  FOLLY_SDT(
      thrift, thrift_context_stack_pre_read, getServiceName(), getMethod());

  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->preRead(ctxs_[i], getMethod());
    }
  }
}

void ContextStack::onReadData(const SerializedMessage& msg) {
  FOLLY_SDT(
      thrift, thrift_context_stack_on_read_data, getServiceName(), getMethod());

  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->onReadData(ctxs_[i], getMethod(), msg);
    }
  }
}

void ContextStack::postRead(
    apache::thrift::transport::THeader* header,
    uint32_t bytes) {
  FOLLY_SDT(
      thrift,
      thrift_context_stack_post_read,
      getServiceName(),
      getMethod(),
      bytes);

  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->postRead(ctxs_[i], getMethod(), header, bytes);
    }
  }
}

void ContextStack::handlerErrorWrapped(const folly::exception_wrapper& ew) {
  FOLLY_SDT(
      thrift,
      thrift_context_stack_handler_error_wrapped,
      getServiceName(),
      getMethod());

  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->handlerErrorWrapped(ctxs_[i], getMethod(), ew);
    }
  }
}

void ContextStack::userExceptionWrapped(
    bool declared,
    const folly::exception_wrapper& ew) {
  FOLLY_SDT(
      thrift,
      thrift_context_stack_user_exception_wrapped,
      getServiceName(),
      getMethod());

  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->userExceptionWrapped(
          ctxs_[i], getMethod(), declared, ew);
    }
  }
}

void ContextStack::asyncComplete() {
  FOLLY_SDT(
      thrift,
      thrift_context_stack_async_complete,
      getServiceName(),
      getMethod());

  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->asyncComplete(ctxs_[i], getMethod());
    }
  }
}

} // namespace thrift
} // namespace apache
