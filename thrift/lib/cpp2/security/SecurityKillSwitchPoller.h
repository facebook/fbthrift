/*
 * Copyright 2016 Facebook, Inc.
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

#ifndef SECURITY_KILL_SWITCH_POLLER_H
#define SECURITY_KILL_SWITCH_POLLER_H

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>

#include <folly/io/async/ScopedEventBaseThread.h>

namespace apache { namespace thrift {

// A class that polls for the existence of the TLS Killswitch
// on some specified interval.  Instances start automatically.
class SecurityKillSwitchPoller : public folly::AsyncTimeout {
 public:
  SecurityKillSwitchPoller();
  // public for test only
  SecurityKillSwitchPoller(const std::chrono::milliseconds& timeout,
                           std::function<bool()> pollFunc);

  ~SecurityKillSwitchPoller();

  // timeout callback
  void timeoutExpired() noexcept override;

  bool isKillSwitchEnabled() const { return switchEnabled_; }

 private:

  void setup();
  void updateSwitchState() noexcept;

  const std::chrono::milliseconds timeout_;
  std::function<bool()> pollFunc_;
  std::atomic<bool> switchEnabled_{false};
  folly::ScopedEventBaseThread thread_;
};
}
}

#endif
