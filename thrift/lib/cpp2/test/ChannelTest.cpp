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

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/HeaderServerChannel.h>
#include <thrift/lib/cpp2/async/Cpp2Channel.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/Cursor.h>
#include <folly/io/async/test/SocketPair.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <folly/io/async/AsyncTimeout.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <folly/io/async/EventBase.h>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp2/async/StubSaslClient.h>
#include <thrift/lib/cpp2/async/StubSaslServer.h>

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using apache::thrift::ContextStack;
using std::unique_ptr;
using folly::IOBuf;
using folly::IOBufQueue;
using std::shared_ptr;
using folly::make_unique;

unique_ptr<IOBuf> makeTestBuf(size_t len) {
  unique_ptr<IOBuf> buf = IOBuf::create(len);
  buf->IOBuf::append(len);
  memset(buf->writableData(), (char) (len % 127), len);
  return buf;
}

class EventBaseAborter : public folly::AsyncTimeout {
 public:
  EventBaseAborter(folly::EventBase* eventBase, uint32_t timeoutMS)
    : folly::AsyncTimeout(eventBase, folly::AsyncTimeout::InternalEnum::INTERNAL)
    , eventBase_(eventBase) {
    scheduleTimeout(timeoutMS);
  }

  void timeoutExpired() noexcept override {
    EXPECT_TRUE(false);
    eventBase_->terminateLoopSoon();
  }

 private:
  folly::EventBase* eventBase_;
};

// Creates/unwraps a framed message (LEN(MSG) | MSG)
class TestFramingHandler : public FramingHandler {
public:
  std::tuple<unique_ptr<IOBuf>, size_t, unique_ptr<THeader>>
  removeFrame(IOBufQueue* q) override {
    assert(q);
    queue_.append(*q);
    if (!queue_.front() || queue_.front()->empty()) {
      return make_tuple(std::unique_ptr<IOBuf>(), 0, nullptr);
    }

    uint32_t len = queue_.front()->computeChainDataLength();

    if (len < 4) {
      size_t remaining = 4 - len;
      return make_tuple(unique_ptr<IOBuf>(), remaining, nullptr);
    }

    folly::io::Cursor c(queue_.front());
    uint32_t msgLen = c.readBE<uint32_t>();
    if (len - 4 < msgLen) {
      size_t remaining = msgLen - (len - 4);
      return make_tuple(unique_ptr<IOBuf>(), remaining, nullptr);
    }

    queue_.trimStart(4);
    return make_tuple(queue_.split(msgLen), 0, nullptr);
  }

  unique_ptr<IOBuf> addFrame(unique_ptr<IOBuf> buf,
                             THeader* header) override {
    assert(buf);
    unique_ptr<IOBuf> framing;

    if (buf->headroom() > 4) {
      framing = std::move(buf);
      buf->prepend(4);
    } else {
      framing = IOBuf::create(4);
      framing->append(4);
      framing->appendChain(std::move(buf));
    }
    folly::io::RWPrivateCursor c(framing.get());
    c.writeBE<uint32_t>(framing->computeChainDataLength() - 4);

    return framing;
  }
private:
  IOBufQueue queue_;
};

template <typename Channel>
unique_ptr<Channel, folly::DelayedDestruction::Destructor> createChannel(
    const shared_ptr<TAsyncTransport>& transport) {
  return Channel::newChannel(transport);
}

template <>
unique_ptr<Cpp2Channel, folly::DelayedDestruction::Destructor> createChannel(
    const shared_ptr<TAsyncTransport>& transport) {
  return Cpp2Channel::newChannel(transport, make_unique<TestFramingHandler>());
}

template<typename Channel1, typename Channel2>
class SocketPairTest {
 public:
  SocketPairTest()
    : eventBase_() {
    folly::SocketPair socketPair;

    socket0_ = TAsyncSocket::newSocket(&eventBase_, socketPair.extractFD0());
    socket1_ = TAsyncSocket::newSocket(&eventBase_, socketPair.extractFD1());

    channel0_ = createChannel<Channel1>(socket0_);
    channel1_ = createChannel<Channel2>(socket1_);
  }
  virtual ~SocketPairTest() {}

  void loop(uint32_t timeoutMS) {
    EventBaseAborter aborter(&eventBase_, timeoutMS);
    eventBase_.loop();
  }

  void run() {
    runWithTimeout();
  }

  void runWithTimeout(uint32_t timeoutMS = 6000) {
    preLoop();
    loop(timeoutMS);
    postLoop();
  }

  virtual void preLoop() {}
  virtual void postLoop() {}

 protected:
  folly::EventBase eventBase_;
  shared_ptr<TAsyncSocket> socket0_;
  shared_ptr<TAsyncSocket> socket1_;
  unique_ptr<Channel1, folly::DelayedDestruction::Destructor> channel0_;
  unique_ptr<Channel2, folly::DelayedDestruction::Destructor> channel1_;
};


class MessageCallback
    : public MessageChannel::SendCallback
    , public MessageChannel::RecvCallback {
 public:
  MessageCallback()
      : sent_(0)
      , recv_(0)
      , sendError_(0)
      , recvError_(0)
      , recvEOF_(0)
      , recvBytes_(0) {}

  void sendQueued() override {}

  void messageSent() override { sent_++; }
  void messageSendError(folly::exception_wrapper&& ex) override {
    sendError_++;
  }

  void messageReceived(unique_ptr<IOBuf>&& buf,
                       unique_ptr<THeader>&& header,
                       unique_ptr<sample>) override {
    recv_++;
    recvBytes_ += buf->computeChainDataLength();
  }
  void messageChannelEOF() override { recvEOF_++; }
  void messageReceiveErrorWrapped(folly::exception_wrapper&& ex) override {
    sendError_++;
  }

  uint32_t sent_;
  uint32_t recv_;
  uint32_t sendError_;
  uint32_t recvError_;
  uint32_t recvEOF_;
  size_t recvBytes_;
};

