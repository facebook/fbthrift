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
#include <signal.h>

#include <boost/test/unit_test.hpp>
#include <boost/random.hpp>

#include "thrift/lib/cpp/async/TAsyncSocket.h"
#include "thrift/lib/cpp/async/TAsyncTimeout.h"
#include "thrift/lib/cpp/async/TBinaryAsyncChannel.h"
#include "thrift/lib/cpp/async/TEventBase.h"
#include "thrift/lib/cpp/async/TFramedAsyncChannel.h"
#include "thrift/lib/cpp/concurrency/Util.h"
#include "thrift/lib/cpp/test/SocketPair.h"
#include "thrift/lib/cpp/test/TimeUtil.h"

using std::string;
using std::vector;
using std::cerr;
using std::endl;
using namespace boost;

using apache::thrift::async::TAsyncChannel;
using apache::thrift::async::TAsyncSocket;
using apache::thrift::async::TAsyncTimeout;
using apache::thrift::async::TBinaryAsyncChannel;
using apache::thrift::async::TEventBase;
using apache::thrift::async::TFramedAsyncChannel;
using apache::thrift::protocol::TBinaryProtocolT;
using apache::thrift::transport::TBufferBase;
using apache::thrift::transport::TFramedTransport;
using apache::thrift::transport::TMemoryBuffer;
using apache::thrift::transport::TTransportException;

using apache::thrift::test::SocketPair;
using apache::thrift::test::TimePoint;


///////////////////////////////////////////////////////////////////////////
// Utility code
//////////////////////////////////////////////////////////////////////////


class ChannelCallback {
 public:
  ChannelCallback()
    : sendDone_(0)
    , sendError_(0)
    , recvDone_(0)
    , recvError_(0)
    , timestamp_(false) {}

  void send(const std::shared_ptr<TAsyncChannel>& channel,
            TMemoryBuffer* message) {
    // helper function to get the correct callbacks
    channel->sendMessage(std::bind(&ChannelCallback::sendDone, this),
                         std::bind(&ChannelCallback::sendError, this), message);
  }

  void recv(const std::shared_ptr<TAsyncChannel>& channel,
            TMemoryBuffer* message) {
    // helper function to get the correct callbacks
    channel->recvMessage(std::bind(&ChannelCallback::recvDone, this),
                         std::bind(&ChannelCallback::recvError, this), message);
  }

  void sendDone() {
    ++sendDone_;
    timestamp_.reset();
  }
  void sendError() {
    ++sendError_;
    timestamp_.reset();
  }

  void recvDone() {
    ++recvDone_;
    timestamp_.reset();
  }
  void recvError() {
    ++recvError_;
    timestamp_.reset();
  }

  uint32_t getSendDone() const {
    return sendDone_;
  }
  uint32_t getSendError() const {
    return sendError_;
  }
  uint32_t getRecvDone() const {
    return recvDone_;
  }
  uint32_t getRecvError() const {
    return recvError_;
  }

  const TimePoint& getTimestamp() const {
    return timestamp_;
  }

 private:
  uint32_t sendDone_;
  uint32_t sendError_;
  uint32_t recvDone_;
  uint32_t recvError_;
  TimePoint timestamp_;
};

class Message {
 public:
  explicit Message(uint32_t len, bool addFrame = false)
    : framed_(addFrame)
    , buf_(new TMemoryBuffer(len + kPadding)) {
    init(len);
  }

  void init(uint32_t len) {
    string value;
    randomizeString(&value, len);

    // Generate a valid thrift TBinaryProtocol message,
    // containing a randomly generated string of the specified length
    std::shared_ptr<TBufferBase> transport;
    if (framed_) {
      transport.reset(new TFramedTransport(buf_));
    } else {
      transport = buf_;
    }
    TBinaryProtocolT<TBufferBase> prot(transport);

    prot.writeMessageBegin("foo", apache::thrift::protocol::T_CALL, 0);
    prot.writeStructBegin("bar");

    prot.writeFieldBegin("value", apache::thrift::protocol::T_STRING, 1);
    prot.writeString(value);
    prot.writeFieldEnd();

    prot.writeFieldStop();
    prot.writeStructEnd();
    prot.writeMessageEnd();

    transport->writeEnd();
    transport->flush();
  }

  void copyTo(TMemoryBuffer* membuf) {
    memcpy(membuf->getWritePtr(getLength()), getBuffer(), getLength());
    membuf->wroteBytes(getLength());
  }

  void checkEqual(TMemoryBuffer* membuf) const {
    // If we have a frame header, skip it when comparing
    uint32_t length = getLength();
    const uint8_t* myBuf = getBuffer();
    if (framed_) {
      length -= sizeof(uint32_t);
      myBuf += sizeof(uint32_t);
    }

    BOOST_CHECK_EQUAL(membuf->available_read(), length);

    uint32_t borrowLen = length;
    const uint8_t* otherBuf = membuf->borrow(nullptr, &borrowLen);
    BOOST_REQUIRE(otherBuf != nullptr);
    BOOST_REQUIRE_EQUAL(borrowLen, length);

    BOOST_CHECK_EQUAL(memcmp(otherBuf, myBuf, length), 0);
  }

