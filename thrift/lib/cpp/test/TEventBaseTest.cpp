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
#include <boost/test/unit_test.hpp>

#include "thrift/lib/cpp/async/TAsyncTimeout.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TEventHandler.h"
#include "thrift/lib/cpp/concurrency/Mutex.h"
#include "thrift/lib/cpp/concurrency/PosixThreadFactory.h"
#include "thrift/lib/cpp/concurrency/Util.h"
#include "thrift/lib/cpp/test/SocketPair.h"
#include "thrift/lib/cpp/test/TimeUtil.h"

#include <iostream>
#include <unistd.h>
#include <memory>

using namespace boost;
using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::test;
using std::deque;
using std::pair;
using std::vector;
using std::make_pair;
using std::cerr;
using std::endl;

///////////////////////////////////////////////////////////////////////////
// Tests for read and write events
///////////////////////////////////////////////////////////////////////////

enum { BUF_SIZE = 4096 };

ssize_t writeToFD(int fd, size_t length) {
  // write an arbitrary amount of data to the fd
  char buf[length];
  memset(buf, 'a', sizeof(buf));
  ssize_t rc = write(fd, buf, sizeof(buf));
  BOOST_CHECK_EQUAL(rc, length);
  return rc;
}

size_t writeUntilFull(int fd) {
  // Write to the fd until EAGAIN is returned
  size_t bytesWritten = 0;
  char buf[BUF_SIZE];
  memset(buf, 'a', sizeof(buf));
  while (true) {
    ssize_t rc = write(fd, buf, sizeof(buf));
    if (rc < 0) {
      BOOST_REQUIRE_EQUAL(errno, EAGAIN);
      break;
    } else {
      bytesWritten += rc;
    }
  }
  return bytesWritten;
}

ssize_t readFromFD(int fd, size_t length) {
  // write an arbitrary amount of data to the fd
  char buf[length];
  return read(fd, buf, sizeof(buf));
}

size_t readUntilEmpty(int fd) {
  // Read from the fd until EAGAIN is returned
  char buf[BUF_SIZE];
  size_t bytesRead = 0;
  while (true) {
    int rc = read(fd, buf, sizeof(buf));
    if (rc == 0) {
      BOOST_FAIL("unexpected EOF");
    } else if (rc < 0) {
      BOOST_REQUIRE_EQUAL(errno, EAGAIN);
      break;
    } else {
      bytesRead += rc;
    }
  }
  return bytesRead;
}

void checkReadUntilEmpty(int fd, size_t expectedLength) {
  BOOST_CHECK_EQUAL(readUntilEmpty(fd), expectedLength);
}

struct ScheduledEvent {
  int milliseconds;
  uint16_t events;
  size_t length;
  ssize_t result;

  void perform(int fd) {
    if (events & TEventHandler::READ) {
      if (length == 0) {
        result = readUntilEmpty(fd);
      } else {
        result = readFromFD(fd, length);
      }
    }
    if (events & TEventHandler::WRITE) {
      if (length == 0) {
        result = writeUntilFull(fd);
      } else {
        result = writeToFD(fd, length);
      }
    }
  }
};

void scheduleEvents(TEventBase* eventBase, int fd, ScheduledEvent* events) {
  for (ScheduledEvent* ev = events; ev->milliseconds > 0; ++ev) {
    eventBase->runAfterDelay(std::bind(&ScheduledEvent::perform, ev, fd),
                             ev->milliseconds);
  }
}

class TestHandler : public TEventHandler {
 public:
  TestHandler(TEventBase* eventBase, int fd)
    : TEventHandler(eventBase, fd), fd_(fd) {}

  virtual void handlerReady(uint16_t events) noexcept {
    ssize_t bytesRead = 0;
    ssize_t bytesWritten = 0;
    if (events & READ) {
      // Read all available data, so TEventBase will stop calling us
      // until new data becomes available
      bytesRead = readUntilEmpty(fd_);
    }
    if (events & WRITE) {
      // Write until the pipe buffer is full, so TEventBase will stop calling
      // us until the other end has read some data
      bytesWritten = writeUntilFull(fd_);
    }

    log.push_back(EventRecord(events, bytesRead, bytesWritten));
  }

  struct EventRecord {
    EventRecord(uint16_t events, size_t bytesRead, size_t bytesWritten)
      : events(events)
      , timestamp()
      , bytesRead(bytesRead)
      , bytesWritten(bytesWritten) {}

    uint16_t events;
    TimePoint timestamp;
    ssize_t bytesRead;
    ssize_t bytesWritten;
  };

  deque<EventRecord> log;

 private:
  int fd_;
};

/**
 * Test a READ event
 */
