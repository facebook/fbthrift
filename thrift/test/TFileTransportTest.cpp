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
#ifndef _GNU_SOURCE
#define _GNU_SOURCE // needed for getopt_long
#endif

#include <sys/time.h>
#include <getopt.h>
#include <boost/test/unit_test.hpp>
#include <boost/scoped_ptr.hpp>
#include <deque>

#include "thrift/lib/cpp/concurrency/Mutex.h"
#include "thrift/lib/cpp/concurrency/Util.h"
#include "thrift/lib/cpp/transport/TFileTransport.h"
#include "thrift/lib/cpp/test/TimeUtil.h"

using apache::thrift::concurrency::Guard;
using apache::thrift::concurrency::Mutex;
using apache::thrift::concurrency::Util;
using apache::thrift::transport::TFileTransport;
using apache::thrift::test::TimePoint;
using boost::scoped_ptr;
using boost::test_tools::predicate_result;
using std::deque;

/**************************************************************************
 * Global state
 **************************************************************************/

// If shm_dir is available, we will change tmp_dir to point to it instead.
static const char* tmp_dir = "/tmp";
static const char* shm_dir = "/dev/shm";

class FsyncHandler {
 public:
  virtual ~FsyncHandler() {}

  virtual int fsync(int fd) = 0;
};

FsyncHandler* fsyncHandler;


/**************************************************************************
 * Helper code
 **************************************************************************/

/**
 * Class to check fsync calls in test_flush_max_us_impl()
 *
 * This makes sure that every call to write() if followed by a call to fsync()
 * within a set amount of time.
 */
class FsyncTimer : public FsyncHandler {
 public:
  typedef deque<predicate_result> ResultList;

  explicit FsyncTimer(uint32_t maxFlushTime)
    : maxFlushTime_(maxFlushTime),
      flushed_(true),
      mutex_(),
      bufferedSince_(),
      results_() {}

  bool isFlushed() const {
    return flushed_;
  }

  /**
   * The test code calls writePerformed() immediately before every call that it
   * makes to write to the TFileTransport.
   */
  void writePerformed() {
    Guard g(mutex_);
    if (flushed_) {
      flushed_= false;
      bufferedSince_.reset();
    }
  }

  virtual int fsync(int fd) {
    TimePoint now;
    Guard g(mutex_);

    // If there weren't any new writes since the last fsync(),
    // we don't need to check the bufferedSince_ timer.
    if (flushed_) {
      return 0;
    }

    // Check how long we've had new data that hasn't been flushed yet,
    // and make sure it's less than maxFlushTime_.
    //
    // Note that we don't call BOOST_CHECK() directly here, since fsync is
    // invoked from other threads.  Instead, just store the check results, then
    // evaluate them later in the main thread.
    bool allowSmaller = true;
    predicate_result checkResult = checkTimeout(bufferedSince_, now,
                                                maxFlushTime_, allowSmaller);
    results_.push_back(checkResult);

    flushed_ = true;
    return 0;
  }

  /**
   * checkResults() should be called from the main thread to evaluate the test
   * results stored by the fsync() thread.
   */
  void checkResults() const {
    Guard g(mutex_);

    BOOST_CHECK(!results_.empty());
    for (ResultList::const_iterator it = results_.begin();
         it != results_.end();
         ++it) {
      // The stuff inside BOOST_CHECK(...) is shown in the error message,
      // so name the variable here instead of just using BOOST_CHECK(*it)
      const predicate_result& result = *it;
      BOOST_CHECK(result);
    }
  }

 private:
  uint32_t maxFlushTime_;
  bool flushed_;
  Mutex mutex_;
  TimePoint bufferedSince_;
  ResultList results_;
};

/**
 * Helper class to clean up temporary files
 */
class TempFile {
 public:
  TempFile(const char* directory, const char* prefix) {
    size_t path_len = strlen(directory) + strlen(prefix) + 8;
    path_ = new char[path_len];
    snprintf(path_, path_len, "%s/%sXXXXXX", directory, prefix);

    fd_ = mkstemp(path_);
    if (fd_ < 0) {
      throw apache::thrift::TLibraryException("mkstemp() failed");
    }
  }