class TestRequestCallback : public RequestCallback, public CloseCallback {
 public:
  void requestSent() override {}
  void replyReceived(ClientReceiveState&& state) override {
    reply_++;
    replyBytes_ += state.buf()->computeChainDataLength();
    if (state.isSecurityActive()) {
      replySecurityActive_++;
    }
    securityStartTime_ = securityStart_;
    securityEndTime_ = securityEnd_;
  }
  void requestError(ClientReceiveState&& state) override {
    std::exception_ptr ex = state.exception();
    try {
      std::rethrow_exception(ex);
    } catch (const std::exception& e) {
      // Verify that exception pointer is passed properly
    }
    replyError_++;
    securityStartTime_ = securityStart_;
    securityEndTime_ = securityEnd_;
  }
  void channelClosed() override { closed_ = true; }

  static void reset() {
    closed_ = false;
    reply_ = 0;
    replyBytes_ = 0;
    replyError_ = 0;
    replySecurityActive_ = 0;
    securityStartTime_ = 0;
    securityEndTime_ = 0;
  }

  static bool closed_;
  static uint32_t reply_;
  static uint32_t replyBytes_;
  static uint32_t replyError_;
  static uint32_t replySecurityActive_;
  static int64_t securityStartTime_;
  static int64_t securityEndTime_;
};

bool TestRequestCallback::closed_ = false;
uint32_t TestRequestCallback::reply_ = 0;
uint32_t TestRequestCallback::replyBytes_ = 0;
uint32_t TestRequestCallback::replyError_ = 0;
uint32_t TestRequestCallback::replySecurityActive_ = 0;
int64_t TestRequestCallback::securityStartTime_ = 0;
int64_t TestRequestCallback::securityEndTime_ = 0;

class ResponseCallback
    : public ResponseChannel::Callback {
 public:
  ResponseCallback()
      : serverClosed_(false)
      , oneway_(0)
      , request_(0)
      , requestBytes_(0) {}

  void requestReceived(unique_ptr<ResponseChannel::Request>&& req) override {
    request_++;
    requestBytes_ += req->getBuf()->computeChainDataLength();
    if (req->isOneway()) {
      oneway_++;
    } else {
      req->sendReply(req->extractBuf());
    }
  }

  void channelClosed(folly::exception_wrapper&& ew) override {
    serverClosed_ = true;
  }

  bool serverClosed_;
  uint32_t oneway_;
  uint32_t request_;
  uint32_t requestBytes_;
};

class MessageTest : public SocketPairTest<Cpp2Channel, Cpp2Channel>
                  , public MessageCallback {
 public:
  explicit MessageTest(size_t len)
      : len_(len)
      , header_(new THeader) {
  }

  void preLoop() override {
    channel0_->sendMessage(&sendCallback_, makeTestBuf(len_), header_.get());
    channel1_->setReceiveCallback(this);
  }

  void postLoop() override {
    EXPECT_EQ(sendCallback_.sendError_, 0);
    EXPECT_EQ(recvError_, 0);
    EXPECT_EQ(recvEOF_, 0);
    EXPECT_EQ(recv_, 1);
    EXPECT_EQ(sendCallback_.sent_, 1);
    EXPECT_EQ(recvBytes_, len_);
  }

  void messageReceived(unique_ptr<IOBuf>&& buf,
                       unique_ptr<THeader>&& header,
                       unique_ptr<sample> sample) override {
    MessageCallback::messageReceived(std::move(buf),
                                     std::move(header),
                                     std::move(sample));
    channel1_->setReceiveCallback(nullptr);
  }


 private:
  size_t len_;
  unique_ptr<THeader> header_;
  MessageCallback sendCallback_;
};

TEST(Channel, Cpp2Channel) {
  MessageTest(1).run();
  MessageTest(100).run();
  MessageTest(1024*1024).run();
}

class MessageCloseTest : public SocketPairTest<Cpp2Channel, Cpp2Channel>
                       , public MessageCallback {
public:
  MessageCloseTest() : header_(new THeader) {}

  void preLoop() override {
    channel0_->sendMessage(&sendCallback_,
                           makeTestBuf(1024*1024),
                           header_.get());
    // Close the other socket after delay
    this->eventBase_.runInLoop(
      std::bind(&TAsyncSocket::close, this->socket1_.get()));
    channel1_->setReceiveCallback(this);
  }

  void postLoop() override {
    EXPECT_EQ(sendCallback_.sendError_, 1);
    EXPECT_EQ(recvError_, 0);
    EXPECT_EQ(recvEOF_, 1);
    EXPECT_EQ(recv_, 0);
    EXPECT_EQ(sendCallback_.sent_, 0);
  }

  void messageChannelEOF() override {
    MessageCallback::messageChannelEOF();
    channel1_->setReceiveCallback(nullptr);
  }

 private:
  MessageCallback sendCallback_;
  unique_ptr<THeader> header_;
};

TEST(Channel, MessageCloseTest) {
  MessageCloseTest().run();
}