  uint32_t getLength() const {
    return buf_->available_read();
  }

  const uint8_t* getBuffer() const {
    uint32_t borrowLen = getLength();
    const uint8_t* buf = buf_->borrow(nullptr, &borrowLen);
    assert(buf != nullptr);
    return buf;
  }

 private:
  void randomizeString(string* ret, uint32_t len) {
    ret->resize(len);

    // Randomly initialize the string
    // TODO: we don't currently seed the RNG.
    boost::mt19937 rng;
    for (uint32_t n = 0; n < len; ++n) {
      // The RNG gives us more than 1 byte of randomness, but
      // we don't currently take advantage of it.  It's easier for now
      // just to proceed 1 byte at a time.
      (*ret)[n] = rng();
    }
  }

  static uint32_t const kPadding = 256; // extra room for serialization overhead

  bool framed_;
  std::shared_ptr<TMemoryBuffer> buf_;
};


struct ChunkInfo {
  ChunkInfo(int b, int d)
    : bytes(b)
    , delayMS(d) {}

  int bytes;
  int delayMS;
};

class ChunkSchedule : public vector<ChunkInfo> {
 public:
  /*
   * Constructor to allow ChunkSchedules to be easily specified
   * in the code.  The arguments are parsed in pairs as (bytes, delay).
   *
   * The arguments must end with a pair that has a non-positive bytes value.
   * A negative bytes value means to send the rest of the data, a value of 0
   * means to close the socket.
   */
  ChunkSchedule(int bytes, int delayMS, ...) {
    push_back(ChunkInfo(bytes, delayMS));

    if (bytes <= 0)
      return;

    va_list ap;
    va_start(ap, delayMS);

    while (true) {
      int b = va_arg(ap, int);
      int d = va_arg(ap, int);

      push_back(ChunkInfo(b, d));
      if (b <= 0) {
        break;
      }
    }

    va_end(ap);
  }
};

class ChunkSender : private TAsyncSocket::WriteCallback,
                    private TAsyncTimeout {
 public:
  ChunkSender(TEventBase* evb, TAsyncSocket* socket,
              const Message* msg, const ChunkSchedule& schedule)
    : TAsyncTimeout(evb)
    , bufOffset_(0)
    , scheduleIndex_(0)
    , currentChunkLen_(0)
    , error_(false)
    , eventBase_(evb)
    , socket_(socket)
    , message_(msg)
    , schedule_(schedule) {
  }

  void start() {
    scheduleNext();
  }

  const ChunkSchedule* getSchedule() const {
    return &schedule_;
  }

  bool error() const {
    return error_;
  }

 private:
  void scheduleNext() {
    assert(scheduleIndex_ < schedule_.size());
    const ChunkInfo& info = schedule_[scheduleIndex_];
    if (info.delayMS <= 0) {
      sendNow();
    } else {
      scheduleTimeout(info.delayMS);
    }
  }

  void sendNow() {
    assert(scheduleIndex_ < schedule_.size());
    const ChunkInfo& info = schedule_[scheduleIndex_];

    assert(bufOffset_ <= message_->getLength());

    uint32_t len;
    if (info.bytes == 0) {
      // close the socket
      socket_->close();
      return;
    } else if (info.bytes < 0) {
      // write the rest of the data
      if (bufOffset_ >= message_->getLength()) {
        // nothing more to write
        return;
      }
      len = message_->getLength() - bufOffset_;
    } else {
      len = info.bytes;
      if (len + bufOffset_ > message_->getLength()) {
        // bug in the test code: ChunkSchedule lists more data than available
        BOOST_FAIL("bad ChunkSchedule");

        len = message_->getLength() - bufOffset_;
        if (len == 0) {
          ++scheduleIndex_;
          if (scheduleIndex_ < schedule_.size()) {
            scheduleNext();
          }
          return;
        }
      }
    }

    currentChunkLen_ = len;
    const uint8_t* buf = message_->getBuffer() + bufOffset_;
    socket_->write(this, buf, len);
  }

  void writeSuccess() noexcept {
    bufOffset_ += currentChunkLen_;

    ++scheduleIndex_;
    if (scheduleIndex_ < schedule_.size()) {
      scheduleNext();
    }
  }

  void writeError(size_t bytesWritten,
                  const TTransportException& ex) noexcept {
    error_ = true;
  }

  virtual void timeoutExpired() noexcept {
    sendNow();
  }

  uint32_t bufOffset_;
  uint32_t scheduleIndex_;
  uint32_t currentChunkLen_;
  bool error_;
  TEventBase* eventBase_;
  TAsyncSocket* socket_;
  const Message* message_;
  ChunkSchedule schedule_;
};

class MultiMessageSize : public vector<int> {
  public:
    MultiMessageSize(int len, ...) {
      push_back(len);

      va_list ap;
      va_start(ap, len);

      while (true) {
        int b = va_arg(ap, int);
        if (b <= 0) {
          break;
        } else {
          push_back(b);
        }
      }

      va_end(ap);
    }
};

