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
#define __STDC_FORMAT_MACROS

#include "thrift/lib/cpp/test/TimeUtil.h"

#include "thrift/lib/cpp/concurrency/Util.h"
#include "thrift/lib/cpp/Thrift.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <sys/utsname.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

using std::string;

namespace apache { namespace thrift { namespace test {

/**
 * glibc doesn't provide gettid(), so define it ourselves.
 */
static pid_t gettid() {
  return syscall(SYS_gettid);
}

/**
 * The /proc/<pid>/schedstat file reports time values in jiffies.
 *
 * Determine how many jiffies are in a second.
 * Returns -1 if the number of jiffies/second cannot be determined.
 */
static int64_t determineJiffiesHZ() {
  // It seems like the only real way to figure out the CONFIG_HZ value used by
  // this kernel is to look it up in the config file.
  //
  // Look in /boot/config-<kernel_release>
  struct utsname unameInfo;
  if (uname(&unameInfo) != 0) {
    T_ERROR("unable to determine jiffies/second: uname failed: %s",
            strerror(errno));
    return -1;
  }

  char configPath[256];
  snprintf(configPath, sizeof(configPath), "/boot/config-%s",
           unameInfo.release);

  FILE* f = fopen(configPath, "r");
  if (f == nullptr) {
    T_ERROR("unable to determine jiffies/second: "
            "cannot open kernel config file %s", configPath);
    return -1;
  }

  int64_t hz = -1;
  char buf[1024];
  while (fgets(buf, sizeof(buf), f) != nullptr) {
    if (strcmp(buf, "CONFIG_NO_HZ=y\n") == 0) {
      // schedstat info seems to be reported in nanoseconds on tickless
      // kernels.
      //
      // The CONFIG_HZ value doesn't matter for our purposes,
      // so return as soon as we see CONFIG_NO_HZ.
      fclose(f);
      return 1000000000;
    } else if (strcmp(buf, "CONFIG_HZ=1000\n") == 0) {
      hz = 1000;
    } else if (strcmp(buf, "CONFIG_HZ=300\n") == 0) {
      hz = 300;
    } else if (strcmp(buf, "CONFIG_HZ=250\n") == 0) {
      hz = 250;
    } else if (strcmp(buf, "CONFIG_HZ=100\n") == 0) {
      hz = 100;
    }
  }
  fclose(f);

  if (hz == -1) {
    T_ERROR("unable to determine jiffies/second: no CONFIG_HZ setting "
            "found in %s", configPath);
    return -1;
  }

  return hz;
}

/**
 * Determine how long this process has spent waiting to get scheduled on the
 * CPU.
 *
 * Returns the number of milliseconds spent waiting, or -1 if the amount of
 * time cannot be determined.
 */
static int64_t getTimeWaitingMS(pid_t tid) {
  static int64_t jiffiesHZ = 0;
  if (jiffiesHZ == 0) {
    jiffiesHZ = determineJiffiesHZ();
  }

  if (jiffiesHZ < 0) {
    // We couldn't figure out how many jiffies there are in a second.
    // Don't bother reading the schedstat info if we can't interpret it.
    return -1;
  }

  int fd = -1;
  try {
    char schedstatFile[256];
    snprintf(schedstatFile, sizeof(schedstatFile),
             "/proc/%d/schedstat", tid);
    fd = open(schedstatFile, O_RDONLY);
    if (fd < 0) {
      throw TLibraryException("failed to open process schedstat file", errno);
    }

    char buf[512];
    ssize_t bytesRead = read(fd, buf, sizeof(buf) - 1);
    if (bytesRead <= 0) {
      throw TLibraryException("failed to read process schedstat file", errno);
    }

    if (buf[bytesRead - 1] != '\n') {
      throw TLibraryException("expected newline at end of schedstat data");
    }
    assert(bytesRead < sizeof(buf));
    buf[bytesRead] = '\0';

    uint64_t activeJiffies = 0;
    uint64_t waitingJiffies = 0;
    uint64_t numTasks = 0;
    int rc = sscanf(buf, "%" PRIu64 " %" PRIu64 " %" PRIu64 "\n",
                    &activeJiffies, &waitingJiffies, &numTasks);
    if (rc != 3) {
      throw TLibraryException("failed to parse schedstat data");
    }

    close(fd);
    return (waitingJiffies * 1000) / jiffiesHZ;
  } catch (const TLibraryException& e) {
    if (fd >= 0) {
      close(fd);
    }
    T_ERROR("error determining process wait time: %s", e.what());
    return -1;
  }
}

void TimePoint::reset() {
  // Remember the current time
  timeStart_ = concurrency::Util::currentTime();

  // Remember how long this process has spent waiting to be scheduled
  tid_ = gettid();
  timeWaiting_ = getTimeWaitingMS(tid_);

  // In case it took a while to read the schedstat info,
  // also record the time after the schedstat check
  timeEnd_ = concurrency::Util::currentTime();
}

std::ostream& operator<<(std::ostream& os, const TimePoint& timePoint) {
  os << "TimePoint(" << timePoint.getTimeStart() << ", " <<
    timePoint.getTimeEnd() << ", " <<
    timePoint.getTimeWaiting() << ")";
  return os;
}

boost::test_tools::predicate_result
checkTimeout(const TimePoint& start, const TimePoint& end,
             int64_t expectedMS, bool allowSmaller, int64_t tolerance) {
  int64_t elapsedMS = end.getTimeStart() - start.getTimeEnd();

  if (!allowSmaller) {
    // timeouts should never fire before the time was up
    if (elapsedMS < expectedMS) {
      boost::test_tools::predicate_result res(false);
      res.message() << "event occurred in only " << elapsedMS <<
        "ms, before expected " << expectedMS << "ms had expired";
      return res;
    }
  }

  // Check that the event fired within a reasonable time of the timout.
  //
  // If the system is under heavy load, our process may have had to wait for a
  // while to be run.  The time spent waiting for the processor shouldn't
  // count against us, so exclude this time from the check.
  int64_t excludedMS;
  if (end.getTid() != start.getTid()) {
    // We can only correctly compute the amount of time waiting to be scheduled
    // if both TimePoints were set in the same thread.
    excludedMS = 0;
  } else {
    excludedMS = end.getTimeWaiting() - start.getTimeWaiting();
    assert(end.getTimeWaiting() >= start.getTimeWaiting());
    // Add a tolerance here due to precision issues on linux, see below note.
    assert( (elapsedMS + tolerance) >= excludedMS);
  }

  int64_t effectiveElapsedMS = elapsedMS - excludedMS;
  if (effectiveElapsedMS < 0) {
    effectiveElapsedMS = 0;
  }

  // On x86 Linux, sleep calls generally have precision only to the nearest
  // millisecond.  The tolerance parameter lets users allow a few ms of slop.
  int64_t overrun = effectiveElapsedMS - expectedMS;
  if (overrun > tolerance) {
    boost::test_tools::predicate_result res(false);
    res.message() << "event took too long: occurred after " <<
      effectiveElapsedMS << "+" << excludedMS << " ms; expected " <<
      expectedMS << "ms";
    return res;
  }

  return true;
}

}}} // apache::thrift::test