class MessageEOFTest : public SocketPairTest<Cpp2Channel, Cpp2Channel>
                     , public MessageCallback {
public:
  MessageEOFTest() : header_(new THeader) {}

  void preLoop() override {
    channel0_->setReceiveCallback(this);
    channel1_->getTransport()->shutdownWrite();
    channel0_->sendMessage(this,
                           makeTestBuf(1024*1024),
                           header_.get());
  }

  void postLoop() override {
    EXPECT_EQ(sendError_, 1);
    EXPECT_EQ(recvError_, 0);
    EXPECT_EQ(recvEOF_, 1);
    EXPECT_EQ(recv_, 0);
    EXPECT_EQ(sent_, 0);
  }

 private:
  unique_ptr<THeader> header_;
};

TEST(Channel, MessageEOFTest) {
  MessageEOFTest().run();
}

class HeaderChannelTest
    : public SocketPairTest<HeaderClientChannel, HeaderServerChannel>
    , public TestRequestCallback
    , public ResponseCallback {
public:
  explicit HeaderChannelTest(size_t len)
      : len_(len) {
  }

  class Callback : public TestRequestCallback {
   public:
    explicit Callback(HeaderChannelTest* c)
    : c_(c) {}
    void replyReceived(ClientReceiveState&& state) override {
      TestRequestCallback::replyReceived(std::move(state));
      c_->channel1_->setCallback(nullptr);
    }
   private:
    HeaderChannelTest* c_;
  };

  void preLoop() override {
    TestRequestCallback::reset();
    channel1_->setCallback(this);
    channel0_->setCloseCallback(this);
    channel0_->sendOnewayRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_),
      std::unique_ptr<THeader>(new THeader));
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_),
      std::unique_ptr<THeader>(new THeader));
    channel0_->setCloseCallback(nullptr);
  }

  void postLoop() override {
    EXPECT_EQ(reply_, 1);
    EXPECT_EQ(replyError_, 0);
    EXPECT_EQ(replyBytes_, len_);
    EXPECT_EQ(closed_, false);
    EXPECT_EQ(serverClosed_, false);
    EXPECT_EQ(request_, 2);
    EXPECT_EQ(requestBytes_, len_*2);
    EXPECT_EQ(oneway_, 1);
    channel1_->setCallback(nullptr);
    EXPECT_EQ(securityStartTime_, 0);
    EXPECT_EQ(securityEndTime_, 0);
  }

 private:
  size_t len_;
};

TEST(Channel, HeaderChannelTest) {
  HeaderChannelTest(1).run();
  HeaderChannelTest(100).run();
  HeaderChannelTest(1024*1024).run();
}

class HeaderChannelClosedTest
    : public SocketPairTest<HeaderClientChannel, HeaderServerChannel> {
  //   , public TestRequestCallback
  //   , public ResponseCallback {
 public:
  explicit HeaderChannelClosedTest() {}

  class Callback : public RequestCallback {
   public:
    explicit Callback(HeaderChannelClosedTest* c)
      : c_(c) {}

    ~Callback() {
      c_->callbackDtor_ = true;
    }

    void replyReceived(ClientReceiveState&& state) override {
      FAIL() << "should not recv reply from closed channel";
    }

    void requestSent() override {
      FAIL() << "should not have sent message on closed channel";
    }

    void requestError(ClientReceiveState&& state) override {
      EXPECT_TRUE(state.isException());
      EXPECT_TRUE(state.exceptionWrapper().with_exception<TTransportException>(
        [this] (const TTransportException& e) {
          EXPECT_EQ(e.getType(), TTransportException::END_OF_FILE);
          c_->gotError_ = true;
        }
      ));
    }

   private:
    HeaderChannelClosedTest* c_;
  };

  void preLoop() override {
    TestRequestCallback::reset();
    channel1_->getTransport()->shutdownWrite();
    seqId_ = channel0_->sendRequest(
      folly::make_unique<Callback>(this),
      // Fake method name for creating a ContextStatck
      folly::make_unique<ContextStack>("{ChannelTest}"),
      makeTestBuf(42),
      folly::make_unique<THeader>());
  }

  void postLoop() override {
    EXPECT_TRUE(gotError_);
    EXPECT_FALSE(channel0_->expireCallback(seqId_));
    EXPECT_TRUE(callbackDtor_);
  }

 private:
  uint32_t seqId_;
  bool gotError_ = true;
  bool callbackDtor_ = false;
};

TEST(Channel, HeaderChannelClosedTest) {
  HeaderChannelClosedTest().run();
}