  ~TempFile() {
    unlink();
    close();
  }

  const char* getPath() const {
    return path_;
  }

  int getFD() const {
    return fd_;
  }

  void unlink() {
    if (path_) {
      ::unlink(path_);
      delete[] path_;
      path_ = nullptr;
    }
  }

  void close() {
    if (fd_ < 0) {
      return;
    }

    ::close(fd_);
    fd_ = -1;
  }

 private:
  char* path_;
  int fd_;
};

// Use our own version of fsync() for testing.
// This returns immediately, so timing in test_destructor() isn't affected by
// waiting on the actual filesystem.
extern "C"
int fsync(int fd) {
  if (fsyncHandler) {
    return fsyncHandler->fsync(fd);
  }
  return 0;
}


/**************************************************************************
 * Test cases
 **************************************************************************/

/**
 * Make sure the TFileTransport destructor exits "quickly".
 *
 * Previous versions had a bug causing the writer thread not to exit
 * right away.
 *
 * It's kind of lame that we just check to see how long the destructor takes in
 * wall-clock time.  This could result in false failures on slower systems, or
 * on heavily loaded machines.
 */
BOOST_AUTO_TEST_CASE(test_destructor) {
  TempFile f(tmp_dir, "thrift.TFileTransportTest.");

  unsigned int const NUM_ITERATIONS = 1000;

  for (unsigned int n = 0; n < NUM_ITERATIONS; ++n) {
    ftruncate(f.getFD(), 0);

    scoped_ptr<TFileTransport> transport(new TFileTransport(f.getPath()));

    // write something so that the writer thread gets started
    transport->write(reinterpret_cast<const uint8_t*>("foo"), 3);

    // Every other iteration, also call flush(), just in case that potentially
    // has any effect on how the writer thread wakes up.
    if (n & 0x1) {
      transport->flush();
    }

    // Time the call to the destructor
    TimePoint start;
    transport.reset();
    TimePoint end;

    // Make sure the destructor exits "quickly".
    //
    // Normally the destructor should complete in under 1ms.  However, allow a
    // few hundred milliseconds of slop so that the test won't fail if we are
    // running on a heavily loaded machine.
    //
    // (Note that TimePoint keeps track of how long we spent waiting on the
    // CPU, precisely so we can perform more accurate timing checks even on
    // heavily loaded servers.  However, the destructor will spend most of its
    // time waiting on the write thread, and TimePoint can only track time that
    // the current thread spent waiting on the CPU.  On heavily loaded systems,
    // the write thread can easily get blocked waiting for CPU and we won't be
    // able to detect that.)
    //
    // I've seen the other thread spend up to 250ms waiting on the CPU on
    // heavily loaded systems.  Allow up to 500ms to be safe.  (The regression
    // that this bug is testing for would cause the destructor to take the full
    // 3000ms timeout interval, so we'll still catch that bug.)
    T_CHECK_TIME_LT(start, end, 500);
  }
}

/**
 * Make sure setFlushMaxUs() is honored.
 */
void test_flush_max_us_impl(uint32_t flush_us, uint32_t write_us,
                            uint32_t test_us) {
  TempFile f(tmp_dir, "thrift.TFileTransportTest.");

  // Create an FsyncTimer to process calls to fsync()
  FsyncTimer fsyncTimer(flush_us);
  fsyncHandler = &fsyncTimer;

  scoped_ptr<TFileTransport> transport(new TFileTransport(f.getPath()));
  // Don't flush because of # of bytes written
  transport->setFlushMaxBytes(0xffffffff);
  uint8_t buf[] = "a";
  uint32_t buflen = sizeof(buf);

  // Set the flush interval
  transport->setFlushMaxUs(flush_us);

  // Make one call to write, to start the writer thread now.
  // (If we just let the thread get created during our test loop,
  // the thread creation sometimes takes long enough to make the first
  // fsync interval fail the check.)
  transport->write(buf, buflen);

  // Loop doing write(), sleep(), ...
  uint32_t total_time = 0;
  while (true) {
    fsyncTimer.writePerformed();
    transport->write(buf, buflen);
    if (total_time > test_us) {
      break;
    }
    usleep(write_us);
    total_time += write_us;
  }

  transport.reset();
  // Make sure the transport called fsync() on the FD before exiting
  BOOST_CHECK(fsyncTimer.isFlushed());

  // Stop logging new fsync() calls
  fsyncHandler = nullptr;

  // FsyncTimer will have recorded a list of predicate_results that we need to
  // evaluate in the main thread.
  fsyncTimer.checkResults();
}