class MultiMessageSenderReceiver : private TAsyncSocket::WriteCallback,
                                   private TAsyncTimeout {
  public:
    MultiMessageSenderReceiver(TEventBase* evb,
                               TAsyncSocket* socket,
                               const MultiMessageSize& multiMessage,
                               bool framed,
                               uint32_t writeTimes,
                               bool queued = false,
                               int delayMS = 2)
    : TAsyncTimeout(evb)
    , writeError_(false)
    , readError_(false)
    , eventBase_(evb)
    , socket_(socket)
    , queued_(queued)
    , delayMS_(delayMS) {
      uint32_t totalSize = 0;
      for (vector<int>::const_iterator it = multiMessage.begin();
           it != multiMessage.end();
           it++) {
        Message message(*it, framed);
        writeMessages_.push_back(message);
        writeMemoryBuffer_.write(message.getBuffer(), message.getLength());
        totalSize += message.getLength();
      }

      assert(writeMessages_.size() > 0);
      writeSize_ = totalSize / writeTimes;
    }

    void initialize(const std::shared_ptr<TAsyncChannel>& channel) {
      int n_msgs = writeMessages_.size();
      uint32_t n_recvs = (queued_) ? n_msgs : 1;
      for (uint32_t i = 0; i < n_recvs; i++) {
        channel->recvMessage(
          std::bind(&MultiMessageSenderReceiver::recvDone, this),
          std::bind(&MultiMessageSenderReceiver::recvError, this),
          &readMemoryBuffer_);
      }
      scheduleNext();
      recvChannel_ = channel;
    }

    bool getReadError() const {
      return readError_;
    }

    bool getWriteError() const {
      return writeError_;
    }

    vector<std::shared_ptr<TMemoryBuffer> >& getReadBuffers() {
      return readBuffers_;
    }

    vector<Message>& getWriteMessages() {
      return writeMessages_;
    }

    void recvDone() {
      uint8_t* request;
      uint32_t requestLen;
      readMemoryBuffer_.extractReadBuffer(&request, &requestLen);
      if (requestLen > 0) {
        std::shared_ptr<TMemoryBuffer> recvBuffer(new TMemoryBuffer(
                                  request,
                                  requestLen,
                                  TMemoryBuffer::TAKE_OWNERSHIP));
        readBuffers_.push_back(recvBuffer);
      } else if (request) {
        delete request;
      }

      // Read another message if we haven't read all of the messages yet
      if (!queued_ && readBuffers_.size() < writeMessages_.size()) {
        recvChannel_->recvMessage(
            std::bind(&MultiMessageSenderReceiver::recvDone, this),
            std::bind(&MultiMessageSenderReceiver::recvError, this),
            &readMemoryBuffer_);
      }
    }

    void recvError() {
      readError_ = true;
    }

    void writeSuccess() noexcept {
      uint32_t sentSize = std::min(writeSize_,
                                   writeMemoryBuffer_.available_read());
      writeMemoryBuffer_.consume(sentSize);
      if (writeMemoryBuffer_.available_read() > 0) {
        scheduleNext();
      }
    }

    void writeError(size_t bytesWritten,
                  const TTransportException& ex) noexcept {
      writeError_ = true;
    }

 private:
   void scheduleNext() {
     if (delayMS_ <= 0) {
       send();
     } else {
       scheduleTimeout(delayMS_);
     }
   }

   virtual void timeoutExpired() noexcept {
     send();
   }

  void send() {
    uint32_t availableSize = writeMemoryBuffer_.available_read();
    const uint8_t* sendBufPtr = writeMemoryBuffer_.borrow(nullptr,
                                                          &availableSize);
    uint32_t sendSize = std::min(writeSize_, availableSize);
    if (sendSize > 0) {
      socket_->write(this, sendBufPtr, sendSize);
    }
  }

  bool writeError_;
  bool readError_;
  TEventBase* eventBase_;
  TAsyncSocket* socket_;
  vector<Message> writeMessages_;
  vector<std::shared_ptr<TMemoryBuffer> > readBuffers_;
  TMemoryBuffer writeMemoryBuffer_;
  TMemoryBuffer readMemoryBuffer_;
  uint32_t writeSize_;
  bool queued_;
  int delayMS_;
  std::shared_ptr<TAsyncChannel> recvChannel_;
};

class EventBaseAborter : public TAsyncTimeout {
 public:
  EventBaseAborter(TEventBase* eventBase, uint32_t timeoutMS)
    : TAsyncTimeout(eventBase,
                    TAsyncTimeout::InternalEnum::INTERNAL)
    , eventBase_(eventBase) {
    scheduleTimeout(timeoutMS);
  }

  virtual void timeoutExpired() noexcept {
    BOOST_FAIL("test timed out");
    eventBase_->terminateLoopSoon();
  }

 private:
  TEventBase* eventBase_;
};