class SecurityNegotiationTest
    : public SocketPairTest<HeaderClientChannel, HeaderServerChannel>
    , public TestRequestCallback
    , public ResponseCallback {
public:
  explicit SecurityNegotiationTest(bool clientSasl, bool clientNonSasl,
                                   bool serverSasl, bool serverNonSasl,
                                   bool expectConn, bool expectSecurity,
                                   bool expectSecurityAttempt,
                                   int64_t expectedSecurityLatency = 0)
      : clientSasl_(clientSasl), clientNonSasl_(clientNonSasl)
      , serverSasl_(serverSasl), serverNonSasl_(serverNonSasl)
      , expectConn_(expectConn), expectSecurity_(expectSecurity)
      , expectSecurityAttempt_(expectSecurityAttempt)
      , expectedSecurityLatency_(expectedSecurityLatency) {
    // Replace handshake mechanism with a stub.
    stubSaslClient_ = new StubSaslClient(socket0_->getEventBase());
    // Force each RTT in stub handshake to take at least 100 ms.
    stubSaslClient_->setForceMsSpentPerRTT(100);
    channel0_->setSaslClient(std::unique_ptr<SaslClient>(stubSaslClient_));
    stubSaslServer_ = new StubSaslServer(socket1_->getEventBase());
    channel1_->setSaslServer(std::unique_ptr<SaslServer>(stubSaslServer_));

    // Capture the timestamp
    auto now = std::chrono::high_resolution_clock::now();
    initTime_ = std::chrono::duration_cast<std::chrono::microseconds>(
                  now.time_since_epoch()).count();

  }

  ~SecurityNegotiationTest() override {
    // In case we are still installed as a callback on the channels, clear the
    // callbacks now.  We are being destroyed now, so it will cause a crash if
    // we are left installed and the channel then tries to call us.
    if (channel0_) {
      channel0_->setCloseCallback(nullptr);
    }
    if (channel1_) {
      channel1_->setCallback(nullptr);
    }
  }

  class Callback : public TestRequestCallback {
   public:
    explicit Callback(SecurityNegotiationTest* c)
    : c_(c) {}
    void replyReceived(ClientReceiveState&& state) override {
      TestRequestCallback::replyReceived(std::move(state));
      // If the request works, clear the close callback, so it's not
      // hanging around.
      if (!c_->closed_) {
        c_->channel0_->setCloseCallback(nullptr);
      }
      if (!c_->serverClosed_) {
        c_->channel1_->setCallback(nullptr);
      }
    }
    // Leave requestError() alone.  If the request fails, don't
    // clear any callbacks.  Failure here (in this test) implies
    // negotiation failure which means the client will destroy its
    // own collection which cleans up the callback, which will cause
    // the server to see a connection close, and we test for both
    // callbacks being invoked.
  private:
    SecurityNegotiationTest* c_;
  };

  void channelClosed() override {
    EXPECT_EQ(channel0_->getSaslPeerIdentity(), "");
    TestRequestCallback::channelClosed();
  }

  void channelClosed(folly::exception_wrapper&& ew) override {
    EXPECT_EQ(channel1_->getSaslPeerIdentity(), "");
    ResponseCallback::channelClosed(std::move(ew));
    channel1_->setCallback(nullptr);
  }

  void preLoop() override {
    TestRequestCallback::reset();
    if (clientSasl_ && clientNonSasl_) {
      channel0_->setSecurityPolicy(THRIFT_SECURITY_PERMITTED);
    } else if (clientSasl_) {
      channel0_->setSecurityPolicy(THRIFT_SECURITY_REQUIRED);
    } else {
      channel0_->setSecurityPolicy(THRIFT_SECURITY_DISABLED);
    }

    if (serverSasl_ && serverNonSasl_) {
      channel1_->setSecurityPolicy(THRIFT_SECURITY_PERMITTED);
    } else if (serverSasl_) {
      channel1_->setSecurityPolicy(THRIFT_SECURITY_REQUIRED);
    } else {
      channel1_->setSecurityPolicy(THRIFT_SECURITY_DISABLED);
    }

    channel1_->setCallback(this);
    channel0_->setCloseCallback(this);
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(1),
      std::unique_ptr<THeader>(new THeader));
  }

  void postLoop() override {
    if (expectConn_) {
      EXPECT_EQ(reply_, 1);
      EXPECT_EQ(replyError_, 0);
      EXPECT_EQ(replyBytes_, 1);
      EXPECT_EQ(closed_, false);
      EXPECT_EQ(serverClosed_, false);
      EXPECT_EQ(request_, 1);
      EXPECT_EQ(requestBytes_, 1);
      EXPECT_EQ(oneway_, 0);
    } else {
      CHECK(!expectSecurity_);
      EXPECT_EQ(reply_, 0);
      EXPECT_EQ(replyError_, 1);
      EXPECT_EQ(replyBytes_, 0);
      EXPECT_EQ(closed_, true);
      EXPECT_EQ(serverClosed_, true);
      EXPECT_EQ(request_, 0);
      EXPECT_EQ(requestBytes_, 0);
      EXPECT_EQ(oneway_, 0);
    }
    if (expectSecurity_) {
      EXPECT_NE(channel0_->getSaslPeerIdentity(), "");
      EXPECT_NE(channel1_->getSaslPeerIdentity(), "");
      EXPECT_EQ(replySecurityActive_, 1);

      // Check the latency incurred for doing security
      CHECK(expectSecurityAttempt_);
      EXPECT_GT(securityStartTime_, initTime_);
      EXPECT_GT(securityEndTime_, securityStartTime_);
      /*
       * Security is expected to succeed. We assert that security latency is
       * less than expectedSecurityLatency_
       */
      EXPECT_LT(securityEndTime_ - securityStartTime_,
                expectedSecurityLatency_);
      EXPECT_GT(securityEndTime_ - securityStartTime_, 200*1000);
    } else {
      EXPECT_EQ(replySecurityActive_, 0);
      if (expectSecurityAttempt_) {
        // Check the latency incurred for doing security
        EXPECT_GT(securityStartTime_, initTime_);
        EXPECT_GT(securityEndTime_, securityStartTime_);
        /*
         * Security is expected to be attempted but not succeed. We assert on
         * security latency at least being greater than expectedSecurityLatency_
         */
        EXPECT_GT(securityEndTime_ - securityStartTime_,
                  expectedSecurityLatency_);
        EXPECT_LT(securityEndTime_ - securityStartTime_,
                  static_cast<int64_t>(expectedSecurityLatency_ * 1.2));
      }
    }
  }

