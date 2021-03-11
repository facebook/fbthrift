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

#include <folly/io/async/EventBaseAtomicNotificationQueue.h>

namespace apache {
namespace thrift {

/**
 * IOWorkerContext used for maintaining a dedicated EventBase queue for
 * server replies.
 */
class IOWorkerContext {
 public:
  virtual ~IOWorkerContext() {
    *(alive_->wlock()) = false;
  }

  /**
   * Get the reply queue.
   *
   * @returns reference to the queue.
   */
  virtual folly::EventBase::EventBaseQueue& getReplyQueue() {
    DCHECK(replyQueue_ != nullptr);
    return *replyQueue_.get();
  }

 protected:
  /**
   * Initializes the queue and registers it to the EventBase.
   *
   * @param eventBase EventBase to attach the queue.
   */
  void init(folly::EventBase& eventBase) {
    replyQueue_ = std::make_unique<folly::EventBase::EventBaseQueue>();
    replyQueue_->setMaxReadAtOnce(0);
    eventBase.runInEventBaseThread(
        [queue = replyQueue_.get(), &evb = eventBase, alive = alive_] {
          auto aliveLocked = alive->rlock();
          if (*aliveLocked) {
            // IOWorkerContext is still alive and so is replyQueue_
            queue->startConsumingInternal(&evb);
          }
        });
  }

 private:
  // A dedicated queue for server responses
  std::unique_ptr<folly::EventBase::EventBaseQueue> replyQueue_;
  // Needed to synchronize deallocating replyQueue_ and
  // calling startConsumingInternal() on eventbase loop.
  std::shared_ptr<folly::Synchronized<bool>> alive_{
      std::make_shared<folly::Synchronized<bool>>(true)};
};

} // namespace thrift
} // namespace apache