template<typename ChannelT>
class SocketPairTest {
 public:
  SocketPairTest()
    : eventBase_()
    , socketPair_() {
    socket0_ = TAsyncSocket::newSocket(&eventBase_, socketPair_[0]);
    socketPair_.extractFD0();

    socket1_ = TAsyncSocket::newSocket(&eventBase_, socketPair_[1]);
    socketPair_.extractFD1();

    channel0_ = ChannelT::newChannel(socket0_);
    channel1_ = ChannelT::newChannel(socket1_);
  }
  virtual ~SocketPairTest() {}

  void loop(uint32_t timeoutMS = 3000) {
    EventBaseAborter aborter(&eventBase_, timeoutMS);
    eventBase_.loop();
  }

  virtual void run() {
    runWithTimeout(3000);
  }

  virtual void runWithTimeout(uint32_t timeoutMS) {
    preLoop();
    loop(timeoutMS);
    postLoop();
  }

  virtual void preLoop() {}
  virtual void postLoop() {}

 protected:
  TEventBase eventBase_;
  SocketPair socketPair_;
  std::shared_ptr<TAsyncSocket> socket0_;
  std::shared_ptr<TAsyncSocket> socket1_;
  std::shared_ptr<ChannelT> channel0_;
  std::shared_ptr<ChannelT> channel1_;
};

template<typename ChannelT>
class NeedsFrame {
};

template<>
class NeedsFrame<TFramedAsyncChannel> {
 public:
  static bool value() {
    return true;
  }
};

template<>
class NeedsFrame<TBinaryAsyncChannel> {
 public:
  static bool value() {
    return false;
  }
};

///////////////////////////////////////////////////////////////////////////
// Test cases
//////////////////////////////////////////////////////////////////////////

template<typename ChannelT>
class SendRecvTest : public SocketPairTest<ChannelT> {
 public:
  explicit SendRecvTest(uint32_t msgLen)
    : msg_(msgLen) {
  }

  void preLoop() {
    msg_.copyTo(&sendBuf_);
    sendCallback_.send(this->channel0_, &sendBuf_);
    recvCallback_.recv(this->channel1_, &recvBuf_);
  }

  void postLoop() {
    BOOST_CHECK_EQUAL(sendCallback_.getSendError(), 0);
    BOOST_CHECK_EQUAL(sendCallback_.getSendDone(), 1);
    BOOST_CHECK_EQUAL(recvCallback_.getRecvError(), 0);
    BOOST_CHECK_EQUAL(recvCallback_.getRecvDone(), 1);
    msg_.checkEqual(&recvBuf_);
  }

 private:
  Message msg_;
  TMemoryBuffer sendBuf_;
  TMemoryBuffer recvBuf_;
  ChannelCallback sendCallback_;
  ChannelCallback recvCallback_;
};

BOOST_AUTO_TEST_CASE(TestSendRecvFramed) {
  SendRecvTest<TFramedAsyncChannel>(1).run();
  SendRecvTest<TFramedAsyncChannel>(100).run();
  SendRecvTest<TFramedAsyncChannel>(1024*1024).run();
}

BOOST_AUTO_TEST_CASE(TestSendRecvBinary) {
  SendRecvTest<TBinaryAsyncChannel>(1).run();
  SendRecvTest<TBinaryAsyncChannel>(100).run();
  SendRecvTest<TBinaryAsyncChannel>(1024*1024).run();
}

template<typename ChannelT>
class MultiSendRecvTest : public SocketPairTest<ChannelT> {
  public:
    MultiSendRecvTest(const MultiMessageSize& multiMessage,
                      uint32_t writeTimes,
                      bool queued = false,
                      int delayMS = 0)
    : multiMessageSenderReceiver_(&this->eventBase_,
                                  this->socket0_.get(),
                                  multiMessage,
                                  NeedsFrame<ChannelT>::value(),
                                  writeTimes,
                                  queued,
                                  delayMS) {
  }

  void preLoop() {
    multiMessageSenderReceiver_.initialize(this->channel1_);
  }

  void postLoop() {
    BOOST_CHECK_EQUAL(multiMessageSenderReceiver_.getReadError(), false);
    BOOST_CHECK_EQUAL(multiMessageSenderReceiver_.getWriteError(), false);

    vector<std::shared_ptr<TMemoryBuffer> >& readBuffers
                      = multiMessageSenderReceiver_.getReadBuffers();
    vector<Message>& writeMessages
                      = multiMessageSenderReceiver_.getWriteMessages();
    BOOST_CHECK_EQUAL(readBuffers.size(), writeMessages.size());
    for (int i = 0; i < writeMessages.size(); i++) {
      writeMessages[i].checkEqual(readBuffers[i].get());
    }
  }

  private:
    MultiMessageSenderReceiver multiMessageSenderReceiver_;
};

