/*
 * Copyright 2014 Facebook, Inc.
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

#include <thrift/lib/cpp/concurrency/Codel.h>
#include <math.h>
#include <gflags/gflags.h>

DEFINE_int32(codel_interval, 100,
             "Codel default interval time in ms");
DEFINE_int32(codel_target_delay, 5,
             "Target codel queueing delay in ms");

namespace apache { namespace thrift {

Codel::Codel()
    : codelMinDelay_(0),
      codelIntervalTime_(std::chrono::steady_clock::now()),
      codelResetDelay_(true),
      overloaded_(false) {}

bool Codel::overloaded(std::chrono::microseconds delay) {
  bool ret = false;
  auto now = std::chrono::steady_clock::now();

  // Avoid another thread updating the value at the same time we are using it
  // to calculate the overloaded state
  auto minDelay = codelMinDelay_;

  if (now  > codelIntervalTime_ &&
      (!codelResetDelay_.load(std::memory_order_acquire)
       && !codelResetDelay_.exchange(true))) {
    codelIntervalTime_ = now + std::chrono::milliseconds(FLAGS_codel_interval);

    if (minDelay > std::chrono::milliseconds(FLAGS_codel_target_delay)) {
      overloaded_ = true;
    } else {
      overloaded_ = false;
    }
  } else {
    if ((codelResetDelay_.load(std::memory_order_acquire) &&
         codelResetDelay_.exchange(false))
        || delay < codelMinDelay_) {
      codelMinDelay_ = delay;
    }
  }

  if (overloaded_ &&
      delay > std::chrono::milliseconds(FLAGS_codel_target_delay * 2)) {
    ret = true;
  }

  return ret;

}

int Codel::getLoad() {
  return codelMinDelay_.count() / (FLAGS_codel_target_delay * 10.0);
}

}} //namespace
