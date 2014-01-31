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
#ifndef THRIFT_TEST_TIMEUTIL_H_
#define THRIFT_TEST_TIMEUTIL_H_ 1

#include <boost/test/unit_test.hpp>
#include <iostream>

namespace apache { namespace thrift { namespace test {

class TimePoint {
 public:
  explicit TimePoint(bool set = true)
    : timeStart_(0),
      timeEnd_(0),
      timeWaiting_(0),
      tid_(0) {
    if (set) {
      reset();
    }
  }

  void reset();

  bool isUnset() const {
    return (timeStart_ == 0 && timeEnd_ == 0 && timeWaiting_ == 0);
  }

  int64_t getTime() const {
    return timeStart_;
  }

  int64_t getTimeStart() const {
    return timeStart_;
  }

  int64_t getTimeEnd() const {
    return timeStart_;
  }

  int64_t getTimeWaiting() const {
    return timeWaiting_;
  }

  pid_t getTid() const {
    return tid_;
  }

 private:
  int64_t timeStart_;
  int64_t timeEnd_;
  int64_t timeWaiting_;
  pid_t tid_;
};

std::ostream& operator<<(std::ostream& os, const TimePoint& timePoint);

boost::test_tools::predicate_result checkTimeout(const TimePoint& start,
                                                 const TimePoint& end,
                                                 int64_t expectedMS,
                                                 bool allowSmaller,
                                                 int64_t tolerance = 5);

/**
 * Check how long a timeout took to fire.
 *
 * This method verifies:
 * - that the timeout did not fire too early (never less than expectedMS)
 * - that the timeout fired within a reasonable period of the expected
 *   duration.  It must fire within the specified tolerance, excluding time
 *   that this process spent waiting to be scheduled.
 *
 * @param start                 A TimePoint object set just before the timeout
 *                              was scheduled.
 * @param end                   A TimePoint object set when the timeout fired.
 * @param expectedMS            The timeout duration, in milliseconds
 * @param tolerance             The tolerance, in milliseconds.
 */
#define T_CHECK_TIMEOUT(start, end, expectedMS, ...) \
  BOOST_CHECK(apache::thrift::test::checkTimeout((start), (end), \
                                                 (expectedMS), false, \
                                                 ##__VA_ARGS__))

/**
 * Verify that an event took less than a specified amount of time.
 *
 * This is similar to T_CHECK_TIMEOUT, but does not fail if the event took less
 * than the allowed time.
 */
#define T_CHECK_TIME_LT(start, end, expectedMS, ...) \
  BOOST_CHECK(apache::thrift::test::checkTimeout((start), (end), \
                                                 (expectedMS), true, \
                                                 ##__VA_ARGS__))

}}} // apache::thrift::test

#endif // THRIFT_TEST_TIMEUTIL_H_