protected:
  StubSaslClient* stubSaslClient_;
  StubSaslServer* stubSaslServer_;

 private:
  bool clientSasl_;
  bool clientNonSasl_;
  bool serverSasl_;
  bool serverNonSasl_;
  bool expectConn_;
  bool expectSecurity_;
  bool expectSecurityAttempt_;
  int64_t expectedSecurityLatency_;
  int64_t initTime_;
};

class SecurityNegotiationClientFailTest : public SecurityNegotiationTest {
public:
  template <typename... Args>
  explicit SecurityNegotiationClientFailTest(Args&&... args)
      : SecurityNegotiationTest(std::forward<Args>(args)...) {
    stubSaslClient_->setForceFallback();
  }
};

class SecurityNegotiationServerFailTest : public SecurityNegotiationTest {
public:
  template <typename... Args>
  explicit SecurityNegotiationServerFailTest(Args&&... args)
      : SecurityNegotiationTest(std::forward<Args>(args)...) {
    stubSaslServer_->setForceFallback();
  }
};

class SecurityNegotiationClientTimeoutGarbageTest :
  public SecurityNegotiationTest {
public:
  template <typename... Args>
  explicit SecurityNegotiationClientTimeoutGarbageTest(Args&&... args)
      : SecurityNegotiationTest(std::forward<Args>(args)...) {
    stubSaslClient_->setForceTimeout();
    stubSaslServer_->setForceSendGarbage();
  }
};

class SecurityNegotiationLatencyTest : public SecurityNegotiationTest {
public:
  template <typename... Args>
  explicit SecurityNegotiationLatencyTest(Args&&... args)
      : SecurityNegotiationTest(std::forward<Args>(args)...) {
    stubSaslClient_->setForceTimeout();
  }
};

TEST(Channel, SecurityNegotiationLatencyTest) {
  // This test forces a timeout on the client side. This means that the client
  // will attempt to do security but not succeed.

  int64_t defaultSaslTimeout = 500 * 1000; //microseconds

  //clientSasl clientNonSasl serverSasl serverNonSasl expectConn expectSecurity
  //expectSecurityAttempt expectedSecurityLatency

  /*
   * For the following three tests connection will not be established because
   * of security failure. Since the first RTT itself times out, the expected
   * security latency is > defaultSaslTimeout.
   */

  // Client = required, Server: required.
  SecurityNegotiationLatencyTest(true, false, true, false, false, false, true,
                                 defaultSaslTimeout).run();

  // Client = required, Server: permitted.
  SecurityNegotiationLatencyTest(true, false, true, true, false, false, true,
                                 defaultSaslTimeout).run();

  // Client = permitted, Server: required.
  SecurityNegotiationLatencyTest(true, true, true, false, false, false, true,
                                 defaultSaslTimeout).run();

  // For the following test connection will be established despite security
  // failure.

  // Client = permitted, Server: permitted.
  SecurityNegotiationLatencyTest(true, true, true, true, true, false, true,
                                 defaultSaslTimeout).run();
}

TEST(Channel, SecurityNegotiationTest) {
  //clientSasl clientNonSasl serverSasl serverNonSasl expectConn expectSecurity
  //expectSecurityAttempt

  /*
   * When expectSecurity is true, we expect security to succed in those runs.
   * The expected latency in these cases < 2*defaultSaslTimeout since the
   * stubSaslClient implementation performs only 2 RTTs.
   */

  int64_t defaultSaslTimeout = 500 * 1000; //microseconds

  // Client: disabled; Server: disabled, disabled, required, permitted
  SecurityNegotiationTest(false, false, false, false, true, false, false).run();
  SecurityNegotiationTest(false, false, false, true, true, false, false).run();
  SecurityNegotiationTest(false, false, true, false, false, false, false).run();
  SecurityNegotiationTest(false, false, true, true, true, false, false).run();

  // Client policy: disabled; Server: disabled, disabled, required, permitted
  SecurityNegotiationTest(false, true, false, false, true, false, false).run();
  SecurityNegotiationTest(false, true, false, true, true, false, false).run();
  SecurityNegotiationTest(false, true, true, false, false, false, false).run();
  SecurityNegotiationTest(false, true, true, true, true, false, false).run();

  // Client policy: required; Server: disabled, disabled, required, permitted
  SecurityNegotiationTest(true, false, false, false, false, false, false).run();
  SecurityNegotiationTest(true, false, false, true, false, false, false).run();
  SecurityNegotiationTest(true, false, true, false, true, true, true,
                          2*defaultSaslTimeout).run();
  SecurityNegotiationTest(true, false, true, true, true, true, true,
                          2*defaultSaslTimeout).run();

  // Client policy: permitted; Server: disabled, disabled, required, permitted
  SecurityNegotiationTest(true, true, false, false, true, false, false).run();
  SecurityNegotiationTest(true, true, false, true, true, false, false).run();
  SecurityNegotiationTest(true, true, true, false, true, true, true,
                          2*defaultSaslTimeout).run();
  SecurityNegotiationTest(true, true, true, true, true, true, true,
                          2*defaultSaslTimeout).run();
}

TEST(Channel, SecurityNegotiationFailTest) {
  SecurityNegotiationServerFailTest(
    true, false, true, true, false,false, false).run();

  SecurityNegotiationClientFailTest(
    true, true, true, false, false, false, false).run();

  SecurityNegotiationClientFailTest(
    true, true, true, true, true, false, false).run();
  SecurityNegotiationServerFailTest(
    true, true, true, true, true, false, false).run();
}

TEST(Channel, SecurityNegotiationTimeoutGarbageTest) {
  SecurityNegotiationClientTimeoutGarbageTest(
    true, false, true, true, false, false, false).run();
}

