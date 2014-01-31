/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#ifndef THRIFT_ASYNC_HHWHEELTIMER_H_
#define THRIFT_ASYNC_HHWHEELTIMER_H_ 1

#include "thrift/lib/cpp/async/TAsyncTimeout.h"
#include "thrift/lib/cpp/async/TDelayedDestruction.h"

#include <boost/intrusive/list.hpp>

#include <chrono>
#include <cstddef>
#include <memory>
#include <list>

namespace apache { namespace thrift { namespace async {

/**
 * Hashed Hierarchical Wheel Timer See paper and powerpoint "Hashed
 * and Hierarchical Timing Wheels: Efficient Data Structures for
 * Implementing a Timer Facility by George Varghese and Anthony Lauck"
 *
 * Comparison:
 * TAsyncTimeout - a single timeout.
 * TAsyncTimeoutSet - a set of efficient timeouts with the same interval
 * HHWheelTimer - a set of efficient timeouts with different interval,
 *    but timeouts are not exact.
 *
 * All of the above are O(1) in insertion, tick update and cancel

 * This implementation ticks once every 10ms.
 * We model timers as the number of ticks until the next
 * due event.  We allow 32-bits of space to track this
 * due interval, and break that into 4 regions of 8 bits.
 * Each region indexes into a bucket of 256 lists.
 *
 * Bucket 0 represents those events that are due the soonest.
 * Each tick causes us to look at the next list in a bucket.
 * The 0th list in a bucket is special; it means that it is time to
 * flush the timers from the next higher bucket and schedule them
 * into a different bucket.
 *
 * This technique results in a very cheap mechanism for
 * maintaining time and timers, provided that we can maintain
 * a consistent rate of ticks.
 */
class HHWheelTimer : protected apache::thrift::async::TAsyncTimeout,
      virtual public apache::thrift::async::TDelayedDestruction {
 public:
  typedef std::unique_ptr<HHWheelTimer, Destructor> UniquePtr;

  /**
   * A callback to be notified when a timeout has expired.
   */
  class Callback {
   public:
    Callback()
      : wheel_(nullptr)
      , expiration_(0) {}

    virtual ~Callback();

    /**
     * timeoutExpired() is invoked when the timeout has expired.
     */
    virtual void timeoutExpired() noexcept = 0;

    /**
     * Cancel the timeout, if it is running.
     *
     * If the timeout is not scheduled, cancelTimeout() does nothing.
     */
    void cancelTimeout() {
      if (wheel_ == nullptr) {
        // We're not scheduled, so there's nothing to do.
        return;
      }
      cancelTimeoutImpl();
    }

    /**
     * Return true if this timeout is currently scheduled, and false otherwise.
     */
    bool isScheduled() const {
      return wheel_ != nullptr;
    }

   private:
    // Get the time remaining until this timeout expires
    std::chrono::milliseconds getTimeRemaining(
          std::chrono::milliseconds now) const {
      if (now >= expiration_) {
        return std::chrono::milliseconds(0);
      }
      return expiration_ - now;
    }

    void setScheduled(HHWheelTimer* wheel,
                      std::chrono::milliseconds);
    void cancelTimeoutImpl();

    HHWheelTimer* wheel_;
    std::chrono::milliseconds expiration_;

    typedef boost::intrusive::list_member_hook<
      boost::intrusive::link_mode<boost::intrusive::auto_unlink> > ListHook;

    ListHook hook_;

    typedef boost::intrusive::list<
      Callback,
      boost::intrusive::member_hook<Callback, ListHook, &Callback::hook_>,
      boost::intrusive::constant_time_size<false> > List;

    std::shared_ptr<RequestContext> context_;

    // Give HHWheelTimer direct access to our members so it can take care
    // of scheduling/cancelling.
    friend class HHWheelTimer;
  };

  /**
   * Create a new HHWheelTimer with the specified interval.
   */
  static int DEFAULT_TICK_INTERVAL;
  explicit HHWheelTimer(apache::thrift::async::TEventBase* eventBase,
                        std::chrono::milliseconds intervalMS =
                        std::chrono::milliseconds(DEFAULT_TICK_INTERVAL));

  /**
   * Destroy the HHWheelTimer.
   *
   * A HHWheelTimer should only be destroyed when there are no more
   * callbacks pending in the set.
   */
  virtual void destroy();

  /**
   * Get the tick interval for this HHWheelTimer.
   *
   * Returns the tick interval in milliseconds.
   */
  std::chrono::milliseconds getTickInterval() const {
    return interval_;
  }

  /**
   * Schedule the specified Callback to be invoked after the
   * specified timeout interval.
   *
   * If the callback is already scheduled, this cancels the existing timeout
   * before scheduling the new timeout.
   */
  void scheduleTimeout(Callback* callback,
                       std::chrono::milliseconds timeout);
  void scheduleTimeoutImpl(Callback* callback,
                       std::chrono::milliseconds timeout);

  /**
   * Return the number of currently pending timeouts
   */
  uint64_t count() const {
    return count_;
  }

  /**
   * This turns on more exact timing.  By default the wheel timer
   * increments its cached time only once everyN (default) ticks.
   *
   * With catchupEveryN at 1, timeouts will only be delayed until the
   * next tick, at which point all overdue timeouts are called.  The
   * wheel timer is approximately 2x slower with this set to 1.
   *
   * Load testing in opt mode showed skew was about 1% with no catchup.
   */
  void setCatchupEveryN(uint32_t everyN) {
    catchupEveryN_ = everyN;
  }

 protected:
  /**
   * Protected destructor.
   *
   * Use destroy() instead.  See the comments in TDelayedDestruction for more
   * details.
   */
  virtual ~HHWheelTimer();

 private:
  // Forbidden copy constructor and assignment operator
  HHWheelTimer(HHWheelTimer const &) = delete;
  HHWheelTimer& operator=(HHWheelTimer const &) = delete;

  // Methods inherited from TAsyncTimeout
  virtual void timeoutExpired() noexcept;

  std::chrono::milliseconds interval_;

  static constexpr int WHEEL_BUCKETS = 4;
  static constexpr int WHEEL_BITS = 8;
  static constexpr unsigned int WHEEL_SIZE = (1 << WHEEL_BITS);
  static constexpr unsigned int WHEEL_MASK = (WHEEL_SIZE - 1);
  static constexpr uint32_t LARGEST_SLOT = 0xffffffffUL;

  typedef Callback::List CallbackList;
  CallbackList buckets_[WHEEL_BUCKETS][WHEEL_SIZE];

  uint32_t timeToWheelTicks(std::chrono::milliseconds t) {
    return t.count() / interval_.count();
  }

  bool cascadeTimers(int bucket, int tick);
  int64_t nextTick_;
  uint64_t count_;
  std::chrono::milliseconds now_;

  static constexpr uint32_t DEFAULT_CATCHUP_EVERY_N = 100;

  uint32_t catchupEveryN_;
  uint32_t expirationsSinceCatchup_;
};

}}} // apache::thrift::async

#endif // THRIFT_ASYNC_TASYNCTIMEOUTSET_H_