BOOST_AUTO_TEST_CASE(ReadEvent) {
  TEventBase eb;
  SocketPair sp;

  // Register for read events
  TestHandler handler(&eb, sp[0]);
  handler.registerHandler(TEventHandler::READ);

  // Register timeouts to perform two write events
  ScheduledEvent events[] = {
    { 10, TEventHandler::WRITE, 2345 },
    { 160, TEventHandler::WRITE, 99 },
    { 0, 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  // Since we didn't use the TEventHandler::PERSIST flag, the handler should
  // have received the first read, then unregistered itself.  Check that only
  // the first chunk of data was received.
  BOOST_REQUIRE_EQUAL(handler.log.size(), 1);
  BOOST_CHECK_EQUAL(handler.log[0].events, TEventHandler::READ);
  T_CHECK_TIMEOUT(start, handler.log[0].timestamp, events[0].milliseconds, 90);
  BOOST_CHECK_EQUAL(handler.log[0].bytesRead, events[0].length);
  BOOST_CHECK_EQUAL(handler.log[0].bytesWritten, 0);
  T_CHECK_TIMEOUT(start, end, events[1].milliseconds, 30);

  // Make sure the second chunk of data is still waiting to be read.
  size_t bytesRemaining = readUntilEmpty(sp[0]);
  BOOST_CHECK_EQUAL(bytesRemaining, events[1].length);
}

/**
 * Test (READ | PERSIST)
 */
BOOST_AUTO_TEST_CASE(ReadPersist) {
  TEventBase eb;
  SocketPair sp;

  // Register for read events
  TestHandler handler(&eb, sp[0]);
  handler.registerHandler(TEventHandler::READ | TEventHandler::PERSIST);

  // Register several timeouts to perform writes
  ScheduledEvent events[] = {
    { 10,  TEventHandler::WRITE, 1024 },
    { 20,  TEventHandler::WRITE, 2211 },
    { 30,  TEventHandler::WRITE, 4096 },
    { 100, TEventHandler::WRITE, 100 },
    { 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Schedule a timeout to unregister the handler after the third write
  eb.runAfterDelay(std::bind(&TestHandler::unregisterHandler, &handler), 85);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  // The handler should have received the first 3 events,
  // then been unregistered after that.
  BOOST_REQUIRE_EQUAL(handler.log.size(), 3);
  for (int n = 0; n < 3; ++n) {
    BOOST_CHECK_EQUAL(handler.log[n].events, TEventHandler::READ);
    T_CHECK_TIMEOUT(start, handler.log[n].timestamp, events[n].milliseconds);
    BOOST_CHECK_EQUAL(handler.log[n].bytesRead, events[n].length);
    BOOST_CHECK_EQUAL(handler.log[n].bytesWritten, 0);
  }
  T_CHECK_TIMEOUT(start, end, events[3].milliseconds);

  // Make sure the data from the last write is still waiting to be read
  size_t bytesRemaining = readUntilEmpty(sp[0]);
  BOOST_CHECK_EQUAL(bytesRemaining, events[3].length);
}

/**
 * Test registering for READ when the socket is immediately readable
 */
BOOST_AUTO_TEST_CASE(ReadImmediate) {
  TEventBase eb;
  SocketPair sp;

  // Write some data to the socket so the other end will
  // be immediately readable
  size_t dataLength = 1234;
  writeToFD(sp[1], dataLength);

  // Register for read events
  TestHandler handler(&eb, sp[0]);
  handler.registerHandler(TEventHandler::READ | TEventHandler::PERSIST);

  // Register a timeout to perform another write
  ScheduledEvent events[] = {
    { 10, TEventHandler::WRITE, 2345 },
    { 0, 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Schedule a timeout to unregister the handler
  eb.runAfterDelay(std::bind(&TestHandler::unregisterHandler, &handler), 20);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  BOOST_REQUIRE_EQUAL(handler.log.size(), 2);

  // There should have been 1 event for immediate readability
  BOOST_CHECK_EQUAL(handler.log[0].events, TEventHandler::READ);
  T_CHECK_TIMEOUT(start, handler.log[0].timestamp, 0);
  BOOST_CHECK_EQUAL(handler.log[0].bytesRead, dataLength);
  BOOST_CHECK_EQUAL(handler.log[0].bytesWritten, 0);

  // There should be another event after the timeout wrote more data
  BOOST_CHECK_EQUAL(handler.log[1].events, TEventHandler::READ);
  T_CHECK_TIMEOUT(start, handler.log[1].timestamp, events[0].milliseconds);
  BOOST_CHECK_EQUAL(handler.log[1].bytesRead, events[0].length);
  BOOST_CHECK_EQUAL(handler.log[1].bytesWritten, 0);

  T_CHECK_TIMEOUT(start, end, 20);
}

/**
 * Test a WRITE event
 */
BOOST_AUTO_TEST_CASE(WriteEvent) {
  TEventBase eb;
  SocketPair sp;

  // Fill up the write buffer before starting
  size_t initialBytesWritten = writeUntilFull(sp[0]);

  // Register for write events
  TestHandler handler(&eb, sp[0]);
  handler.registerHandler(TEventHandler::WRITE);

  // Register timeouts to perform two reads
  ScheduledEvent events[] = {
    { 10, TEventHandler::READ, 0 },
    { 60, TEventHandler::READ, 0 },
    { 0, 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  // Since we didn't use the TEventHandler::PERSIST flag, the handler should
  // have only been able to write once, then unregistered itself.
  BOOST_REQUIRE_EQUAL(handler.log.size(), 1);
  BOOST_CHECK_EQUAL(handler.log[0].events, TEventHandler::WRITE);
  T_CHECK_TIMEOUT(start, handler.log[0].timestamp, events[0].milliseconds);
  BOOST_CHECK_EQUAL(handler.log[0].bytesRead, 0);
  BOOST_CHECK_GT(handler.log[0].bytesWritten, 0);
  T_CHECK_TIMEOUT(start, end, events[1].milliseconds);

  BOOST_CHECK_EQUAL(events[0].result, initialBytesWritten);
  BOOST_CHECK_EQUAL(events[1].result, handler.log[0].bytesWritten);
}

/**
 * Test (WRITE | PERSIST)
 */
BOOST_AUTO_TEST_CASE(WritePersist) {
  TEventBase eb;
  SocketPair sp;

  // Fill up the write buffer before starting
  size_t initialBytesWritten = writeUntilFull(sp[0]);

  // Register for write events
  TestHandler handler(&eb, sp[0]);
  handler.registerHandler(TEventHandler::WRITE | TEventHandler::PERSIST);

  // Register several timeouts to read from the socket at several intervals
  ScheduledEvent events[] = {
    { 10,  TEventHandler::READ, 0 },
    { 40,  TEventHandler::READ, 0 },
    { 70,  TEventHandler::READ, 0 },
    { 100, TEventHandler::READ, 0 },
    { 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Schedule a timeout to unregister the handler after the third read
  eb.runAfterDelay(std::bind(&TestHandler::unregisterHandler, &handler), 85);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  // The handler should have received the first 3 events,
  // then been unregistered after that.
  BOOST_REQUIRE_EQUAL(handler.log.size(), 3);
  BOOST_CHECK_EQUAL(events[0].result, initialBytesWritten);
  for (int n = 0; n < 3; ++n) {
    BOOST_CHECK_EQUAL(handler.log[n].events, TEventHandler::WRITE);
    T_CHECK_TIMEOUT(start, handler.log[n].timestamp, events[n].milliseconds);
    BOOST_CHECK_EQUAL(handler.log[n].bytesRead, 0);
    BOOST_CHECK_GT(handler.log[n].bytesWritten, 0);
    BOOST_CHECK_EQUAL(handler.log[n].bytesWritten, events[n + 1].result);
  }
  T_CHECK_TIMEOUT(start, end, events[3].milliseconds);
}

/**
 * Test registering for WRITE when the socket is immediately writable
 */
BOOST_AUTO_TEST_CASE(WriteImmediate) {
  TEventBase eb;
  SocketPair sp;

  // Register for write events
  TestHandler handler(&eb, sp[0]);
  handler.registerHandler(TEventHandler::WRITE | TEventHandler::PERSIST);

  // Register a timeout to perform a read
  ScheduledEvent events[] = {
    { 10, TEventHandler::READ, 0 },
    { 0, 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Schedule a timeout to unregister the handler
  int64_t unregisterTimeout = 40;
  eb.runAfterDelay(std::bind(&TestHandler::unregisterHandler, &handler),
                   unregisterTimeout);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  BOOST_REQUIRE_EQUAL(handler.log.size(), 2);

  // Since the socket buffer was initially empty,
  // there should have been 1 event for immediate writability
  BOOST_CHECK_EQUAL(handler.log[0].events, TEventHandler::WRITE);
  T_CHECK_TIMEOUT(start, handler.log[0].timestamp, 0);
  BOOST_CHECK_EQUAL(handler.log[0].bytesRead, 0);
  BOOST_CHECK_GT(handler.log[0].bytesWritten, 0);

  // There should be another event after the timeout wrote more data
  BOOST_CHECK_EQUAL(handler.log[1].events, TEventHandler::WRITE);
  T_CHECK_TIMEOUT(start, handler.log[1].timestamp, events[0].milliseconds);
  BOOST_CHECK_EQUAL(handler.log[1].bytesRead, 0);
  BOOST_CHECK_GT(handler.log[1].bytesWritten, 0);

  T_CHECK_TIMEOUT(start, end, unregisterTimeout);
}

/**
 * Test (READ | WRITE) when the socket becomes readable first
 */
BOOST_AUTO_TEST_CASE(ReadWrite) {
  TEventBase eb;
  SocketPair sp;

  // Fill up the write buffer before starting
  size_t sock0WriteLength = writeUntilFull(sp[0]);

  // Register for read and write events
  TestHandler handler(&eb, sp[0]);
  handler.registerHandler(TEventHandler::READ_WRITE);

  // Register timeouts to perform a write then a read.
  ScheduledEvent events[] = {
    { 10, TEventHandler::WRITE, 2345 },
    { 40, TEventHandler::READ, 0 },
    { 0, 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  // Since we didn't use the TEventHandler::PERSIST flag, the handler should
  // have only noticed readability, then unregistered itself.  Check that only
  // one event was logged.
  BOOST_REQUIRE_EQUAL(handler.log.size(), 1);
  BOOST_CHECK_EQUAL(handler.log[0].events, TEventHandler::READ);
  T_CHECK_TIMEOUT(start, handler.log[0].timestamp, events[0].milliseconds);
  BOOST_CHECK_EQUAL(handler.log[0].bytesRead, events[0].length);
  BOOST_CHECK_EQUAL(handler.log[0].bytesWritten, 0);
  BOOST_CHECK_EQUAL(events[1].result, sock0WriteLength);
  T_CHECK_TIMEOUT(start, end, events[1].milliseconds);
}

/**
 * Test (READ | WRITE) when the socket becomes writable first
 */
BOOST_AUTO_TEST_CASE(WriteRead) {
  TEventBase eb;
  SocketPair sp;

  // Fill up the write buffer before starting
  size_t sock0WriteLength = writeUntilFull(sp[0]);

  // Register for read and write events
  TestHandler handler(&eb, sp[0]);
  handler.registerHandler(TEventHandler::READ_WRITE);

  // Register timeouts to perform a read then a write.
  size_t sock1WriteLength = 2345;
  ScheduledEvent events[] = {
    { 10, TEventHandler::READ, 0 },
    { 40, TEventHandler::WRITE, sock1WriteLength },
    { 0, 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  // Since we didn't use the TEventHandler::PERSIST flag, the handler should
  // have only noticed writability, then unregistered itself.  Check that only
  // one event was logged.
  BOOST_REQUIRE_EQUAL(handler.log.size(), 1);
  BOOST_CHECK_EQUAL(handler.log[0].events, TEventHandler::WRITE);
  T_CHECK_TIMEOUT(start, handler.log[0].timestamp, events[0].milliseconds);
  BOOST_CHECK_EQUAL(handler.log[0].bytesRead, 0);
  BOOST_CHECK_GT(handler.log[0].bytesWritten, 0);
  BOOST_CHECK_EQUAL(events[0].result, sock0WriteLength);
  BOOST_CHECK_EQUAL(events[1].result, sock1WriteLength);
  T_CHECK_TIMEOUT(start, end, events[1].milliseconds);

  // Make sure the written data is still waiting to be read.
  size_t bytesRemaining = readUntilEmpty(sp[0]);
  BOOST_CHECK_EQUAL(bytesRemaining, events[1].length);
}

/**
 * Test (READ | WRITE) when the socket becomes readable and writable
 * at the same time.
 */
BOOST_AUTO_TEST_CASE(ReadWriteSimultaneous) {
  TEventBase eb;
  SocketPair sp;

  // Fill up the write buffer before starting
  size_t sock0WriteLength = writeUntilFull(sp[0]);

  // Register for read and write events
  TestHandler handler(&eb, sp[0]);
  handler.registerHandler(TEventHandler::READ_WRITE);

  // Register a timeout to perform a read and write together
  ScheduledEvent events[] = {
    { 10, TEventHandler::READ | TEventHandler::WRITE, 0 },
    { 0, 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  // It's not strictly required that the TEventBase register us about both
  // events in the same call.  So, it's possible that if the TEventBase
  // implementation changes this test could start failing, and it wouldn't be
  // considered breaking the API.  However for now it's nice to exercise this
  // code path.
  BOOST_REQUIRE_EQUAL(handler.log.size(), 1);
  BOOST_CHECK_EQUAL(handler.log[0].events,
                    TEventHandler::READ | TEventHandler::WRITE);
  T_CHECK_TIMEOUT(start, handler.log[0].timestamp, events[0].milliseconds);
  BOOST_CHECK_EQUAL(handler.log[0].bytesRead, sock0WriteLength);
  BOOST_CHECK_GT(handler.log[0].bytesWritten, 0);
  T_CHECK_TIMEOUT(start, end, events[0].milliseconds);
}

/**
 * Test (READ | WRITE | PERSIST)
 */
BOOST_AUTO_TEST_CASE(ReadWritePersist) {
  TEventBase eb;
  SocketPair sp;

  // Register for read and write events
  TestHandler handler(&eb, sp[0]);
  handler.registerHandler(TEventHandler::READ | TEventHandler::WRITE |
                          TEventHandler::PERSIST);

  // Register timeouts to perform several reads and writes
  ScheduledEvent events[] = {
    { 10, TEventHandler::WRITE, 2345 },
    { 20, TEventHandler::READ, 0 },
    { 35, TEventHandler::WRITE, 200 },
    { 45, TEventHandler::WRITE, 15 },
    { 55, TEventHandler::READ, 0 },
    { 120, TEventHandler::WRITE, 2345 },
    { 0, 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Schedule a timeout to unregister the handler
  eb.runAfterDelay(std::bind(&TestHandler::unregisterHandler, &handler), 80);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  BOOST_REQUIRE_EQUAL(handler.log.size(), 6);

  // Since we didn't fill up the write buffer immediately, there should
  // be an immediate event for writability.
  BOOST_CHECK_EQUAL(handler.log[0].events, TEventHandler::WRITE);
  T_CHECK_TIMEOUT(start, handler.log[0].timestamp, 0);
  BOOST_CHECK_EQUAL(handler.log[0].bytesRead, 0);
  BOOST_CHECK_GT(handler.log[0].bytesWritten, 0);

  // Events 1 through 5 should correspond to the scheduled events
  for (int n = 1; n < 6; ++n) {
    ScheduledEvent* event = &events[n - 1];
    T_CHECK_TIMEOUT(start, handler.log[n].timestamp, event->milliseconds);
    if (event->events == TEventHandler::READ) {
      BOOST_CHECK_EQUAL(handler.log[n].events, TEventHandler::WRITE);
      BOOST_CHECK_EQUAL(handler.log[n].bytesRead, 0);
      BOOST_CHECK_GT(handler.log[n].bytesWritten, 0);
    } else {
      BOOST_CHECK_EQUAL(handler.log[n].events, TEventHandler::READ);
      BOOST_CHECK_EQUAL(handler.log[n].bytesRead, event->length);
      BOOST_CHECK_EQUAL(handler.log[n].bytesWritten, 0);
    }
  }

  // The timeout should have unregistered the handler before the last write.
  // Make sure that data is still waiting to be read
  size_t bytesRemaining = readUntilEmpty(sp[0]);
  BOOST_CHECK_EQUAL(bytesRemaining, events[5].length);
}


class PartialReadHandler : public TestHandler {
 public:
  PartialReadHandler(TEventBase* eventBase, int fd, size_t readLength)
    : TestHandler(eventBase, fd), fd_(fd), readLength_(readLength) {}

  virtual void handlerReady(uint16_t events) noexcept {
    assert(events == TEventHandler::READ);
    ssize_t bytesRead = readFromFD(fd_, readLength_);
    log.push_back(EventRecord(events, bytesRead, 0));
  }

 private:
  int fd_;
  size_t readLength_;
};

/**
 * Test reading only part of the available data when a read event is fired.
 * When PERSIST is used, make sure the handler gets notified again the next
 * time around the loop.
 */
BOOST_AUTO_TEST_CASE(ReadPartial) {
  TEventBase eb;
  SocketPair sp;

  // Register for read events
  size_t readLength = 100;
  PartialReadHandler handler(&eb, sp[0], readLength);
  handler.registerHandler(TEventHandler::READ | TEventHandler::PERSIST);

  // Register a timeout to perform a single write,
  // with more data than PartialReadHandler will read at once
  ScheduledEvent events[] = {
    { 10, TEventHandler::WRITE, (3*readLength) + (readLength / 2) },
    { 0, 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Schedule a timeout to unregister the handler
  eb.runAfterDelay(std::bind(&TestHandler::unregisterHandler, &handler), 30);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  BOOST_REQUIRE_EQUAL(handler.log.size(), 4);

  // The first 3 invocations should read readLength bytes each
  for (int n = 0; n < 3; ++n) {
    BOOST_CHECK_EQUAL(handler.log[n].events, TEventHandler::READ);
    T_CHECK_TIMEOUT(start, handler.log[n].timestamp, events[0].milliseconds);
    BOOST_CHECK_EQUAL(handler.log[n].bytesRead, readLength);
    BOOST_CHECK_EQUAL(handler.log[n].bytesWritten, 0);
  }
  // The last read only has readLength/2 bytes
  BOOST_CHECK_EQUAL(handler.log[3].events, TEventHandler::READ);
  T_CHECK_TIMEOUT(start, handler.log[3].timestamp, events[0].milliseconds);
  BOOST_CHECK_EQUAL(handler.log[3].bytesRead, readLength / 2);
  BOOST_CHECK_EQUAL(handler.log[3].bytesWritten, 0);
}


class PartialWriteHandler : public TestHandler {
 public:
  PartialWriteHandler(TEventBase* eventBase, int fd, size_t writeLength)
    : TestHandler(eventBase, fd), fd_(fd), writeLength_(writeLength) {}

  virtual void handlerReady(uint16_t events) noexcept {
    assert(events == TEventHandler::WRITE);
    ssize_t bytesWritten = writeToFD(fd_, writeLength_);
    log.push_back(EventRecord(events, 0, bytesWritten));
  }

 private:
  int fd_;
  size_t writeLength_;
};

/**
 * Test writing without completely filling up the write buffer when the fd
 * becomes writable.  When PERSIST is used, make sure the handler gets
 * notified again the next time around the loop.
 */
BOOST_AUTO_TEST_CASE(WritePartial) {
  TEventBase eb;
  SocketPair sp;

  // Fill up the write buffer before starting
  size_t initialBytesWritten = writeUntilFull(sp[0]);

  // Register for write events
  size_t writeLength = 100;
  PartialWriteHandler handler(&eb, sp[0], writeLength);
  handler.registerHandler(TEventHandler::WRITE | TEventHandler::PERSIST);

  // Register a timeout to read, so that more data can be written
  ScheduledEvent events[] = {
    { 10, TEventHandler::READ, 0 },
    { 0, 0, 0 },
  };
  scheduleEvents(&eb, sp[1], events);

  // Schedule a timeout to unregister the handler
  eb.runAfterDelay(std::bind(&TestHandler::unregisterHandler, &handler), 30);

  // Loop
  TimePoint start;
  eb.loop();
  TimePoint end;

  // Depending on how big the socket buffer is, there will be multiple writes
  // Only check the first 5
  int numChecked = 5;
  BOOST_REQUIRE_GE(handler.log.size(), numChecked);
  BOOST_CHECK_EQUAL(events[0].result, initialBytesWritten);

  // The first 3 invocations should read writeLength bytes each
  for (int n = 0; n < numChecked; ++n) {
    BOOST_CHECK_EQUAL(handler.log[n].events, TEventHandler::WRITE);
    T_CHECK_TIMEOUT(start, handler.log[n].timestamp, events[0].milliseconds);
    BOOST_CHECK_EQUAL(handler.log[n].bytesRead, 0);
    BOOST_CHECK_EQUAL(handler.log[n].bytesWritten, writeLength);
  }
}


/**
 * Test destroying a registered TEventHandler
 */
BOOST_AUTO_TEST_CASE(DestroyHandler) {
  class DestroyHandler : public TAsyncTimeout {
   public:
    DestroyHandler(TEventBase* eb, TEventHandler* h)
      : TAsyncTimeout(eb)
      , handler_(h) {}

    virtual void timeoutExpired() noexcept {
      delete handler_;
    }

   private:
    TEventHandler* handler_;
  };

  TEventBase eb;
  SocketPair sp;

  // Fill up the write buffer before starting
  size_t initialBytesWritten = writeUntilFull(sp[0]);

  // Register for write events
  TestHandler* handler = new TestHandler(&eb, sp[0]);
  handler->registerHandler(TEventHandler::WRITE | TEventHandler::PERSIST);

  // After 10ms, read some data, so that the handler
  // will be notified that it can write.
  eb.runAfterDelay(std::bind(checkReadUntilEmpty, sp[1], initialBytesWritten),
                   10);

  // Start a timer to destroy the handler after 25ms
  // This mainly just makes sure the code doesn't break or assert
  DestroyHandler dh(&eb, handler);
  dh.scheduleTimeout(25);

  TimePoint start;
  eb.loop();
  TimePoint end;

  // Make sure the TEventHandler was uninstalled properly when it was
  // destroyed, and the TEventBase loop exited
  T_CHECK_TIMEOUT(start, end, 25);

  // Make sure that the handler wrote data to the socket
  // before it was destroyed
  size_t bytesRemaining = readUntilEmpty(sp[1]);
  BOOST_CHECK_GT(bytesRemaining, 0);
}


///////////////////////////////////////////////////////////////////////////
// Tests for timeout events
///////////////////////////////////////////////////////////////////////////

BOOST_AUTO_TEST_CASE(RunAfterDelay) {
  TEventBase eb;

  TimePoint timestamp1(false);
  TimePoint timestamp2(false);
  TimePoint timestamp3(false);
  eb.runAfterDelay(std::bind(&TimePoint::reset, &timestamp1), 10);
  eb.runAfterDelay(std::bind(&TimePoint::reset, &timestamp2), 20);
  eb.runAfterDelay(std::bind(&TimePoint::reset, &timestamp3), 40);

  TimePoint start;
  eb.loop();
  TimePoint end;

  T_CHECK_TIMEOUT(start, timestamp1, 10);
  T_CHECK_TIMEOUT(start, timestamp2, 20);
  T_CHECK_TIMEOUT(start, timestamp3, 40);
  T_CHECK_TIMEOUT(start, end, 40);
}

/**
 * Test the behavior of runAfterDelay() when some timeouts are
 * still scheduled when the TEventBase is destroyed.
 */
BOOST_AUTO_TEST_CASE(RunAfterDelayDestruction) {
  TimePoint timestamp1(false);
  TimePoint timestamp2(false);
  TimePoint timestamp3(false);
  TimePoint timestamp4(false);
  TimePoint start(false);
  TimePoint end(false);

  {
    TEventBase eb;

    // Run two normal timeouts
    eb.runAfterDelay(std::bind(&TimePoint::reset, &timestamp1), 10);
    eb.runAfterDelay(std::bind(&TimePoint::reset, &timestamp2), 20);

    // Schedule a timeout to stop the event loop after 40ms
    eb.runAfterDelay(std::bind(&TEventBase::terminateLoopSoon, &eb), 40);

    // Schedule 2 timeouts that would fire after the event loop stops
    eb.runAfterDelay(std::bind(&TimePoint::reset, &timestamp3), 80);
    eb.runAfterDelay(std::bind(&TimePoint::reset, &timestamp4), 160);

    start.reset();
    eb.loop();
    end.reset();
  }

  T_CHECK_TIMEOUT(start, timestamp1, 10);
  T_CHECK_TIMEOUT(start, timestamp2, 20);
  T_CHECK_TIMEOUT(start, end, 40);

  BOOST_CHECK(timestamp3.isUnset());
  BOOST_CHECK(timestamp4.isUnset());

  // Ideally this test should be run under valgrind to ensure that no
  // memory is leaked.
}

class TestTimeout : public TAsyncTimeout {
 public:
  explicit TestTimeout(TEventBase* eventBase)
    : TAsyncTimeout(eventBase)
    , timestamp(false) {}

  virtual void timeoutExpired() noexcept {
    timestamp.reset();
  }

  TimePoint timestamp;
};

BOOST_AUTO_TEST_CASE(BasicTimeouts) {
  TEventBase eb;

  TestTimeout t1(&eb);
  TestTimeout t2(&eb);
  TestTimeout t3(&eb);
  t1.scheduleTimeout(10);
  t2.scheduleTimeout(20);
  t3.scheduleTimeout(40);

  TimePoint start;
  eb.loop();
  TimePoint end;

  T_CHECK_TIMEOUT(start, t1.timestamp, 10);
  T_CHECK_TIMEOUT(start, t2.timestamp, 20);
  T_CHECK_TIMEOUT(start, t3.timestamp, 40);
  T_CHECK_TIMEOUT(start, end, 40);
}

class ReschedulingTimeout : public TAsyncTimeout {
 public:
  ReschedulingTimeout(TEventBase* evb, const vector<uint32_t>& timeouts)
    : TAsyncTimeout(evb)
    , timeouts_(timeouts)
    , iterator_(timeouts_.begin()) {}

  void start() {
    reschedule();
  }

  virtual void timeoutExpired() noexcept {
    timestamps.push_back(TimePoint());
    reschedule();
  }

  void reschedule() {
    if (iterator_ != timeouts_.end()) {
      uint32_t timeout = *iterator_;
      ++iterator_;
      scheduleTimeout(timeout);
    }
  }

  vector<TimePoint> timestamps;

 private:
  int iteration_;
  vector<uint32_t> timeouts_;
  vector<uint32_t>::const_iterator iterator_;
};

/**
 * Test rescheduling the same timeout multiple times
 */
BOOST_AUTO_TEST_CASE(ReuseTimeout) {
  TEventBase eb;

  vector<uint32_t> timeouts;
  timeouts.push_back(10);
  timeouts.push_back(30);
  timeouts.push_back(15);

  ReschedulingTimeout t(&eb, timeouts);
  t.start();

  TimePoint start;
  eb.loop();
  TimePoint end;

  // Use a higher tolerance than usual.  We're waiting on 3 timeouts
  // consecutively.  In general, each timeout may go over by a few
  // milliseconds, and we're tripling this error by witing on 3 timeouts.
  int64_t tolerance = 6;

  BOOST_REQUIRE_EQUAL(timeouts.size(), t.timestamps.size());
  uint32_t total = 0;
  for (int n = 0; n < timeouts.size(); ++n) {
    total += timeouts[n];
    T_CHECK_TIMEOUT(start, t.timestamps[n], total, tolerance);
  }
  T_CHECK_TIMEOUT(start, end, total, tolerance);
}

/**
 * Test rescheduling a timeout before it has fired
 */
BOOST_AUTO_TEST_CASE(RescheduleTimeout) {
  TEventBase eb;

  TestTimeout t1(&eb);
  TestTimeout t2(&eb);
  TestTimeout t3(&eb);

  t1.scheduleTimeout(15);
  t2.scheduleTimeout(30);
  t3.scheduleTimeout(30);

  auto f = static_cast<bool(TAsyncTimeout::*)(uint32_t)>(
      &TAsyncTimeout::scheduleTimeout);

  // after 10ms, reschedule t2 to run sooner than originally scheduled
  eb.runAfterDelay(std::bind(f, &t2, 10), 10);
  // after 10ms, reschedule t3 to run later than originally scheduled
  eb.runAfterDelay(std::bind(f, &t3, 40), 10);

  TimePoint start;
  eb.loop();
  TimePoint end;

  T_CHECK_TIMEOUT(start, t1.timestamp, 15);
  T_CHECK_TIMEOUT(start, t2.timestamp, 20);
  T_CHECK_TIMEOUT(start, t3.timestamp, 50);
  T_CHECK_TIMEOUT(start, end, 50);
}

/**
 * Test cancelling a timeout
 */
BOOST_AUTO_TEST_CASE(CancelTimeout) {
  TEventBase eb;

  vector<uint32_t> timeouts;
  timeouts.push_back(10);
  timeouts.push_back(30);
  timeouts.push_back(25);

  ReschedulingTimeout t(&eb, timeouts);
  t.start();
  eb.runAfterDelay(std::bind(&TAsyncTimeout::cancelTimeout, &t), 50);

  TimePoint start;
  eb.loop();
  TimePoint end;

  BOOST_REQUIRE_EQUAL(t.timestamps.size(), 2);
  T_CHECK_TIMEOUT(start, t.timestamps[0], 10);
  T_CHECK_TIMEOUT(start, t.timestamps[1], 40);
  T_CHECK_TIMEOUT(start, end, 50);
}

/**
 * Test destroying a scheduled timeout object
 */
BOOST_AUTO_TEST_CASE(DestroyTimeout) {
  class DestroyTimeout : public TAsyncTimeout {
   public:
    DestroyTimeout(TEventBase* eb, TAsyncTimeout* t)
      : TAsyncTimeout(eb)
      , timeout_(t) {}

    virtual void timeoutExpired() noexcept {
      delete timeout_;
    }

   private:
    TAsyncTimeout* timeout_;
  };

  TEventBase eb;

  TestTimeout* t1 = new TestTimeout(&eb);
  t1->scheduleTimeout(30);

  DestroyTimeout dt(&eb, t1);
  dt.scheduleTimeout(10);

  TimePoint start;
  eb.loop();
  TimePoint end;

  T_CHECK_TIMEOUT(start, end, 10);
}


///////////////////////////////////////////////////////////////////////////
// Test for runInThreadTestFunc()
///////////////////////////////////////////////////////////////////////////

struct RunInThreadData {
  RunInThreadData(int numThreads, int opsPerThread)
    : opsPerThread(opsPerThread)
    , opsToGo(numThreads*opsPerThread) {}

  TEventBase evb;
  deque< pair<int, int> > values;

  int opsPerThread;
  int opsToGo;
};

struct RunInThreadArg {
  RunInThreadArg(RunInThreadData* data,
                 int threadId,
                 int value)
    : data(data)
    , thread(threadId)
    , value(value) {}

  RunInThreadData* data;
  int thread;
  int value;
};

void runInThreadTestFunc(RunInThreadArg* arg) {
  arg->data->values.push_back(make_pair(arg->thread, arg->value));
  RunInThreadData* data = arg->data;
  delete arg;

  if(--data->opsToGo == 0) {
    // Break out of the event base loop if we are the last thread running
    data->evb.terminateLoopSoon();
  }
}

class RunInThreadTester : public concurrency::Runnable {
 public:
  RunInThreadTester(int id, RunInThreadData* data) : id_(id), data_(data) {}

  void run() {
    // Call evb->runInThread() a number of times
    {
      for (int n = 0; n < data_->opsPerThread; ++n) {
        RunInThreadArg* arg = new RunInThreadArg(data_, id_, n);
        data_->evb.runInEventBaseThread(runInThreadTestFunc, arg);
        usleep(10);
      }
    }
  }

 private:
  int id_;
  RunInThreadData* data_;
};

BOOST_AUTO_TEST_CASE(RunInThread) {
  uint32_t numThreads = 50;
  uint32_t opsPerThread = 100;
  RunInThreadData data(numThreads, opsPerThread);

  PosixThreadFactory threadFactory;
  threadFactory.setDetached(false);
  deque< std::shared_ptr<Thread> > threads;
  for (int n = 0; n < numThreads; ++n) {
    std::shared_ptr<RunInThreadTester> runner(new RunInThreadTester(n, &data));
    std::shared_ptr<Thread> thread = threadFactory.newThread(runner);
    threads.push_back(thread);
    thread->start();
  }

  // Add a timeout event to run after 3 seconds.
  // Otherwise loop() will return immediately since there are no events to run.
  // Once the last thread exits, it will stop the loop().  However, this
  // timeout also stops the loop in case there is a bug performing the normal
  // stop.
  data.evb.runAfterDelay(std::bind(&TEventBase::terminateLoopSoon, &data.evb),
                         3000);

  TimePoint start;
  data.evb.loop();
  TimePoint end;

  // Verify that the loop exited because all threads finished and requested it
  // to stop.  This should happen much sooner than the 3 second timeout.
  // Assert that it happens in under a second.  (This is still tons of extra
  // padding.)
  int64_t timeTaken = end.getTime() - start.getTime();
  BOOST_CHECK_LT(timeTaken, 1000);
  VLOG(11) << "Time taken: " << timeTaken;

  // Verify that we have all of the events from every thread
  int expectedValues[numThreads];
  for (int n = 0; n < numThreads; ++n) {
    expectedValues[n] = 0;
  }
  for (deque< pair<int, int> >::const_iterator it = data.values.begin();
       it != data.values.end();
       ++it) {
    int threadID = it->first;
    int value = it->second;
    BOOST_CHECK_EQUAL(expectedValues[threadID], value);
    ++expectedValues[threadID];
  }
  for (int n = 0; n < numThreads; ++n) {
    BOOST_CHECK_EQUAL(expectedValues[n], opsPerThread);
  }

  // Wait on all of the threads.  Otherwise we can exit and clean up
  // RunInThreadData before the last thread exits, while it is still holding
  // the RunInThreadData's mutex.
  for (deque< std::shared_ptr<Thread> >::const_iterator it = threads.begin();
       it != threads.end();
       ++it) {
    (*it)->join();
  }
}

///////////////////////////////////////////////////////////////////////////
// Tests for runInLoop()
///////////////////////////////////////////////////////////////////////////

class CountedLoopCallback : public TEventBase::LoopCallback {
 public:
  CountedLoopCallback(TEventBase* eventBase,
                      unsigned int count,
                      std::function<void()> action =
                        std::function<void()>())
    : eventBase_(eventBase)
    , count_(count)
    , action_(action) {}

  virtual void runLoopCallback() noexcept {
    --count_;
    if (count_ > 0) {
      eventBase_->runInLoop(this);
    } else if (action_) {
      action_();
    }
  }

  unsigned int getCount() const {
    return count_;
  }

 private:
  TEventBase* eventBase_;
  unsigned int count_;
  std::function<void()> action_;
};

// Test that TEventBase::loop() doesn't exit while there are
// still LoopCallbacks remaining to be invoked.
BOOST_AUTO_TEST_CASE(RepeatedRunInLoop) {
  TEventBase eventBase;

  CountedLoopCallback c(&eventBase, 10);
  eventBase.runInLoop(&c);
  // The callback shouldn't have run immediately
  BOOST_CHECK_EQUAL(c.getCount(), 10);
  eventBase.loop();

  // loop() should loop until the CountedLoopCallback stops
  // re-installing itself.
  BOOST_CHECK_EQUAL(c.getCount(), 0);
}

// Test runInLoop() calls with terminateLoopSoon()
BOOST_AUTO_TEST_CASE(RunInLoopStopLoop) {
  TEventBase eventBase;

  CountedLoopCallback c1(&eventBase, 20);
  CountedLoopCallback c2(&eventBase, 10,
                         std::bind(&TEventBase::terminateLoopSoon, &eventBase));

  eventBase.runInLoop(&c1);
  eventBase.runInLoop(&c2);
  BOOST_CHECK_EQUAL(c1.getCount(), 20);
  BOOST_CHECK_EQUAL(c2.getCount(), 10);

  eventBase.loopForever();

  // c2 should have stopped the loop after 10 iterations
  BOOST_CHECK_EQUAL(c2.getCount(), 0);

  // We allow the TEventBase to run the loop callbacks in whatever order it
  // chooses.  We'll accept c1's count being either 10 (if the loop terminated
  // after c1 ran on the 10th iteration) or 11 (if c2 terminated the loop
  // before c1 ran).
  //
  // (With the current code, c1 will always run 10 times, but we don't consider
  // this a hard API requirement.)
  BOOST_CHECK_GE(c1.getCount(), 10);
  BOOST_CHECK_LE(c1.getCount(), 11);
}

// Test cancelling runInLoop() callbacks
BOOST_AUTO_TEST_CASE(CancelRunInLoop) {
  TEventBase eventBase;

  CountedLoopCallback c1(&eventBase, 20);
  CountedLoopCallback c2(&eventBase, 20);
  CountedLoopCallback c3(&eventBase, 20);

  std::function<void()> cancelC1Action =
    std::bind(&TEventBase::LoopCallback::cancelLoopCallback, &c1);
  std::function<void()> cancelC2Action =
    std::bind(&TEventBase::LoopCallback::cancelLoopCallback, &c2);

  CountedLoopCallback cancelC1(&eventBase, 10, cancelC1Action);
  CountedLoopCallback cancelC2(&eventBase, 10, cancelC2Action);

  // Install cancelC1 after c1
  eventBase.runInLoop(&c1);
  eventBase.runInLoop(&cancelC1);

  // Install cancelC2 before c2
  eventBase.runInLoop(&cancelC2);
  eventBase.runInLoop(&c2);

  // Install c3
  eventBase.runInLoop(&c3);

  BOOST_CHECK_EQUAL(c1.getCount(), 20);
  BOOST_CHECK_EQUAL(c2.getCount(), 20);
  BOOST_CHECK_EQUAL(c3.getCount(), 20);
  BOOST_CHECK_EQUAL(cancelC1.getCount(), 10);
  BOOST_CHECK_EQUAL(cancelC2.getCount(), 10);

  // Run the loop
  eventBase.loop();

  // cancelC1 and cancelC3 should have both fired after 10 iterations and
  // stopped re-installing themselves
  BOOST_CHECK_EQUAL(cancelC1.getCount(), 0);
  BOOST_CHECK_EQUAL(cancelC2.getCount(), 0);
  // c3 should have continued on for the full 20 iterations
  BOOST_CHECK_EQUAL(c3.getCount(), 0);

  // c1 and c2 should have both been cancelled on the 10th iteration.
  //
  // Callbacks are always run in the order they are installed,
  // so c1 should have fired 10 times, and been canceled after it ran on the
  // 10th iteration.  c2 should have only fired 9 times, because cancelC2 will
  // have run before it on the 10th iteration, and cancelled it before it
  // fired.
  BOOST_CHECK_EQUAL(c1.getCount(), 10);
  BOOST_CHECK_EQUAL(c2.getCount(), 11);
}

class TerminateTestCallback : public TEventBase::LoopCallback,
                              public TEventHandler {
 public:
  TerminateTestCallback(TEventBase* eventBase, int fd)
    : TEventHandler(eventBase, fd),
      eventBase_(eventBase),
      loopInvocations_(0),
      maxLoopInvocations_(0),
      eventInvocations_(0),
      maxEventInvocations_(0) {}

  void reset(uint32_t maxLoopInvocations, uint32_t maxEventInvocations) {
    loopInvocations_ = 0;
    maxLoopInvocations_ = maxLoopInvocations;
    eventInvocations_ = 0;
    maxEventInvocations_ = maxEventInvocations;

    cancelLoopCallback();
    unregisterHandler();
  }

  virtual void handlerReady(uint16_t events) noexcept {
    // We didn't register with PERSIST, so we will have been automatically
    // unregistered already.
    BOOST_REQUIRE(!isHandlerRegistered());

    ++eventInvocations_;
    if (eventInvocations_ >= maxEventInvocations_) {
      return;
    }

    eventBase_->runInLoop(this);
  }
  virtual void runLoopCallback() noexcept {
    ++loopInvocations_;
    if (loopInvocations_ >= maxLoopInvocations_) {
      return;
    }

    registerHandler(READ);
  }

  uint32_t getLoopInvocations() const {
    return loopInvocations_;
  }
  uint32_t getEventInvocations() const {
    return eventInvocations_;
  }

 private:
  TEventBase* eventBase_;
  uint32_t loopInvocations_;
  uint32_t maxLoopInvocations_;
  uint32_t eventInvocations_;
  uint32_t maxEventInvocations_;
};

/**
 * Test that TEventBase::loop() correctly detects when there are no more events
 * left to run.
 *
 * This uses a single callback, which alternates registering itself as a loop
 * callback versus a TEventHandler callback.  This exercises a regression where
 * TEventBase::loop() incorrectly exited if there were no more fd handlers
 * registered, but a loop callback installed a new fd handler.
 */
BOOST_AUTO_TEST_CASE(LoopTermination) {
  TEventBase eventBase;

  // Open a pipe and close the write end,
  // so the read endpoint will be readable
  int pipeFds[2];
  int rc = pipe(pipeFds);
  BOOST_CHECK_EQUAL(rc, 0);
  close(pipeFds[1]);
  TerminateTestCallback callback(&eventBase, pipeFds[0]);

  // Test once where the callback will exit after a loop callback
  callback.reset(10, 100);
  eventBase.runInLoop(&callback);
  eventBase.loop();
  BOOST_CHECK_EQUAL(callback.getLoopInvocations(), 10);
  BOOST_CHECK_EQUAL(callback.getEventInvocations(), 9);

  // Test once where the callback will exit after an fd event callback
  callback.reset(100, 7);
  eventBase.runInLoop(&callback);
  eventBase.loop();
  BOOST_CHECK_EQUAL(callback.getLoopInvocations(), 7);
  BOOST_CHECK_EQUAL(callback.getEventInvocations(), 7);

  close(pipeFds[0]);
}

///////////////////////////////////////////////////////////////////////////
// Tests for latency calculations
///////////////////////////////////////////////////////////////////////////

class IdleTimeTimeoutSeries : public TAsyncTimeout {

 public:

  explicit IdleTimeTimeoutSeries(TEventBase *base,
                                 std::deque<std::uint64_t>& timeout) :
    TAsyncTimeout(base),
    base_(base),
    timeouts_(0),
    timeout_(timeout) {
      scheduleTimeout(1);
    }

  virtual ~IdleTimeTimeoutSeries() {}

  void timeoutExpired() noexcept {
    ++timeouts_;

    if(timeout_.empty()){
      cancelTimeout();
    } else {
      uint64_t sleepTime = timeout_.front();
      timeout_.pop_front();
      if (sleepTime) {
        usleep(sleepTime);
      }
      scheduleTimeout(1);
    }
  }

  int getTimeouts() const {
    return timeouts_;
  }

 private:
  TEventBase *base_;
  int timeouts_;
  std::deque<uint64_t>& timeout_;
};

/**
 * Verify that idle time is correctly accounted for when decaying our loop
 * time.
 *
 * This works by creating a high loop time (via usleep), expecting a latency
 * callback with known value, and then scheduling a timeout for later. This
 * later timeout is far enough in the future that the idle time should have
 * caused the loop time to decay.
 */
BOOST_AUTO_TEST_CASE(IdleTime) {
  TEventBase eventBase;
  eventBase.setLoadAvgMsec(1000);
  eventBase.resetLoadAvg(5900.0);
  std::deque<uint64_t> timeouts0(4, 8080);
  timeouts0.push_front(8000);
  timeouts0.push_back(14000);
  IdleTimeTimeoutSeries tos0(&eventBase, timeouts0);
  std::deque<uint64_t> timeouts(20, 20);
  std::unique_ptr<IdleTimeTimeoutSeries> tos;

  int latencyCallbacks = 0;
  eventBase.setMaxLatency(6000, [&]() {
    ++latencyCallbacks;

    switch (latencyCallbacks) {
    case 1:
      BOOST_CHECK_EQUAL(6, tos0.getTimeouts());
      BOOST_CHECK_CLOSE(6100, eventBase.getAvgLoopTime(), 2);
      tos.reset(new IdleTimeTimeoutSeries(&eventBase, timeouts));
      break;

    default:
      BOOST_CHECK_MESSAGE(false, "Unexpected latency callback");
      break;
    }
  });

  // Kick things off with an "immedite" timeout
  tos0.scheduleTimeout(1);

  eventBase.loop();

  BOOST_CHECK_EQUAL(1, latencyCallbacks);
  BOOST_CHECK_EQUAL(7, tos0.getTimeouts());
  BOOST_CHECK_CLOSE(5900, eventBase.getAvgLoopTime(), 2);
  BOOST_CHECK_EQUAL(21, tos->getTimeouts());
}

/**
 * Test that thisLoop functionality works with terminateLoopSoon
 */
BOOST_AUTO_TEST_CASE(ThisLoop) {
  TEventBase eb;
  bool runInLoop = false;
  bool runThisLoop = false;

  eb.runInLoop([&](){
      eb.terminateLoopSoon();
      eb.runInLoop([&]() {
          runInLoop = true;
        });
      eb.runInLoop([&]() {
          runThisLoop = true;
        }, true);
    }, true);
  eb.loopForever();

  // Should not work
  BOOST_CHECK_EQUAL(false, runInLoop);
  // Should work with thisLoop
  BOOST_CHECK_EQUAL(true, runThisLoop);
}

BOOST_AUTO_TEST_CASE(EventBaseThreadLoop) {
  TEventBase base;
  bool ran = false;

  base.runInEventBaseThread([&](){
    ran = true;
  });
  base.loop();

  BOOST_CHECK_EQUAL(true, ran);
}

BOOST_AUTO_TEST_CASE(EventBaseThreadName) {
  TEventBase base;
  base.setName("foo");
  base.loop();

#if (__GLIBC__ >= 2) && (__GLIBC_MINOR__ >= 12)
  char name[16];
  pthread_getname_np(pthread_self(), name, 16);
  BOOST_CHECK_EQUAL(0, strcmp("foo", name));
#endif
}

///////////////////////////////////////////////////////////////////////////
// init_unit_test_suite()
///////////////////////////////////////////////////////////////////////////

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value = "TEventBaseTest";

  if (argc != 1) {
    cerr << "error: unhandled arguments:";
    for (int n = 1; n < argc; ++n) {
      cerr << " " << argv[n];
    }
    cerr << endl;
    exit(1);
  }

  return nullptr;
}