BOOST_AUTO_TEST_CASE(TestMultiSendRecvBinary) {
  typedef MultiSendRecvTest<TBinaryAsyncChannel> MultiSendRecvBinaryTest;

  //size below 1024 for each message
  MultiMessageSize sizes(911, 911, 911, -1);

  // each time send one whole message below 1024
  MultiSendRecvBinaryTest(sizes, 3, 0).run();
  MultiSendRecvBinaryTest(sizes, 3, 2).run();

  // send all messages for one time
  MultiSendRecvBinaryTest(sizes, 1, 0).run();

  // each time send one and half message.
  MultiSendRecvBinaryTest(sizes, 2).run();
  MultiSendRecvBinaryTest(sizes, 2, 2).run();

  //size above 1024 for each message
  MultiMessageSize bigSizes(1911 * 1911,
                            1911 * 1911,
                            1911 * 1911,
                            -1);

  // each time send one whole message above 1024
  MultiSendRecvBinaryTest(bigSizes, 3, 0).run();
  MultiSendRecvBinaryTest(bigSizes, 3, 2).run();

  // send all messages for one time
  MultiSendRecvBinaryTest(bigSizes, 1, 0).run();

  // each time send one and half message
  MultiSendRecvBinaryTest(bigSizes, 2).run();
  MultiSendRecvBinaryTest(bigSizes, 2, 2).run();
}

BOOST_AUTO_TEST_CASE(TestMultiSendRecvBinaryQueued) {
  typedef MultiSendRecvTest<TBinaryAsyncChannel> MultiSendRecvBinaryTest;

  //size below 1024 for each message
  MultiMessageSize sizes(911, 911, 911, -1);

  // each time send one whole message below 1024
  MultiSendRecvBinaryTest(sizes, 3, true, 0).run();
  MultiSendRecvBinaryTest(sizes, 3, true, 2).run();

  // send all messages for one time
  MultiSendRecvBinaryTest(sizes, 1, true, 0).run();

  // each time send one and half message.
  MultiSendRecvBinaryTest(sizes, 2, true).run();
  MultiSendRecvBinaryTest(sizes, 2, true, 2).run();

  //size above 1024 for each message
  MultiMessageSize bigSizes(1911 * 1911,
                            1911 * 1911,
                            1911 * 1911,
                            -1);

  // each time send one whole message above 1024
  MultiSendRecvBinaryTest(bigSizes, 3, true, 0).run();
  MultiSendRecvBinaryTest(bigSizes, 3, true, 2).run();

  // send all messages for one time
  MultiSendRecvBinaryTest(bigSizes, 1, true, 0).run();

  // each time send one and half message
  MultiSendRecvBinaryTest(bigSizes, 2, true).run();
  MultiSendRecvBinaryTest(bigSizes, 2, true, 2).run();
}

BOOST_AUTO_TEST_CASE(TestMultiSendRecvFramed) {
  typedef MultiSendRecvTest<TFramedAsyncChannel> MultiSendRecvFramedTest;

  //size below 1024 for each message
  MultiMessageSize sizes(911, 911, 911, -1);

  // each time send one whole message below 1024
  MultiSendRecvFramedTest(sizes, 3, 0).run();
  MultiSendRecvFramedTest(sizes, 3, 2).run();

  // send all messages for one time
  MultiSendRecvFramedTest(sizes, 1, 0).run();

  // each time send one and half message.
  MultiSendRecvFramedTest(sizes, 2).run();
  MultiSendRecvFramedTest(sizes, 2, 2).run();

  //size above 1024 for each message
  MultiMessageSize bigSizes(1911 * 1911,
                            1911 * 1911,
                            1911 * 1911,
                            -1);

  // each time send one whole message above 1024
  MultiSendRecvFramedTest(bigSizes, 3, 0).run();
  MultiSendRecvFramedTest(bigSizes, 3, 2).run();

  // send all messages for one time
  MultiSendRecvFramedTest(bigSizes, 1, 0).run();

  // each time send one and half message
  MultiSendRecvFramedTest(bigSizes, 2).run();
  MultiSendRecvFramedTest(bigSizes, 2, 2).run();
}


BOOST_AUTO_TEST_CASE(TestMultiSendRecvFramedQueued) {
  typedef MultiSendRecvTest<TFramedAsyncChannel> MultiSendRecvBinaryTest;

  //size below 1024 for each message
  MultiMessageSize sizes(911, 911, 911, -1);

  // each time send one whole message below 1024
  MultiSendRecvBinaryTest(sizes, 3, true, 0).run();
  MultiSendRecvBinaryTest(sizes, 3, true, 2).run();

  // send all messages for one time
  MultiSendRecvBinaryTest(sizes, 1, true, 0).run();

  // each time send one and half message.
  MultiSendRecvBinaryTest(sizes, 2, true).run();
  MultiSendRecvBinaryTest(sizes, 2, true, 2).run();

  //size above 1024 for each message
  MultiMessageSize bigSizes(1911 * 1911,
                            1911 * 1911,
                            1911 * 1911,
                            -1);

  // each time send one whole message above 1024
  MultiSendRecvBinaryTest(bigSizes, 3, true, 0).run();
  MultiSendRecvBinaryTest(bigSizes, 3, true, 2).run();

  // send all messages for one time
  MultiSendRecvBinaryTest(bigSizes, 1, true, 0).run();

  // each time send one and half message
  MultiSendRecvBinaryTest(bigSizes, 2, true).run();
  MultiSendRecvBinaryTest(bigSizes, 2, true, 2).run();
}

