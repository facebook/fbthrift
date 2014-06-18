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

#pragma once

#include <atomic>
#include <chrono>

namespace apache { namespace thrift {

/* Codel algorithm implementation:
 * http://en.wikipedia.org/wiki/CoDel
 *
 * Algorithm modified slightly:  Instead of changing the interval time
 * based on the average min delay, instead we use an alternate timeout
 * for each task if the min delay during the interval period is too
 * high.
 *
 * This was found to have better latency metrics than changing the
 * window size, since we can communicate with the sender via thrift
 * instead of only via the tcp window size congestion control, as in TCP.
 */
class Codel {

 public:
  Codel();

  // Comment
  bool overloaded(std::chrono::microseconds delay);

  int getLoad();

 private:
  std::chrono::microseconds codelMinDelay_;
  std::chrono::time_point<std::chrono::steady_clock> codelIntervalTime_;

  // Comment
  std::atomic<bool> codelResetDelay_;

  bool overloaded_;
};

}} // Namespace
