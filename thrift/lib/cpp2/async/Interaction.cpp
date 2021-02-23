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

void Tile::__fbthrift_releaseRef(folly::EventBase& eb) {
  eb.dcheckIsInEventBaseThread();
  DCHECK_GT(refCount_, 0u);

  if (auto serial = dynamic_cast<SerialInteractionTile*>(this)) {
    auto& queue = serial->taskQueue_;
    if (!queue.empty()) {
      DCHECK_GT(refCount_, queue.size());
      dynamic_cast<concurrency::ThreadManager&>(*destructionExecutor_)
          .getKeepAlive(
              concurrency::PRIORITY::NORMAL,
              concurrency::ThreadManager::Source::INTERNAL)
          ->add([task = std::move(queue.front())]() mutable { task->run(); });
      queue.pop();
    }
  }

  if (--refCount_ == 0) {
    std::move(destructionExecutor_).add([this](auto) { delete this; });
  }
}

} // namespace thrift
} // namespace apache
