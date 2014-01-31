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
#include "thrift/lib/cpp/async/HHWheelTimer.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TUndelayedDestruction.h"
#include "thrift/lib/cpp/test/TimeUtil.h"

#include <boost/test/unit_test.hpp>
#include <vector>

namespace unit_test = boost::unit_test;
using namespace apache::thrift::async;
using namespace apache::thrift::test;
using std::chrono::milliseconds;

typedef TUndelayedDestruction<HHWheelTimer> StackWheelTimer;

class TestTimeout : public HHWheelTimer::Callback {
 public:
  TestTimeout() {}
  TestTimeout(HHWheelTimer* t, milliseconds timeout) {
    t->scheduleTimeout(this, timeout);
  }
  virtual void timeoutExpired() noexcept {
    timestamps.push_back(TimePoint());
    if (fn) {
      fn();
    }
  }

  std::deque<TimePoint> timestamps;
  std::function<void()> fn;
};

/*
 * Test firing some simple timeouts that are fired once and never rescheduled
 */
BOOST_AUTO_TEST_CASE(FireOnce) {
  TEventBase eventBase;
  StackWheelTimer t(&eventBase, milliseconds(1));

  const HHWheelTimer::Callback* nullCallback = nullptr;

  TestTimeout t1;
  TestTimeout t2;
  TestTimeout t3;

  BOOST_REQUIRE_EQUAL(t.count(), 0);

  t.scheduleTimeout(&t1, milliseconds(5));
  t.scheduleTimeout(&t2, milliseconds(5));
  // Verify scheduling it twice cancels, then schedules.
  // Should only get one callback.
  t.scheduleTimeout(&t2, milliseconds(5));
  t.scheduleTimeout(&t3, milliseconds(10));

  BOOST_REQUIRE_EQUAL(t.count(), 3);

  TimePoint start;
  eventBase.loop();
  TimePoint end;

  BOOST_REQUIRE_EQUAL(t1.timestamps.size(), 1);
  BOOST_REQUIRE_EQUAL(t2.timestamps.size(), 1);
  BOOST_REQUIRE_EQUAL(t3.timestamps.size(), 1);

  BOOST_REQUIRE_EQUAL(t.count(), 0);

  T_CHECK_TIMEOUT(start, t1.timestamps[0], 5);
  T_CHECK_TIMEOUT(start, t2.timestamps[0], 5);
  T_CHECK_TIMEOUT(start, t3.timestamps[0], 10);
  T_CHECK_TIMEOUT(start, end, 10);
}

/*
 * Test cancelling a timeout when it is scheduled to be fired right away.
 */

BOOST_AUTO_TEST_CASE(CancelTimeout) {
  TEventBase eventBase;
  StackWheelTimer t(&eventBase, milliseconds(1));

  // Create several timeouts that will all fire in 5ms.
  TestTimeout t5_1(&t, milliseconds(5));
  TestTimeout t5_2(&t, milliseconds(5));
  TestTimeout t5_3(&t, milliseconds(5));
  TestTimeout t5_4(&t, milliseconds(5));
  TestTimeout t5_5(&t, milliseconds(5));

  // Also create a few timeouts to fire in 10ms
  TestTimeout t10_1(&t, milliseconds(10));
  TestTimeout t10_2(&t, milliseconds(10));
  TestTimeout t10_3(&t, milliseconds(10));

  TestTimeout t20_1(&t, milliseconds(20));
  TestTimeout t20_2(&t, milliseconds(20));

  // Have t5_1 cancel t5_2 and t5_4.
  //
  // Cancelling t5_2 will test cancelling a timeout that is at the head of the
  // list and ready to be fired.
  //
  // Cancelling t5_4 will test cancelling a timeout in the middle of the list
  t5_1.fn = [&] {
    t5_2.cancelTimeout();
    t5_4.cancelTimeout();
  };

  // Have t5_3 cancel t5_5.
  // This will test cancelling the last remaining timeout.
  //
  // Then have t5_3 reschedule itself.
  t5_3.fn = [&] {
    t5_5.cancelTimeout();
    // Reset our function so we won't continually reschedule ourself
    std::function<void()> fnDtorGuard;
    t5_3.fn.swap(fnDtorGuard);
    t.scheduleTimeout(&t5_3, milliseconds(5));

    // Also test cancelling timeouts in another timeset that isn't ready to
    // fire yet.
    //
    // Cancel the middle timeout in ts10.
    t10_2.cancelTimeout();
    // Cancel both the timeouts in ts20.
    t20_1.cancelTimeout();
    t20_2.cancelTimeout();
  };

  TimePoint start;
  eventBase.loop();
  TimePoint end;

  BOOST_REQUIRE_EQUAL(t5_1.timestamps.size(), 1);
  T_CHECK_TIMEOUT(start, t5_1.timestamps[0], 5);

  BOOST_REQUIRE_EQUAL(t5_3.timestamps.size(), 2);
  T_CHECK_TIMEOUT(start, t5_3.timestamps[0], 5);
  T_CHECK_TIMEOUT(t5_3.timestamps[0], t5_3.timestamps[1], 5);

  BOOST_REQUIRE_EQUAL(t10_1.timestamps.size(), 1);
  T_CHECK_TIMEOUT(start, t10_1.timestamps[0], 10);
  BOOST_REQUIRE_EQUAL(t10_3.timestamps.size(), 1);
  T_CHECK_TIMEOUT(start, t10_3.timestamps[0], 10);

  // Cancelled timeouts
  BOOST_CHECK_EQUAL(t5_2.timestamps.size(), 0);
  BOOST_CHECK_EQUAL(t5_4.timestamps.size(), 0);
  BOOST_CHECK_EQUAL(t5_5.timestamps.size(), 0);
  BOOST_CHECK_EQUAL(t10_2.timestamps.size(), 0);
  BOOST_CHECK_EQUAL(t20_1.timestamps.size(), 0);
  BOOST_CHECK_EQUAL(t20_2.timestamps.size(), 0);

  T_CHECK_TIMEOUT(start, end, 10);
}