const int kRecvDelay = 200;
const int kTimeout = 50;

template<typename ChannelT>
class TimeoutQueuedTest : public SocketPairTest<ChannelT> {
 public:

  explicit TimeoutQueuedTest(int n_msgs = 3)
      : n_msgs_(n_msgs)
      , start_(false)
      , msg_(911) {
  }

  void preLoop() {

    this->channel1_->setRecvTimeout(kRecvDelay * n_msgs_ + kTimeout);

    for (int i = 0; i < n_msgs_; i++ ) {
      // queue some reads 200ms apart
      this->eventBase_.runAfterDelay(
        std::bind(&TimeoutQueuedTest<ChannelT>::recvMe, this),
        kRecvDelay * i);
    }

    this->eventBase_.runAfterDelay(
      std::bind(&TimeoutQueuedTest<ChannelT>::sendMe, this),
      kRecvDelay * n_msgs_);

  }

  void sendMe() {
    // Send one message to test that the timeout for queued reads
    // adjusts based time recv was called.  Also tests that all queued
    // readers receive errors on the first timeout
    msg_.copyTo(&sendBuf_);
    sendCallback_.send(this->channel0_, &sendBuf_);
    start_.reset();
  }

  void recvMe() {
    recvCallback_.recv(this->channel1_, &readMemoryBuffer_);
  }

  void postLoop() {
    BOOST_CHECK_EQUAL(recvCallback_.getRecvError(), 2);
    BOOST_CHECK_EQUAL(recvCallback_.getRecvDone(), 1);

    T_CHECK_TIMEOUT(start_, recvCallback_.getTimestamp(),
                    n_msgs_ * kRecvDelay + kTimeout);
  }

 private:
  uint32_t n_msgs_;
  TimePoint start_;
  Message msg_;
  TMemoryBuffer sendBuf_;
  TMemoryBuffer readMemoryBuffer_;
  ChannelCallback sendCallback_;
  ChannelCallback recvCallback_;
};


BOOST_AUTO_TEST_CASE(TestTimeoutQueued) {
  TimeoutQueuedTest<TFramedAsyncChannel>().run();
  TimeoutQueuedTest<TBinaryAsyncChannel>().run();
}


template<typename ChannelT>
class RecvChunksTest : public SocketPairTest<ChannelT> {
 public:
  explicit RecvChunksTest(const ChunkSchedule& schedule,
                          uint32_t timeout = 0,
                          uint32_t msgLen = 1024*1024)
    : start_(false)
    , timeout_(timeout)
    , msg_(msgLen, NeedsFrame<ChannelT>::value())
    , sender_(&this->eventBase_, this->socket0_.get(), &msg_, schedule) {
  }

  void preLoop() {
    if (timeout_ > 0) {
      this->channel1_->setRecvTimeout(timeout_);
    }
    start_.reset();
    recvCallback_.recv(this->channel1_, &recvBuf_);
    sender_.start();
  }