BOOST_AUTO_TEST_CASE(test_flush_max_10_4) {
  // fsync every 10ms, write every 4ms, for 200ms
  test_flush_max_us_impl(10000, 4000, 200000);
}

BOOST_AUTO_TEST_CASE(test_flush_max_10_14) {
  // fsync every 10ms, write every 14ms, for 200ms
  test_flush_max_us_impl(10000, 14000, 200000);
}

BOOST_AUTO_TEST_CASE(test_flush_max_50_21) {
  // fsync every 50ms, write every 21ms, for 300ms
  test_flush_max_us_impl(50000, 21000, 300000);
}

BOOST_AUTO_TEST_CASE(test_flush_max_50_68) {
  // fsync every 50ms, write every 68ms, for 300ms
  test_flush_max_us_impl(50000, 68000, 300000);
}

/**
 * Make sure flush() is fast when there is nothing to do.
 *
 * TFileTransport used to have a bug where flush() would wait for the fsync
 * timeout to expire.
 */
BOOST_AUTO_TEST_CASE(test_noop_flush) {
  TempFile f(tmp_dir, "thrift.TFileTransportTest.");
  TFileTransport transport(f.getPath());

  // Write something to start the writer thread.
  uint8_t buf[] = "a";
  transport.write(buf, 1);

  TimePoint start;
  for (unsigned int n = 0; n < 10; ++n) {
    transport.flush();

    // Fail if at any point we've been running for longer than half a second.
    // (With the buggy code, TFileTransport used to take 3 seconds per flush())
    //
    // Use a fatal fail so we break out early, rather than continuing to make
    // many more slow flush() calls.
    TimePoint now;
    T_CHECK_TIME_LT(start, now, 500);
  }
}

/**************************************************************************
 * General Initialization
 **************************************************************************/

void print_usage(FILE* f, const char* argv0) {
  fprintf(f, "Usage: %s [boost_options] [options]\n", argv0);
  fprintf(f, "Options:\n");
  fprintf(f, "  --tmp-dir=DIR, -t DIR\n");
  fprintf(f, "  --help\n");
}

void parse_args(int argc, char* argv[]) {
  int seed;
  int *seedptr = nullptr;

  struct option long_opts[] = {
    { "help", false, nullptr, 'h' },
    { "tmp-dir", true, nullptr, 't' },
    { nullptr, 0, nullptr, 0 }
  };

  while (true) {
    optopt = 1;
    int optchar = getopt_long(argc, argv, "ht:", long_opts, nullptr);
    if (optchar == -1) {
      break;
    }

    switch (optchar) {
      case 't':
        tmp_dir = optarg;
        break;
      case 'h':
        print_usage(stdout, argv[0]);
        exit(0);
      case '?':
        exit(1);
      default:
        // Only happens if someone adds another option to the optarg string,
        // but doesn't update the switch statement to handle it.
        fprintf(stderr, "unknown option \"-%c\"\n", optchar);
        exit(1);
    }
  }
}

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  boost::unit_test::framework::master_test_suite().p_name.value =
    "TFileTransportTest";

  // Parse arguments
  parse_args(argc, argv);

  // If this machine has a writable /dev/shm directory, write temporary files
  // to /dev/shm instead of /tmp.  /dev/shm is a memory filesystem, and should
  // be faster than /tmp.  Using /tmp can cause the timing check in
  // test_destructor to fail if the disk is very busy.
  if (access(shm_dir, W_OK) == 0) {
    tmp_dir = shm_dir;
  }

  return nullptr;
}