/*
 * Test destroying a HHWheelTimer with timeouts outstanding
 */

BOOST_AUTO_TEST_CASE(DestroyTimeoutSet) {
  TEventBase eventBase;

  HHWheelTimer::UniquePtr t(
    new HHWheelTimer(&eventBase, milliseconds(1)));

  TestTimeout t5_1(t.get(), milliseconds(5));
  TestTimeout t5_2(t.get(), milliseconds(5));
  TestTimeout t5_3(t.get(), milliseconds(5));

  TestTimeout t10_1(t.get(), milliseconds(10));
  TestTimeout t10_2(t.get(), milliseconds(10));

  // Have t5_2 destroy t
  // Note that this will call destroy() inside t's timeoutExpired()
  // method.
  t5_2.fn = [&] {
    t5_3.cancelTimeout();
    t5_1.cancelTimeout();
    t10_1.cancelTimeout();
    t10_2.cancelTimeout();
    t.reset();};

  TimePoint start;
  eventBase.loop();
  TimePoint end;

  BOOST_REQUIRE_EQUAL(t5_1.timestamps.size(), 1);
  T_CHECK_TIMEOUT(start, t5_1.timestamps[0], 5);
  BOOST_REQUIRE_EQUAL(t5_2.timestamps.size(), 1);
  T_CHECK_TIMEOUT(start, t5_2.timestamps[0], 5);

  BOOST_CHECK_EQUAL(t5_3.timestamps.size(), 0);
  BOOST_CHECK_EQUAL(t10_1.timestamps.size(), 0);
  BOOST_CHECK_EQUAL(t10_2.timestamps.size(), 0);

  T_CHECK_TIMEOUT(start, end, 5);
}

/*
 * Test the tick interval parameter
 */