  void postLoop() {
    bool expectTimeout = false;
    int64_t expectedMS = 0;
    int64_t tolerance = 0;
    uint32_t expectedBytes = 0;
    for (ChunkSchedule::const_iterator it = sender_.getSchedule()->begin();
         it != sender_.getSchedule()->end();
         ++it) {
      // Allow 2ms of processing overhead for every scheduled event.
      tolerance += 2;

      if (0 < timeout_ && timeout_ < it->delayMS) {
        // We expect to time out waiting for this chunk of data
        expectedMS += timeout_;
        expectTimeout = true;
        break;
      }

      expectedMS += it->delayMS;
      if (it->bytes < 0) {
        // The full message should be written
        expectedBytes = msg_.getLength();
      } else {
        expectedBytes += it->bytes;
      }
    }

    // Unframed transports require many more read callbacks to fully read the
    // data.  Add extra tolerance for the overhead in this case.
    //
    // The number of calls is roughly log(size/4096) / log(1.5)
    // (Since the code starts with an initial buffer size of 4096, and grows by
    // a factor of 1.5 each time it reallocates.)  Add an extra millisecond of
    // tolerance for every expected call.
    if (!NeedsFrame<ChannelT>::value() && expectedBytes > 4096) {
      double numCalls = log(expectedBytes / 4096) / log(1.5);
      printf("expected %f calls for %u bytes\n", numCalls, expectedBytes);
      tolerance += static_cast<int64_t>(numCalls);
    }

    if (expectTimeout) {
      // We should time out after expectedMS
      BOOST_TEST_MESSAGE("RecvChunksTest: testing for timeout");
      BOOST_CHECK_EQUAL(sender_.error(), true);
      BOOST_CHECK_EQUAL(recvCallback_.getRecvError(), 1);
      BOOST_CHECK_EQUAL(recvCallback_.getRecvDone(), 0);
      BOOST_CHECK_EQUAL(this->channel1_->timedOut(), true);
      BOOST_CHECK_EQUAL(this->channel1_->error(), true);
      BOOST_CHECK_EQUAL(this->channel1_->good(), false);

      T_CHECK_TIMEOUT(start_, recvCallback_.getTimestamp(), expectedMS,
                      tolerance);
    } else if (expectedBytes == 0) {
      // We should get EOF after expectedMS, before any data was ever sent
      //
      // This seems like a weird special case.  TAsyncChannel calls the normal
      // callback in this case, even though no message was received.  Maybe we
      // should consider changing this TAsyncChannel behavior?
      BOOST_TEST_MESSAGE("RecvChunksTest: testing for EOF with no data");
      BOOST_CHECK_EQUAL(sender_.error(), false);
      BOOST_CHECK_EQUAL(recvCallback_.getRecvError(), 0);
      BOOST_CHECK_EQUAL(recvCallback_.getRecvDone(), 1);
      BOOST_CHECK_EQUAL(this->channel1_->timedOut(), false);
      BOOST_CHECK_EQUAL(this->channel1_->error(), false);
      BOOST_CHECK_EQUAL(this->channel1_->good(), false);
      BOOST_CHECK_EQUAL(recvBuf_.available_read(), 0);

      T_CHECK_TIMEOUT(start_, recvCallback_.getTimestamp(), expectedMS,
                      tolerance);
    } else if (expectedBytes < msg_.getLength()) {
      // We should get EOF after expectedMS
      BOOST_TEST_MESSAGE("RecvChunksTest: testing for EOF");
      BOOST_CHECK_EQUAL(sender_.error(), false);
      BOOST_CHECK_EQUAL(recvCallback_.getRecvError(), 1);
      BOOST_CHECK_EQUAL(recvCallback_.getRecvDone(), 0);
      BOOST_CHECK_EQUAL(this->channel1_->timedOut(), false);
      BOOST_CHECK_EQUAL(this->channel1_->error(), false);
      BOOST_CHECK_EQUAL(this->channel1_->good(), false);

      T_CHECK_TIMEOUT(start_, recvCallback_.getTimestamp(), expectedMS,
                      tolerance);
    } else {
      // We expect success after expectedMS
      BOOST_TEST_MESSAGE("RecvChunksTest: testing for success");
      BOOST_CHECK_EQUAL(sender_.error(), false);
      BOOST_CHECK_EQUAL(recvCallback_.getRecvError(), 0);
      BOOST_CHECK_EQUAL(recvCallback_.getRecvDone(), 1);
      BOOST_CHECK_EQUAL(this->channel1_->timedOut(), false);
      BOOST_CHECK_EQUAL(this->channel1_->error(), false);
      BOOST_CHECK_EQUAL(this->channel1_->good(), true);
      msg_.checkEqual(&recvBuf_);

      T_CHECK_TIMEOUT(start_, recvCallback_.getTimestamp(), expectedMS,
                      tolerance);
    }
  }

 private:
  TimePoint start_;
  uint32_t timeout_;
  Message msg_;
  ChunkSender sender_;
  TMemoryBuffer recvBuf_;
  ChannelCallback recvCallback_;
};


BOOST_AUTO_TEST_CASE(TestRecvFrameChunks) {
  typedef RecvChunksTest<TFramedAsyncChannel> RecvFrameTest;

  // The frame header is 4 bytes.  Test sending each byte separately,
  // 5ms apart, followed by the body.
  ChunkSchedule s1(1, 5,
                   1, 5,
                   1, 5,
                   1, 5,
                   100, 10,
                   -1, 10);
  // Test reading the whole message
  RecvFrameTest(s1).run();
  // Setting the timeout to 15ms should still succeed--the code only times out
  // if no data is received for the specified period
  RecvFrameTest(s1, 15).run();

  // Test timing out before any data is sent
  RecvFrameTest(ChunkSchedule(-1, 50),
                20).run();
  // Test timing out after part of the frame header is sent
  RecvFrameTest(ChunkSchedule(2, 10,
                              -1, 50),
                20).run();
  // Test timing out after the frame header is sent
  RecvFrameTest(ChunkSchedule(4, 10,
                              -1, 50),
                20).run();
  // Test timing out after part of the body is snet
  RecvFrameTest(ChunkSchedule(100, 10,
                              -1, 50),
                20).run();

  // Test closing the connection before any data is sent
  RecvFrameTest(ChunkSchedule(0, 5)).run();
  // Test closing the connection after part of the frame header is sent
  RecvFrameTest(ChunkSchedule(2, 10,
                              0, 5)).run();
  // Test closing the connection after the frame header is sent
  RecvFrameTest(ChunkSchedule(4, 10,
                              0, 5)).run();
  // Test closing the connection after part of the body is snet
  RecvFrameTest(ChunkSchedule(100, 10,
                              0, 5)).run();


  // Some various other schedules
  RecvFrameTest(ChunkSchedule(1, 10,
                              1, 10,
                              100, 10,
                              -1, 5)).run();
}

