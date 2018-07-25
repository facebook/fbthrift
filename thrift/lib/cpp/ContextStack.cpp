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

#include <thrift/lib/cpp/ContextStack.h>

namespace apache {
namespace thrift {

void ContextStack::preWrite() {
  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->preWrite(ctxs_[i], getMethod());
    }
  }
}

void ContextStack::onWriteData(const SerializedMessage& msg) {
  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->onWriteData(ctxs_[i], getMethod(), msg);
    }
  }
}

void ContextStack::postWrite(uint32_t bytes) {
  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->postWrite(ctxs_[i], getMethod(), bytes);
    }
  }
}

void ContextStack::preRead() {
  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->preRead(ctxs_[i], getMethod());
    }
  }
}

void ContextStack::onReadData(const SerializedMessage& msg) {
  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->onReadData(ctxs_[i], getMethod(), msg);
    }
  }
}

void ContextStack::postRead(
    apache::thrift::transport::THeader* header,
    uint32_t bytes) {
  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->postRead(ctxs_[i], getMethod(), header, bytes);
    }
  }
}

void ContextStack::handlerError() {
  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->handlerError(ctxs_[i], getMethod());
    }
  }
}

void ContextStack::handlerErrorWrapped(const folly::exception_wrapper& ew) {
  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->handlerErrorWrapped(ctxs_[i], getMethod(), ew);
    }
  }
}

void ContextStack::userException(
    const std::string& ex,
    const std::string& ex_what) {
  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->userException(ctxs_[i], getMethod(), ex, ex_what);
    }
  }
}

void ContextStack::userExceptionWrapped(
    bool declared,
    const folly::exception_wrapper& ew) {
  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->userExceptionWrapped(
          ctxs_[i], getMethod(), declared, ew);
    }
  }
}

void ContextStack::asyncComplete() {
  if (handlers_) {
    for (size_t i = 0; i < handlers_->size(); i++) {
      (*handlers_)[i]->asyncComplete(ctxs_[i], getMethod());
    }
  }
}

} // namespace thrift
} // namespace apache
