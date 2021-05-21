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

bool Tile::__fbthrift_maybeEnqueue(std::unique_ptr<concurrency::Runnable>&&) {
  return false;
}

void Tile::__fbthrift_releaseRef(
    folly::EventBase& eb, InteractionReleaseEvent event) {
  eb.dcheckIsInEventBaseThread();
  DCHECK_GT(refCount_, 0u);

  if (event != InteractionReleaseEvent::STREAM_END) {
    if (auto serial = dynamic_cast<SerialInteractionTile*>(this)) {
      auto& queue = serial->taskQueue_;
      if (!queue.empty()) {
        DCHECK_GT(
            refCount_ + (event == InteractionReleaseEvent::STREAM_TRANSFER),
            queue.size());
        dynamic_cast<concurrency::ThreadManager&>(*destructionExecutor_)
            .getKeepAlive(
                concurrency::PRIORITY::NORMAL,
                concurrency::ThreadManager::Source::INTERNAL)
            ->add([task = std::move(queue.front())]() { task->run(); });
        queue.pop();
      } else {
        serial->hasActiveRequest_ = false;
      }
    }
  }

  if (event != InteractionReleaseEvent::STREAM_TRANSFER && --refCount_ == 0) {
    std::move(destructionExecutor_).add([this](auto) { delete this; });
  }
}

bool TilePromise::__fbthrift_maybeEnqueue(
    std::unique_ptr<concurrency::Runnable>&& task) {
  continuations_.push_back(std::move(task));
  return true;
}

bool SerialInteractionTile::__fbthrift_maybeEnqueue(
    std::unique_ptr<concurrency::Runnable>&& task) {
  if (hasActiveRequest_) {
    taskQueue_.push(std::move(task));
    return true;
  }

  hasActiveRequest_ = true;
  return false;
}

} // namespace thrift
} // namespace apache