class InOrderTest
    : public SocketPairTest<HeaderClientChannel, HeaderServerChannel>
    , public TestRequestCallback
    , public ResponseCallback {
 public:
  explicit InOrderTest()
      : len_(1) {
  }

  class Callback : public TestRequestCallback {
   public:
    explicit Callback(InOrderTest* c)
    : c_(c) {}
    void replyReceived(ClientReceiveState&& state) override {
      if (reply_ == 1) {
        c_->channel1_->setCallback(nullptr);
        // Verify that they came back in the same order
        EXPECT_EQ(state.buf()->computeChainDataLength(), c_->len_ + 1);
      }
      TestRequestCallback::replyReceived(std::move(state));
    }

    void requestReceived(unique_ptr<ResponseChannel::Request>&& req) {
      c_->request_++;
      c_->requestBytes_ += req->getBuf()->computeChainDataLength();
      if (c_->firstbuf_) {
        req->sendReply(req->extractBuf());
        c_->firstbuf_->sendReply(c_->firstbuf_->extractBuf());
      } else {
        c_->firstbuf_ = std::move(req);
      }
    }

   private:
    InOrderTest* c_;
  };

  void preLoop() override {
    TestRequestCallback::reset();
    channel0_->setFlags(0); // turn off out of order
    channel1_->setCallback(this);
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_),
      std::unique_ptr<THeader>(new THeader));
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_ + 1),
      std::unique_ptr<THeader>(new THeader));
  }

  void postLoop() override {
    EXPECT_EQ(reply_, 2);
    EXPECT_EQ(replyError_, 0);
    EXPECT_EQ(replyBytes_, 2*len_ + 1);
    EXPECT_EQ(closed_, false);
    EXPECT_EQ(serverClosed_, false);
    EXPECT_EQ(request_, 2);
    EXPECT_EQ(requestBytes_, 2*len_ + 1);
    EXPECT_EQ(oneway_, 0);
    EXPECT_EQ(securityStartTime_, 0);
    EXPECT_EQ(securityEndTime_, 0);
  }

 private:
  std::unique_ptr<ResponseChannel::Request> firstbuf_;
  size_t len_;
};

TEST(Channel, InOrderTest) {
  InOrderTest().run();
}

class BadSeqIdTest
    : public SocketPairTest<HeaderClientChannel, HeaderServerChannel>
    , public TestRequestCallback
    , public ResponseCallback {
public:
  explicit BadSeqIdTest(size_t len)
      : len_(len) {
  }

  class Callback : public TestRequestCallback {
   public:
    explicit Callback(BadSeqIdTest* c)
    : c_(c) {}

    void requestError(ClientReceiveState&& state) override {
      c_->channel1_->setCallback(nullptr);
      TestRequestCallback::requestError(std::move(state));
    }

   private:
    BadSeqIdTest* c_;
  };

  void requestReceived(unique_ptr<ResponseChannel::Request>&& req) override {
    request_++;
    requestBytes_ += req->getBuf()->computeChainDataLength();
    if (req->isOneway()) {
      oneway_++;
      return;
    }
    unique_ptr<THeader> header(new THeader);
    header->setSequenceNumber(-1);
    HeaderServerChannel::HeaderRequest r(
      channel1_.get(),
      req->extractBuf(),
      std::move(header),
      true /*out of order*/,
      std::unique_ptr<MessageChannel::RecvCallback::sample>(nullptr));
    r.sendReply(r.extractBuf());
  }

  void preLoop() override {
    TestRequestCallback::reset();
    channel0_->setTimeout(1000);
    channel1_->setCallback(this);
    channel0_->sendOnewayRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_),
      std::unique_ptr<THeader>(new THeader));
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_),
      std::unique_ptr<THeader>(new THeader));
  }

  void postLoop() override {
    EXPECT_EQ(reply_, 0);
    EXPECT_EQ(replyError_, 1);
    EXPECT_EQ(replyBytes_, 0);
    EXPECT_EQ(closed_, false);
    EXPECT_EQ(serverClosed_, false);
    EXPECT_EQ(request_, 2);
    EXPECT_EQ(requestBytes_, len_*2);
    EXPECT_EQ(oneway_, 1);
    EXPECT_EQ(securityStartTime_, 0);
    EXPECT_EQ(securityEndTime_, 0);
  }

 private:
  size_t len_;
};

TEST(Channel, BadSeqIdTest) {
  BadSeqIdTest(1).run();
}


class TimeoutTest
    : public SocketPairTest<HeaderClientChannel, HeaderServerChannel>
    , public TestRequestCallback
    , public ResponseCallback {
public:
  explicit TimeoutTest(uint32_t timeout)
      : timeout_(timeout)
      , len_(1) {
  }

  void preLoop() override {
    TestRequestCallback::reset();
    channel1_->setCallback(this);
    channel0_->setTimeout(timeout_);
    channel0_->setCloseCallback(this);
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new TestRequestCallback),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_),
      std::unique_ptr<THeader>(new THeader));
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new TestRequestCallback),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_),
      std::unique_ptr<THeader>(new THeader));
  }

  void postLoop() override {
    EXPECT_EQ(reply_, 0);
    EXPECT_EQ(replyError_, 2);
    EXPECT_EQ(replyBytes_, 0);
    EXPECT_EQ(closed_, false); // client timeouts do not close connection
    EXPECT_EQ(serverClosed_, false);
    EXPECT_EQ(request_, 2);
    EXPECT_EQ(requestBytes_, len_ * 2);
    EXPECT_EQ(oneway_, 0);
    channel0_->setCloseCallback(nullptr);
    channel1_->setCallback(nullptr);
    EXPECT_EQ(securityStartTime_, 0);
    EXPECT_EQ(securityEndTime_, 0);
  }

  void requestReceived(unique_ptr<ResponseChannel::Request>&& req) override {
    request_++;
    requestBytes_ += req->getBuf()->computeChainDataLength();
    // Don't respond, let it time out
    // TestRequestCallback::replyReceived(std::move(buf));
    channel1_->getEventBase()->tryRunAfterDelay(
      [&](){
        channel1_->setCallback(nullptr);
        channel0_->setCloseCallback(nullptr);
      },
      timeout_ * 2); // enough time for server socket to close also
  }

 private:
  uint32_t timeout_;
  size_t len_;
};

