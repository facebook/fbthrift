/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#ifndef _THRIFT_CONCURRENCY_UTIL_H_
#define _THRIFT_CONCURRENCY_UTIL_H_ 1

#include <cstdint>
#include <ctime>

#include <folly/Utility.h>
#include <folly/portability/SysTime.h>

namespace apache {
namespace thrift {
namespace concurrency {

/**
 * Utility methods
 *
 * This class contains basic utility methods for converting time formats,
 * and other common platform-dependent concurrency operations.
 * It should not be included in API headers for other concurrency library
 * headers, since it will, by definition, pull in all sorts of horrid
 * platform dependent crap.  Rather it should be included directly in
 * concurrency library implementation source.
 *
 * @version $Id:$
 */
class Util {
 public:
  static const int64_t NS_PER_S = 1000000000LL;
  static const int64_t US_PER_S = 1000000LL;
  static const int64_t MS_PER_S = 1000LL;

  static const int64_t NS_PER_MS = NS_PER_S / MS_PER_S;
  static const int64_t NS_PER_US = NS_PER_S / US_PER_S;
  static const int64_t US_PER_MS = US_PER_S / MS_PER_S;

  /**
   * Converts millisecond timestamp into a timespec struct
   *
   * @param result timespec& result
   * @param value or duration in milliseconds
   */
  static void toTimespec(struct timespec& result, int64_t value) {
    result.tv_sec = value / MS_PER_S; // ms to s
    result.tv_nsec = (value % MS_PER_S) * NS_PER_MS; // ms to ns
  }

  static void toTicks(
      int64_t& result,
      int64_t secs,
      int64_t oldTicks,
      int64_t oldTicksPerSec,
      int64_t newTicksPerSec) {
    result = secs * newTicksPerSec;
    result += oldTicks * newTicksPerSec / oldTicksPerSec;

    int64_t oldPerNew = oldTicksPerSec / newTicksPerSec;
    if (oldPerNew && ((oldTicks % oldPerNew) >= (oldPerNew / 2))) {
      ++result;
    }
  }
  /**
   * Converts struct timespec to arbitrary-sized ticks since epoch
   */
  static void toTicks(
      int64_t& result, const struct timespec& value, int64_t ticksPerSec) {
    return toTicks(result, value.tv_sec, value.tv_nsec, NS_PER_S, ticksPerSec);
  }

  /**
   * Converts struct timeval to arbitrary-sized ticks since epoch
   */
  static void toTicks(
      int64_t& result, const struct timeval& value, int64_t ticksPerSec) {
    return toTicks(result, value.tv_sec, value.tv_usec, US_PER_S, ticksPerSec);
  }

  /**
   * Converts struct timespec to milliseconds
   */
  static void toMilliseconds(int64_t& result, const struct timespec& value) {
    return toTicks(result, value, MS_PER_S);
  }

  /**
   * Converts struct timeval to milliseconds
   */
  static void toMilliseconds(int64_t& result, const struct timeval& value) {
    return toTicks(result, value, MS_PER_S);
  }

  /**
   * Get current time as milliseconds from epoch
   */
  static int64_t currentTime();
};

} // namespace concurrency
} // namespace thrift
} // namespace apache

#endif // #ifndef _THRIFT_CONCURRENCY_UTIL_H_
