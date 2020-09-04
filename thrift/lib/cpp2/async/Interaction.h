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

namespace apache {
namespace thrift {

class InteractionId {
 public:
  InteractionId(InteractionId&& other) noexcept {
    id_ = other.id_;
    other.release();
  }
  InteractionId& operator=(InteractionId&& other) {
    if (this != &other) {
      CHECK_EQ(id_, 0) << "Interactions must always be terminated";
      id_ = other.id_;
      other.release();
    }
    return *this;
  }
  InteractionId(const InteractionId&) = delete;
  InteractionId& operator=(const InteractionId&) = delete;
  ~InteractionId() {
    CHECK_EQ(id_, 0) << "Interactions must always be terminated";
  }

  operator int64_t() const {
    return id_;
  }

  void bind(int64_t id) {
    CHECK_EQ(id_, 0) << "Can only bind dummy id objects";
    id_ = id;
  }

 private:
  InteractionId(int64_t id) : id_(id) {}

  void release() {
    id_ = 0;
  }

  int64_t id_;

  friend class RequestChannel;
};
} // namespace thrift
} // namespace apache