TEST(Channel, TimeoutTest) {
  TimeoutTest(25).run();
  TimeoutTest(100).run();
  TimeoutTest(250).run();
}

// Test client per-call timeout options
class OptionsTimeoutTest
    : public SocketPairTest<HeaderClientChannel, HeaderServerChannel>
    , public TestRequestCallback
    , public ResponseCallback {
public:
  explicit OptionsTimeoutTest()
      : len_(1) {
  }

  void preLoop() override {
    TestRequestCallback::reset();
    channel1_->setCallback(this);
    channel0_->setTimeout(1000);
    RpcOptions options;
    options.setTimeout(std::chrono::milliseconds(25));
    channel0_->sendRequest(
        options,
        std::unique_ptr<RequestCallback>(new TestRequestCallback),
        // Fake method name for creating a ContextStatck
        std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
        makeTestBuf(len_),
        std::unique_ptr<THeader>(new THeader));
    // Verify the timeout worked within 10ms
    channel0_->getEventBase()->tryRunAfterDelay([&](){
        EXPECT_EQ(replyError_, 1);
      }, 35);
    // Verify that subsequent successful requests don't delay timeout
    channel0_->getEventBase()->tryRunAfterDelay([&](){
        channel0_->sendRequest(
          std::unique_ptr<RequestCallback>(new TestRequestCallback),
          // Fake method name for creating a ContextStatck
          std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
          makeTestBuf(len_),
          std::unique_ptr<THeader>(new THeader));
      }, 20);
  }

  void postLoop() override {
    EXPECT_EQ(reply_, 1);
    EXPECT_EQ(replyError_, 1);
    EXPECT_EQ(replyBytes_, 1);
    EXPECT_EQ(closed_, false); // client timeouts do not close connection
    EXPECT_EQ(serverClosed_, false);
    EXPECT_EQ(request_, 2);
    EXPECT_EQ(requestBytes_, len_ * 2);
    EXPECT_EQ(oneway_, 0);
    EXPECT_EQ(securityStartTime_, 0);
    EXPECT_EQ(securityEndTime_, 0);
  }

  void requestReceived(unique_ptr<ResponseChannel::Request>&& req) override {
    if (request_ == 0) {
      request_++;
      requestBytes_ += req->getBuf()->computeChainDataLength();
    } else {
      ResponseCallback::requestReceived(std::move(req));
      channel1_->setCallback(nullptr);
    }
  }

 private:
  size_t len_;
};

TEST(Channel, OptionsTimeoutTest) {
  OptionsTimeoutTest().run();
}

class ClientCloseTest
    : public SocketPairTest<HeaderClientChannel, HeaderServerChannel>
    , public TestRequestCallback
    , public ResponseCallback {
public:
  explicit ClientCloseTest(bool halfClose)
      : halfClose_(halfClose) {
  }

  void preLoop() override {
    TestRequestCallback::reset();
    channel1_->setCallback(this);
    channel0_->setCloseCallback(this);
    if (halfClose_) {
      channel1_->getEventBase()->tryRunAfterDelay(
        [&](){channel1_->getTransport()->shutdownWrite();

        },
        10);
    } else {
      channel1_->getEventBase()->tryRunAfterDelay(
        [&](){channel1_->getTransport()->close();},
        10);
    }
    channel1_->getEventBase()->tryRunAfterDelay(
      [&](){
        channel1_->setCallback(nullptr);
      },
    20);
    channel0_->getEventBase()->tryRunAfterDelay(
      [&](){
        channel0_->setCloseCallback(nullptr);
      },
    20);
  }

  void postLoop() override {
    EXPECT_EQ(reply_, 0);
    EXPECT_EQ(replyError_, 0);
    EXPECT_EQ(replyBytes_, 0);
    EXPECT_EQ(closed_, true);
    EXPECT_EQ(serverClosed_, !halfClose_);
    EXPECT_EQ(request_, 0);
    EXPECT_EQ(requestBytes_, 0);
    EXPECT_EQ(oneway_, 0);
    EXPECT_EQ(securityStartTime_, 0);
    EXPECT_EQ(securityEndTime_, 0);
  }

 private:
  bool halfClose_;
};

TEST(Channel, ClientCloseTest) {
  ClientCloseTest(true).run();
  ClientCloseTest(false).run();
}