BOOST_AUTO_TEST_CASE(TestRecvBinaryChunks) {
  typedef RecvChunksTest<TBinaryAsyncChannel> RecvBinaryTest;

  // Test sending the first four bytes byte separately,
  // 5ms apart, followed by the rest of the message.
  ChunkSchedule s1(1, 5,
                   1, 5,
                   1, 5,
                   1, 5,
                   100, 10,
                   -1, 10);
  // Test reading the whole message
  RecvBinaryTest(s1).run();
  // Setting the timeout to 15ms should still succeed--the code only times out
  // if no data is received for the specified period
  RecvBinaryTest(s1, 15).run();

  // Test timing out before any data is sent
  RecvBinaryTest(ChunkSchedule(-1, 50),
                 20).run();
  // Test timing out after part of the frame header is sent
  RecvBinaryTest(ChunkSchedule(2, 10,
                               -1, 50),
                 20).run();
  // Test timing out after the frame header is sent
  RecvBinaryTest(ChunkSchedule(4, 10,
                               -1, 50),
                 20).run();
  // Test timing out after part of the body is snet
  RecvBinaryTest(ChunkSchedule(100, 10,
                               -1, 50),
                 20).run();

  // Test closing the connection before any data is sent
  RecvBinaryTest(ChunkSchedule(0, 5)).run();
  // Test closing the connection after sending 4 bytes
  RecvBinaryTest(ChunkSchedule(2, 10,
                               0, 5)).run();
  // Test closing the connection after sending 100 bytes
  RecvBinaryTest(ChunkSchedule(100, 10,
                               0, 5)).run();


  // Some various other schedules
  RecvBinaryTest(ChunkSchedule(1, 10,
                               1, 10,
                               100, 10,
                               -1, 5)).run();
}

template<typename ChannelT>
class SendTimeoutTest : public SocketPairTest<ChannelT> {
 public:
  explicit SendTimeoutTest(uint32_t timeout)
    : timeout_(timeout)
    , start_(false)
    , msg_(1024*1024) {
  }

  void preLoop() {
    this->socket0_->setSendTimeout(timeout_);
    msg_.copyTo(&sendBuf_);
    sendCallback_.send(this->channel0_, &sendBuf_);
    // don't receive on the other socket

    start_.reset();
  }

  void postLoop() {
    BOOST_CHECK_EQUAL(sendCallback_.getSendError(), 1);
    BOOST_CHECK_EQUAL(sendCallback_.getSendDone(), 0);
    T_CHECK_TIMEOUT(start_, sendCallback_.getTimestamp(), timeout_);
  }

 private:
  uint32_t timeout_;
  TimePoint start_;
  Message msg_;
  TMemoryBuffer sendBuf_;
  ChannelCallback sendCallback_;
};

BOOST_AUTO_TEST_CASE(TestSendTimeoutFramed) {
  SendTimeoutTest<TFramedAsyncChannel>(25).run();
  SendTimeoutTest<TFramedAsyncChannel>(100).run();
  SendTimeoutTest<TFramedAsyncChannel>(250).run();
}

BOOST_AUTO_TEST_CASE(TestSendTimeoutBinary) {
  SendTimeoutTest<TBinaryAsyncChannel>(25).run();
  SendTimeoutTest<TBinaryAsyncChannel>(100).run();
  SendTimeoutTest<TBinaryAsyncChannel>(250).run();
}

template<typename ChannelT>
class SendClosedTest : public SocketPairTest<ChannelT> {
 public:
  explicit SendClosedTest(int closeTimeout = 5)
    : closeTimeout_(closeTimeout)
    , start_(false)
    , msg_(1024*1024) {
  }

  void preLoop() {
    msg_.copyTo(&sendBuf_);
    sendCallback_.send(this->channel0_, &sendBuf_);

    // Close the other socket after 25ms
    this->eventBase_.runAfterDelay(
        std::bind(&TAsyncSocket::close, this->socket1_.get()),
        closeTimeout_);

    start_.reset();
  }

  void postLoop() {
    BOOST_CHECK_EQUAL(sendCallback_.getSendError(), 1);
    BOOST_CHECK_EQUAL(sendCallback_.getSendDone(), 0);
    T_CHECK_TIMEOUT(start_, sendCallback_.getTimestamp(), closeTimeout_);
  }

 private:
  uint32_t closeTimeout_;
  TimePoint start_;
  Message msg_;
  TMemoryBuffer sendBuf_;
  ChannelCallback sendCallback_;
};

BOOST_AUTO_TEST_CASE(TestSendClosed) {
  SendClosedTest<TFramedAsyncChannel>().run();
  SendClosedTest<TBinaryAsyncChannel>().run();
}

unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  unit_test::framework::master_test_suite().p_name.value = "TAsyncChannelTest";

  // Ignore SIGPIPE.  This just makes it easier to debug in gdb and valgrind,
  // so they won't stop on the signal.
  signal(SIGPIPE, SIG_IGN);

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
