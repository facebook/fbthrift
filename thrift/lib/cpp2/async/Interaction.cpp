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

#include <thrift/lib/cpp2/async/Interaction.h>

namespace apache {
namespace thrift {

bool Tile::__fbthrift_maybeEnqueue(
    std::unique_ptr<concurrency::Runnable>&&,
    const concurrency::ThreadManager::ExecutionScope&) {
  return false;
}

void Tile::decRef(folly::EventBase& eb, InteractionReleaseEvent event) {
  eb.dcheckIsInEventBaseThread();
  DCHECK_GT(refCount_, 0u);

  if (event != InteractionReleaseEvent::STREAM_END) {
    if (auto serial = dynamic_cast<SerialInteractionTile*>(this)) {
      auto& queue = serial->taskQueue_;
      if (!queue.empty()) {
        DCHECK_GT(
            refCount_ + (event == InteractionReleaseEvent::STREAM_TRANSFER),
            queue.size());
        auto& [task, scope] = queue.front();
        tm_->getKeepAlive(
               std::move(scope), concurrency::ThreadManager::Source::INTERNAL)
            ->add([task = std::move(task)]() { task->run(); });
        queue.pop();
      } else {
        serial->hasActiveRequest_ = false;
      }
    }
  }

  if (event != InteractionReleaseEvent::STREAM_TRANSFER && --refCount_ == 0) {
    if (tm_) {
      std::move(tm_).add([this](auto&&) { delete this; });
    } else {
      delete this;
    }
  }
}

bool TilePromise::__fbthrift_maybeEnqueue(
    std::unique_ptr<concurrency::Runnable>&& task,
    const concurrency::ThreadManager::ExecutionScope& scope) {
  continuations_.emplace(std::move(task), scope);
  return true;
}

void TilePromise::fulfill(
    Tile& tile, concurrency::ThreadManager& tm, folly::EventBase& eb) {
  DCHECK(!continuations_.empty());
  tile.tm_ = &tm;

  // Inline destruction of this is possible at the setTile()
  auto continuations = std::move(continuations_);
  while (!continuations.empty()) {
    auto& [task, scope] = continuations.front();
    dynamic_cast<InteractionTask&>(*task).setTile({&tile, &eb});
    if (!tile.__fbthrift_maybeEnqueue(std::move(task), scope)) {
      tm.getKeepAlive(
            std::move(scope),
            concurrency::ThreadManager::Source::EXISTING_INTERACTION)
          ->add([task = std::move(task)]() mutable { task->run(); });
    }
    continuations.pop();
  }
}

void TilePromise::failWith(
    folly::exception_wrapper ew, const std::string& exCode) {
  auto continuations = std::move(continuations_);
  while (!continuations.empty()) {
    auto& [task, scope] = continuations.front();
    dynamic_cast<InteractionTask&>(*task).failWith(ew, exCode);
    continuations.pop();
  }
}

bool SerialInteractionTile::__fbthrift_maybeEnqueue(
    std::unique_ptr<concurrency::Runnable>&& task,
    const concurrency::ThreadManager::ExecutionScope& scope) {
  if (hasActiveRequest_) {
    taskQueue_.emplace(std::move(task), scope);
    return true;
  }

  hasActiveRequest_ = true;
  return false;
}

void TilePtr::release(InteractionReleaseEvent event) {
  if (tile_) {
    if (eb_->inRunningEventBaseThread()) {
      tile_->decRef(*eb_, event);
      tile_ = nullptr;
    } else {
      std::move(eb_).add([tile = std::exchange(tile_, nullptr),
                          event](auto&& eb) { tile->decRef(*eb, event); });
    }
  }
}

TileStreamGuard::TileStreamGuard(TilePtr&& ptr) : tile_(std::move(ptr)) {
  if (auto tile = tile_.get()) {
    tile_->decRef(*tile_.eb_, InteractionReleaseEvent::STREAM_TRANSFER);
  }
}

} // namespace thrift
} // namespace apache