class ServerCloseTest
    : public SocketPairTest<HeaderClientChannel, HeaderServerChannel>
    , public TestRequestCallback
    , public ResponseCallback {
public:
  explicit ServerCloseTest(bool halfClose)
      : halfClose_(halfClose) {
  }

  void preLoop() override {
    TestRequestCallback::reset();
    channel1_->setCallback(this);
    channel0_->setCloseCallback(this);
    if (halfClose_) {
      channel0_->getEventBase()->tryRunAfterDelay(
        [&](){channel0_->getTransport()->shutdownWrite();

        },
        10);
    } else {
      channel0_->getEventBase()->tryRunAfterDelay(
        [&](){channel0_->getTransport()->close();},
        10);
    }
    channel1_->getEventBase()->tryRunAfterDelay(
      [&](){
        channel1_->setCallback(nullptr);
      },
    20);
    channel0_->getEventBase()->tryRunAfterDelay(
      [&](){
        channel0_->setCloseCallback(nullptr);
      },
    20);
  }

  void postLoop() override {
    EXPECT_EQ(reply_, 0);
    EXPECT_EQ(replyError_, 0);
    EXPECT_EQ(replyBytes_, 0);
    EXPECT_EQ(closed_, !halfClose_);
    EXPECT_EQ(serverClosed_, true);
    EXPECT_EQ(request_, 0);
    EXPECT_EQ(requestBytes_, 0);
    EXPECT_EQ(oneway_, 0);
    EXPECT_EQ(securityStartTime_, 0);
    EXPECT_EQ(securityEndTime_, 0);
  }

 private:
  bool halfClose_;
};

TEST(Channel, ServerCloseTest) {
  ServerCloseTest(true).run();
  ServerCloseTest(false).run();
}


class DestroyAsyncTransport : public apache::thrift::async::TAsyncTransport {
 public:
  DestroyAsyncTransport() : cb_(nullptr) { }
  void setReadCB(
      folly::AsyncTransportWrapper::ReadCallback* callback) override {
    cb_ = callback;
  }
  ReadCallback* getReadCallback() const override {
    return dynamic_cast<ReadCallback*>(cb_);
  }
  void write(folly::AsyncTransportWrapper::WriteCallback* c,
             const void* v,
             size_t s,
             WriteFlags flags) override {}
  void writev(folly::AsyncTransportWrapper::WriteCallback* c,
              const iovec* v,
              size_t s,
              WriteFlags flags) override {}
  void writeChain(folly::AsyncTransportWrapper::WriteCallback* c,
                  std::unique_ptr<folly::IOBuf>&& i,
                  WriteFlags flags) override {}
  void close() override {}
  void closeNow() override {}
  void shutdownWrite() override {}
  void shutdownWriteNow() override {}
  bool good() const override { return true; }
  bool readable() const override { return false; }
  bool connecting() const override { return false; }
  bool error() const override { return false; }
  void attachEventBase(folly::EventBase* e) override {}
  void detachEventBase() override {}
  bool isDetachable() const override { return true; }
  folly::EventBase* getEventBase() const override { return nullptr; }
  void setSendTimeout(uint32_t ms) override {}
  uint32_t getSendTimeout() const override { return 0; }
  void getLocalAddress(folly::SocketAddress* a) const override {}
  void getPeerAddress(folly::SocketAddress* a) const override {}
  size_t getAppBytesWritten() const override { return 0; }
  size_t getRawBytesWritten() const override { return 0; }
  size_t getAppBytesReceived() const override { return 0; }
  size_t getRawBytesReceived() const override { return 0; }
  void setEorTracking(bool track) override {}
  bool isEorTrackingEnabled() const override { return false; }

  void invokeEOF() {
    cb_->readEOF();
  }
 private:
  folly::AsyncTransportWrapper::ReadCallback* cb_;
};

class DestroyRecvCallback : public MessageChannel::RecvCallback {
 public:
  typedef std::unique_ptr<Cpp2Channel, folly::DelayedDestruction::Destructor>
      ChannelPointer;
  explicit DestroyRecvCallback(ChannelPointer&& channel)
      : channel_(std::move(channel)),
        invocations_(0) {
    channel_->setReceiveCallback(this);
  }
  void messageReceived(
    std::unique_ptr<folly::IOBuf>&&,
    std::unique_ptr<apache::thrift::transport::THeader>&&,
    std::unique_ptr<MessageChannel::RecvCallback::sample> sample) override {}
  void messageChannelEOF() override {
    EXPECT_EQ(invocations_, 0);
    invocations_++;
    channel_.reset();
  }
  void messageReceiveErrorWrapped(folly::exception_wrapper&&) override {}

 private:
  ChannelPointer channel_;
  int invocations_;
};


TEST(Channel, DestroyInEOF) {
  auto t = new DestroyAsyncTransport;
  std::shared_ptr<TAsyncTransport> transport(t);
  auto channel = createChannel<Cpp2Channel>(transport);
  DestroyRecvCallback drc(std::move(channel));
  t->invokeEOF();
}

class NullCloseCallback : public CloseCallback {
 public:
  void channelClosed() override {}
};

TEST(Channel, SetKeepRegisteredForClose) {
  int lfd = socket(PF_INET, SOCK_STREAM, 0);
  int rc = listen(lfd, 10);
  CHECK(rc == 0);
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  rc = getsockname(lfd, (struct sockaddr*)&addr, &addrlen);

  folly::EventBase base;
  auto transport =
    TAsyncSocket::newSocket(&base, "127.0.0.1", ntohs(addr.sin_port));
  auto channel = createChannel<HeaderClientChannel>(transport);
  NullCloseCallback ncc;
  channel->setCloseCallback(&ncc);
  channel->setKeepRegisteredForClose(false);

  EXPECT_TRUE(base.loop());

  close(lfd);
}

int main(int argc, char** argv) {
  testing::InitGoogleTest(&argc, argv);
  google::InitGoogleLogging(argv[0]);
  google::ParseCommandLineFlags(&argc, &argv, true);

  return RUN_ALL_TESTS();
}