BOOST_AUTO_TEST_CASE(AtMostEveryN) {
  TEventBase eventBase;

  // Create a timeout set with a 10ms interval, to fire no more than once
  // every 3ms.
  milliseconds interval(25);
  milliseconds atMostEveryN(6);
  StackWheelTimer t(&eventBase, atMostEveryN);

  // Create 60 timeouts to be added to ts10 at 1ms intervals.
  uint32_t numTimeouts = 60;
  std::vector<TestTimeout> timeouts(numTimeouts);

  // Create a scheduler timeout to add the timeouts 1ms apart.
  uint32_t index = 0;
  StackWheelTimer ts1(&eventBase, milliseconds(1));
  TestTimeout scheduler(&ts1, milliseconds(1));
  scheduler.fn = [&] {
    if (index >= numTimeouts) {
      return;
    }
    // Call timeoutExpired() on the timeout so it will record a timestamp.
    // This is done only so we can record when we scheduled the timeout.
    // This way if ts1 starts to fall behind a little over time we will still
    // be comparing the ts10 timeouts to when they were first scheduled (rather
    // than when we intended to schedule them).  The scheduler may fall behind
    // eventually since we don't really schedule it once every millisecond.
    // Each time it finishes we schedule it for 1 millisecond in the future.
    // The amount of time it takes to run, and any delays it encounters
    // getting scheduled may eventually add up over time.
    timeouts[index].timeoutExpired();

    // Schedule the new timeout
    t.scheduleTimeout(&timeouts[index], interval);
    // Reschedule ourself
    ts1.scheduleTimeout(&scheduler, milliseconds(1));
    ++index;
  };

  // Go ahead and schedule the first timeout now.
  //scheduler.fn();

  TimePoint start;
  eventBase.loop();
  TimePoint end;

  // We scheduled timeouts 1ms apart, when the HHWheelTimer is only allowed
  // to wake up at most once every 3ms.  It will therefore wake up every 3ms
  // and fire groups of approximately 3 timeouts at a time.
  //
  // This is "approximately 3" since it may get slightly behind and fire 4 in
  // one interval, etc.  T_CHECK_TIMEOUT normally allows a few milliseconds of
  // tolerance.  We have to add the same into our checking algorithm here.
  for (uint32_t idx = 0; idx < numTimeouts; ++idx) {
    BOOST_REQUIRE_EQUAL(timeouts[idx].timestamps.size(), 2);

    TimePoint scheduledTime(timeouts[idx].timestamps[0]);
    TimePoint firedTime(timeouts[idx].timestamps[1]);

    // Assert that the timeout fired at roughly the right time.
    // T_CHECK_TIMEOUT() normally has a tolerance of 5ms.  Allow an additional
    // atMostEveryN.
    milliseconds tolerance = milliseconds(5) + interval;
    T_CHECK_TIMEOUT(scheduledTime, firedTime, atMostEveryN.count(),
                    tolerance.count());

    // Assert that the difference between the previous timeout and now was
    // either very small (fired in the same event loop), or larger than
    // atMostEveryN.
    if (idx == 0) {
      // no previous value
      continue;
    }
    TimePoint prev(timeouts[idx - 1].timestamps[1]);

    milliseconds delta((firedTime.getTimeStart() - prev.getTimeEnd()) -
                       (firedTime.getTimeWaiting() - prev.getTimeWaiting()));
    if (delta > milliseconds(1)) {
      T_CHECK_TIMEOUT(prev, firedTime, atMostEveryN.count()); }
  }
}

/*
 * Test an event loop that is blocking
 */

BOOST_AUTO_TEST_CASE(SlowLoop) {
  TEventBase eventBase;
  StackWheelTimer t(&eventBase, milliseconds(1));

  TestTimeout t1;
  TestTimeout t2;

  BOOST_REQUIRE_EQUAL(t.count(), 0);

  eventBase.runInLoop([](){usleep(10000);});
  t.scheduleTimeout(&t1, milliseconds(5));

  BOOST_REQUIRE_EQUAL(t.count(), 1);

  TimePoint start;
  eventBase.loop();
  TimePoint end;

  BOOST_REQUIRE_EQUAL(t1.timestamps.size(), 1);
  BOOST_REQUIRE_EQUAL(t.count(), 0);

  // Check that the timeout was delayed by sleep
  T_CHECK_TIMEOUT(start, t1.timestamps[0], 15, 1);
  T_CHECK_TIMEOUT(start, end, 15, 1);

  // Try it again, this time with catchup timing every loop
  t.setCatchupEveryN(1);

  eventBase.runInLoop([](){usleep(10000);});
  t.scheduleTimeout(&t2, milliseconds(5));

  BOOST_REQUIRE_EQUAL(t.count(), 1);

  TimePoint start2;
  eventBase.loop();
  TimePoint end2;

  BOOST_REQUIRE_EQUAL(t2.timestamps.size(), 1);
  BOOST_REQUIRE_EQUAL(t.count(), 0);

  // Check that the timeout was NOT delayed by sleep
  T_CHECK_TIMEOUT(start2, t2.timestamps[0], 10, 1);
  T_CHECK_TIMEOUT(start2, end2, 10, 1);
}

///////////////////////////////////////////////////////////////////////////
// init_unit_test_suite
///////////////////////////////////////////////////////////////////////////

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value = "TAsyncTimeoutTest";

  if (argc != 1) {
    std::cerr << "error: unhandled arguments:";
    for (int n = 1; n < argc; ++n) {
      std::cerr << " " << argv[n];
    }
    std::cerr << std::endl;
    exit(1);
  }

  return nullptr;
}
