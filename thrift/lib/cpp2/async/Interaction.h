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

  operator int64_t() const { return id_; }

 private:
  InteractionId(int64_t id) : id_(id) {}

  void release() { id_ = 0; }

  int64_t id_;

  friend class RequestChannel;
};

enum class InteractionReleaseEvent {
  NORMAL,
  STREAM_TRANSFER,
  STREAM_END,
};

class Tile {
 public:
  virtual ~Tile() = default;

  void __fbthrift_acquireRef(folly::EventBase& eb) {
    eb.dcheckIsInEventBaseThread();
    ++refCount_;
  }
  void __fbthrift_releaseRef(
      folly::EventBase& eb,
      InteractionReleaseEvent event = InteractionReleaseEvent::NORMAL);

  // Only moves in arg when it returns true
  virtual bool __fbthrift_maybeEnqueue(
      std::unique_ptr<concurrency::Runnable>&& task);

 private:
  size_t refCount_{1};
  folly::Executor::KeepAlive<> destructionExecutor_;
  friend class GeneratedAsyncProcessor;
  friend class TilePromise;
};

class SerialInteractionTile : public Tile {
 public:
  bool __fbthrift_maybeEnqueue(
      std::unique_ptr<concurrency::Runnable>&& task) override;

 private:
  std::queue<std::shared_ptr<concurrency::Runnable>> taskQueue_;
  bool hasActiveRequest_{false};
  friend class Tile;
};

class TilePromise final : public Tile {
 public:
  bool __fbthrift_maybeEnqueue(
      std::unique_ptr<concurrency::Runnable>&& task) override;

  template <typename InteractionEventTask>
  void fulfill(
      Tile& tile, concurrency::ThreadManager& tm, folly::EventBase& eb) {
    DCHECK(!continuations_.empty());

    auto ka = tm.getKeepAlive(
        concurrency::PRIORITY::NORMAL,
        concurrency::ThreadManager::Source::EXISTING_INTERACTION);
    for (auto& task : continuations_) {
      dynamic_cast<InteractionEventTask&>(*task).setTile(tile);
      tile.__fbthrift_acquireRef(eb);
      --refCount_;
      if (!tile.__fbthrift_maybeEnqueue(std::move(task))) {
        ka->add([task = std::move(task)]() mutable { task->run(); });
      }
    }
    continuations_.clear();
  }

  template <typename EventTask>
  void failWith(folly::exception_wrapper ew, const std::string& exCode) {
    for (auto& task : continuations_) {
      dynamic_cast<EventTask&>(*task).failWith(ew, exCode);
    }
    continuations_.clear();
  }

 private:
  std::deque<std::unique_ptr<concurrency::Runnable>> continuations_;
  friend class GeneratedAsyncProcessor;
};

} // namespace thrift
} // namespace apache
