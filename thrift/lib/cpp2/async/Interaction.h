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

#include <forward_list>

#include <folly/ExceptionWrapper.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/concurrency/ThreadManager.h>

namespace apache {
namespace thrift {

class InteractionId {
 public:
  InteractionId() : id_(0) {}
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

 private:
  InteractionId(int64_t id) : id_(id) {}

  void release() {
    id_ = 0;
  }

  int64_t id_;

  friend class RequestChannel;
};

class Tile {
 public:
  virtual ~Tile() = default;

  virtual bool __fbthrift_isPromise() const {
    return false;
  }
  virtual bool __fbthrift_isSerial() const {
    return false;
  }

  void __fbthrift_acquireRef(folly::EventBase& eb) {
    eb.dcheckIsInEventBaseThread();
    ++refCount_;
  }
  void __fbthrift_releaseRef(folly::EventBase& eb);

 private:
  bool terminationRequested_{false};
  size_t refCount_{0};
  folly::Executor::KeepAlive<> destructionExecutor_;
  friend class GeneratedAsyncProcessor;
  friend class TilePromise;
};

class SerialInteractionTile : public Tile {
  bool __fbthrift_isSerial() const final {
    return true;
  }

 private:
  std::queue<std::shared_ptr<concurrency::Runnable>> taskQueue_;
  friend class GeneratedAsyncProcessor;
  friend class Tile;
  friend class TilePromise;
};

class TilePromise final : public Tile {
 public:
  void addContinuation(std::shared_ptr<concurrency::Runnable> task) {
    continuations_.push_front(std::move(task));
  }

  template <typename InteractionEventTask>
  void
  fulfill(Tile& tile, concurrency::ThreadManager& tm, folly::EventBase& eb) {
    DCHECK(!continuations_.empty());

    bool isSerial = __fbthrift_isSerial(), first = true;
    continuations_.reverse();
    for (auto& task : continuations_) {
      if (!isSerial || std::exchange(first, false)) {
        tile.__fbthrift_acquireRef(eb);
        dynamic_cast<InteractionEventTask&>(*task).setTile(tile);
        --refCount_;
        tm.add(
            std::move(task),
            0, // timeout
            0, // expiration
            concurrency::ThreadManager::Source::EXISTING_INTERACTION);
      } else {
        static_cast<SerialInteractionTile&>(tile).taskQueue_.push(
            std::move(task));
      }
    }
    continuations_.clear();
    DCHECK_EQ(refCount_, 0u);
  }

  template <typename EventTask>
  void failWith(folly::exception_wrapper ew, const std::string& exCode) {
    continuations_.reverse();
    for (auto& task : continuations_) {
      dynamic_cast<EventTask&>(*task).failWith(ew, exCode);
    }
    continuations_.clear();
  }

  bool __fbthrift_isPromise() const override {
    return true;
  }

 private:
  std::forward_list<std::shared_ptr<concurrency::Runnable>> continuations_;
  bool destructionRequested_{false}; // set when connection is closed to prevent
                                     // continuations from running
  friend class GeneratedAsyncProcessor;
};

} // namespace thrift
} // namespace apache
