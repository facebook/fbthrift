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
#include <thrift/lib/cpp/test/SocketPair.h>
#include <thrift/lib/cpp/EventHandlerBase.h>
#include <thrift/lib/cpp/async/TAsyncTimeout.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <folly/SocketAddress.h>
#include <thrift/lib/cpp2/async/StubSaslClient.h>
#include <thrift/lib/cpp2/async/StubSaslServer.h>

using namespace apache::thrift;
using namespace apache::thrift::async;
using namespace apache::thrift::test;
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

class EventBaseAborter : public TAsyncTimeout {
 public:
  EventBaseAborter(TEventBase* eventBase, uint32_t timeoutMS)
    : TAsyncTimeout(eventBase, TAsyncTimeout::InternalEnum::INTERNAL)
    , eventBase_(eventBase) {
    scheduleTimeout(timeoutMS);
  }

  virtual void timeoutExpired() noexcept {
    EXPECT_TRUE(false);
    eventBase_->terminateLoopSoon();
  }

 private:
  TEventBase* eventBase_;
};

// Creates/unwraps a framed message (LEN(MSG) | MSG)
class FramingHandler : public FramingChannelHandler {
public:
  std::pair<unique_ptr<IOBuf>, size_t> removeFrame(IOBufQueue* q) override {
    assert(q);
    queue_.append(*q);
    if (!queue_.front() || queue_.front()->empty()) {
      return make_pair(std::unique_ptr<IOBuf>(), 0);
    }

    uint32_t len = queue_.front()->computeChainDataLength();

    if (len < 4) {
      size_t remaining = 4 - len;
      return make_pair(unique_ptr<IOBuf>(), remaining);
    }

    folly::io::Cursor c(queue_.front());
    uint32_t msgLen = c.readBE<uint32_t>();
    if (len - 4 < msgLen) {
      size_t remaining = msgLen - (len - 4);
      return make_pair(unique_ptr<IOBuf>(), remaining);
    }

    queue_.trimStart(4);
    return make_pair(queue_.split(msgLen), 0);
  }

  unique_ptr<IOBuf> addFrame(unique_ptr<IOBuf> buf) override {
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

    return std::move(framing);
  }
private:
  IOBufQueue queue_;
};

template <typename Channel>
unique_ptr<Channel, TDelayedDestruction::Destructor> createChannel(
    const shared_ptr<TAsyncTransport>& transport) {
  return Channel::newChannel(transport);
}

template <>
unique_ptr<Cpp2Channel, TDelayedDestruction::Destructor> createChannel(
    const shared_ptr<TAsyncTransport>& transport) {
  return Cpp2Channel::newChannel(transport, make_unique<FramingHandler>());
}

template<typename Channel1, typename Channel2>
class SocketPairTest {
 public:
  SocketPairTest()
    : eventBase_() {
    SocketPair socketPair;

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
  TEventBase eventBase_;
  shared_ptr<TAsyncSocket> socket0_;
  shared_ptr<TAsyncSocket> socket1_;
  unique_ptr<Channel1, TDelayedDestruction::Destructor> channel0_;
  unique_ptr<Channel2, TDelayedDestruction::Destructor> channel1_;
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

  virtual void sendQueued() { }

  virtual void messageSent() {
    sent_++;
  }
  virtual void messageSendError(folly::exception_wrapper&& ex) {
    sendError_++;
  }

  virtual void messageReceived(unique_ptr<IOBuf>&& buf,
                               unique_ptr<sample>) {
    recv_++;
    recvBytes_ += buf->computeChainDataLength();
  }
  virtual void messageChannelEOF() {
    recvEOF_++;
  }
  virtual void messageReceiveErrorWrapped(folly::exception_wrapper&& ex) {
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
  virtual void requestSent() {}
  virtual void replyReceived(ClientReceiveState&& state) {
    reply_++;
    replyBytes_ += state.buf()->computeChainDataLength();
    if (state.isSecurityActive()) {
      replySecurityActive_++;
    }
  }
  virtual void requestError(ClientReceiveState&& state) {
    std::exception_ptr ex = state.exception();
    try {
      std::rethrow_exception(ex);
    } catch (const std::exception& e) {
      // Verify that exception pointer is passed properly
    }
    replyError_++;
  }
  virtual void channelClosed() {
    closed_ = true;
  }

  static void reset() {
    closed_ = false;
    reply_ = 0;
    replyBytes_ = 0;
    replyError_ = 0;
    replySecurityActive_ = 0;
  }

  static bool closed_;
  static uint32_t reply_;
  static uint32_t replyBytes_;
  static uint32_t replyError_;
  static uint32_t replySecurityActive_;
};

bool TestRequestCallback::closed_ = false;
uint32_t TestRequestCallback::reply_ = 0;
uint32_t TestRequestCallback::replyBytes_ = 0;
uint32_t TestRequestCallback::replyError_ = 0;
uint32_t TestRequestCallback::replySecurityActive_ = 0;

class ResponseCallback
    : public ResponseChannel::Callback {
 public:
  ResponseCallback()
      : serverClosed_(false)
      , oneway_(0)
      , request_(0)
      , requestBytes_(0) {}

  virtual void requestReceived(unique_ptr<ResponseChannel::Request>&& req) {
    request_++;
    requestBytes_ += req->getBuf()->computeChainDataLength();
    if (req->isOneway()) {
      oneway_++;
    } else {
      req->sendReply(req->extractBuf());
    }
  }

  virtual void channelClosed(folly::exception_wrapper&& ew) {
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
      : len_(len) {
  }

  void preLoop() {
    channel0_->sendMessage(&sendCallback_, makeTestBuf(len_));
    channel1_->setReceiveCallback(this);
  }

  void postLoop() {
    EXPECT_EQ(sendCallback_.sendError_, 0);
    EXPECT_EQ(recvError_, 0);
    EXPECT_EQ(recvEOF_, 0);
    EXPECT_EQ(recv_, 1);
    EXPECT_EQ(sendCallback_.sent_, 1);
    EXPECT_EQ(recvBytes_, len_);
  }

  virtual void messageReceived(unique_ptr<IOBuf>&& buf,
                               unique_ptr<sample> sample) {
    MessageCallback::messageReceived(std::move(buf), std::move(sample));
    channel1_->setReceiveCallback(nullptr);
  }


 private:
  size_t len_;
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
  void preLoop() {
    channel0_->sendMessage(&sendCallback_, makeTestBuf(1024*1024));
    // Close the other socket after delay
    this->eventBase_.runInLoop(
      std::bind(&TAsyncSocket::close, this->socket1_.get()));
    channel1_->setReceiveCallback(this);
  }

  void postLoop() {
    EXPECT_EQ(sendCallback_.sendError_, 1);
    EXPECT_EQ(recvError_, 0);
    EXPECT_EQ(recvEOF_, 1);
    EXPECT_EQ(recv_, 0);
    EXPECT_EQ(sendCallback_.sent_, 0);
  }

  virtual void messageChannelEOF() {
    MessageCallback::messageChannelEOF();
    channel1_->setReceiveCallback(nullptr);
  }

 private:
  MessageCallback sendCallback_;
};

TEST(Channel, MessageCloseTest) {
  MessageCloseTest().run();
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
    void replyReceived(ClientReceiveState&& state) {
      TestRequestCallback::replyReceived(std::move(state));
      c_->channel1_->setCallback(nullptr);
    }
   private:
    HeaderChannelTest* c_;
  };

  void preLoop() {
    TestRequestCallback::reset();
    channel1_->setCallback(this);
    channel0_->setCloseCallback(this);
    channel0_->sendOnewayRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_));
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_));
    channel0_->setCloseCallback(nullptr);
  }

  void postLoop() {
    EXPECT_EQ(reply_, 1);
    EXPECT_EQ(replyError_, 0);
    EXPECT_EQ(replyBytes_, len_);
    EXPECT_EQ(closed_, false);
    EXPECT_EQ(serverClosed_, false);
    EXPECT_EQ(request_, 2);
    EXPECT_EQ(requestBytes_, len_*2);
    EXPECT_EQ(oneway_, 1);
    channel1_->setCallback(nullptr);
  }

 private:
  size_t len_;
};

TEST(Channel, HeaderChannelTest) {
  HeaderChannelTest(1).run();
  HeaderChannelTest(100).run();
  HeaderChannelTest(1024*1024).run();
}

class SecurityNegotiationTest
    : public SocketPairTest<HeaderClientChannel, HeaderServerChannel>
    , public TestRequestCallback
    , public ResponseCallback {
public:
  explicit SecurityNegotiationTest(bool clientSasl, bool clientNonSasl,
                                   bool serverSasl, bool serverNonSasl,
                                   bool expectConn, bool expectSecurity)
      : clientSasl_(clientSasl), clientNonSasl_(clientNonSasl)
      , serverSasl_(serverSasl), serverNonSasl_(serverNonSasl)
      , expectConn_(expectConn), expectSecurity_(expectSecurity) {
    // Replace handshake mechanism with a stub.
    stubSaslClient_ = new StubSaslClient(socket0_->getEventBase());
    channel0_->setSaslClient(std::unique_ptr<SaslClient>(stubSaslClient_));
    stubSaslServer_ = new StubSaslServer(socket1_->getEventBase());
    channel1_->setSaslServer(std::unique_ptr<SaslServer>(stubSaslServer_));
  }

  ~SecurityNegotiationTest() {
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
    void replyReceived(ClientReceiveState&& state) {
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

  virtual void channelClosed() {
    EXPECT_EQ(channel0_->getSaslPeerIdentity(), "");
    TestRequestCallback::channelClosed();
  }

  virtual void channelClosed(folly::exception_wrapper&& ew) {
    EXPECT_EQ(channel1_->getSaslPeerIdentity(), "");
    ResponseCallback::channelClosed(std::move(ew));
    channel1_->setCallback(nullptr);
  }

  void preLoop() {
    TestRequestCallback::reset();
    if (clientSasl_ && clientNonSasl_) {
      channel0_->getHeader()->setSecurityPolicy(THRIFT_SECURITY_PERMITTED);
    } else if (clientSasl_) {
      channel0_->getHeader()->setSecurityPolicy(THRIFT_SECURITY_REQUIRED);
    } else {
      channel0_->getHeader()->setSecurityPolicy(THRIFT_SECURITY_DISABLED);
    }

    if (serverSasl_ && serverNonSasl_) {
      channel1_->getHeader()->setSecurityPolicy(THRIFT_SECURITY_PERMITTED);
    } else if (serverSasl_) {
      channel1_->getHeader()->setSecurityPolicy(THRIFT_SECURITY_REQUIRED);
    } else {
      channel1_->getHeader()->setSecurityPolicy(THRIFT_SECURITY_DISABLED);
    }

    channel1_->setCallback(this);
    channel0_->setCloseCallback(this);
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(1));
  }

  void postLoop() {
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
    } else {
      EXPECT_EQ(replySecurityActive_, 0);
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

TEST(Channel, SecurityNegotiationTest) {
  //clientSasl clientNonSasl serverSasl serverNonSasl expectConn expectSecurity
  SecurityNegotiationTest(false, false, false, false, true, false).run();
  SecurityNegotiationTest(false, false, false, true, true, false).run();
  SecurityNegotiationTest(false, false, true, false, false, false).run();
  SecurityNegotiationTest(false, false, true, true, true, false).run();

  SecurityNegotiationTest(false, true, false, false, true, false).run();
  SecurityNegotiationTest(false, true, false, true, true, false).run();
  SecurityNegotiationTest(false, true, true, false, false, false).run();
  SecurityNegotiationTest(false, true, true, true, true, false).run();

  SecurityNegotiationTest(true, false, false, false, false, false).run();
  SecurityNegotiationTest(true, false, false, true, false, false).run();
  SecurityNegotiationTest(true, false, true, false, true, true).run();
  SecurityNegotiationTest(true, false, true, true, true, true).run();

  SecurityNegotiationTest(true, true, false, false, true, false).run();
  SecurityNegotiationTest(true, true, false, true, true, false).run();
  SecurityNegotiationTest(true, true, true, false, true, true).run();
  SecurityNegotiationTest(true, true, true, true, true, true).run();
}

TEST(Channel, SecurityNegotiationFailTest) {
  SecurityNegotiationServerFailTest(true, false, true, true, false,false).run();

  SecurityNegotiationClientFailTest(true, true, true, false, false,false).run();

  SecurityNegotiationClientFailTest(true, true, true, true, true, false).run();
  SecurityNegotiationServerFailTest(true, true, true, true, true, false).run();
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
    void replyReceived(ClientReceiveState&& state) {
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

  void preLoop() {
    TestRequestCallback::reset();
    channel0_->getHeader()->setFlags(0); // turn off out of order
    channel1_->setCallback(this);
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_));
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_ + 1));
  }

  void postLoop() {
    EXPECT_EQ(reply_, 2);
    EXPECT_EQ(replyError_, 0);
    EXPECT_EQ(replyBytes_, 2*len_ + 1);
    EXPECT_EQ(closed_, false);
    EXPECT_EQ(serverClosed_, false);
    EXPECT_EQ(request_, 2);
    EXPECT_EQ(requestBytes_, 2*len_ + 1);
    EXPECT_EQ(oneway_, 0);
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

    void requestError(ClientReceiveState&& state) {
      c_->channel1_->setCallback(nullptr);
      TestRequestCallback::requestError(std::move(state));
    }

   private:
    BadSeqIdTest* c_;
  };

  void requestReceived(unique_ptr<ResponseChannel::Request>&& req) {
    request_++;
    requestBytes_ += req->getBuf()->computeChainDataLength();
    if (req->isOneway()) {
      oneway_++;
      return;
    }
    std::map<std::string, std::string> headers;
    std::vector<uint16_t> transforms;
    HeaderServerChannel::HeaderRequest r(
      -1 /* bad seqid */, channel1_.get(),
      req->extractBuf(), headers, transforms,
      true /*out of order*/,
      std::unique_ptr<MessageChannel::RecvCallback::sample>(nullptr));
    r.sendReply(r.extractBuf());
  }

  void preLoop() {
    TestRequestCallback::reset();
    channel0_->setTimeout(1000);
    channel1_->setCallback(this);
    channel0_->sendOnewayRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_));
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new Callback(this)),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_));
  }

  void postLoop() {
    EXPECT_EQ(reply_, 0);
    EXPECT_EQ(replyError_, 1);
    EXPECT_EQ(replyBytes_, 0);
    EXPECT_EQ(closed_, false);
    EXPECT_EQ(serverClosed_, false);
    EXPECT_EQ(request_, 2);
    EXPECT_EQ(requestBytes_, len_*2);
    EXPECT_EQ(oneway_, 1);
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

  void preLoop() {
    TestRequestCallback::reset();
    channel1_->setCallback(this);
    channel0_->setTimeout(timeout_);
    channel0_->setCloseCallback(this);
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new TestRequestCallback),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_));
    channel0_->sendRequest(
      std::unique_ptr<RequestCallback>(new TestRequestCallback),
      // Fake method name for creating a ContextStatck
      std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
      makeTestBuf(len_));
  }

  void postLoop() {
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
  }

  void requestReceived(unique_ptr<ResponseChannel::Request>&& req) {
    request_++;
    requestBytes_ += req->getBuf()->computeChainDataLength();
    // Don't respond, let it time out
    // TestRequestCallback::replyReceived(std::move(buf));
    channel1_->getEventBase()->runAfterDelay(
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

  void preLoop() {
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
        makeTestBuf(len_));
    // Verify the timeout worked within 10ms
    channel0_->getEventBase()->runAfterDelay([&](){
        EXPECT_EQ(replyError_, 1);
      }, 35);
    // Verify that subsequent successful requests don't delay timeout
    channel0_->getEventBase()->runAfterDelay([&](){
        channel0_->sendRequest(
          std::unique_ptr<RequestCallback>(new TestRequestCallback),
          // Fake method name for creating a ContextStatck
          std::unique_ptr<ContextStack>(new ContextStack("{ChannelTest}")),
          makeTestBuf(len_));
      }, 20);
  }

  void postLoop() {
    EXPECT_EQ(reply_, 1);
    EXPECT_EQ(replyError_, 1);
    EXPECT_EQ(replyBytes_, 1);
    EXPECT_EQ(closed_, false); // client timeouts do not close connection
    EXPECT_EQ(serverClosed_, false);
    EXPECT_EQ(request_, 2);
    EXPECT_EQ(requestBytes_, len_ * 2);
    EXPECT_EQ(oneway_, 0);
  }

  void requestReceived(unique_ptr<ResponseChannel::Request>&& req) {
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

  void preLoop() {
    TestRequestCallback::reset();
    channel1_->setCallback(this);
    channel0_->setCloseCallback(this);
    if (halfClose_) {
      channel1_->getEventBase()->runAfterDelay(
        [&](){channel1_->getTransport()->shutdownWrite();

        },
        10);
    } else {
      channel1_->getEventBase()->runAfterDelay(
        [&](){channel1_->getTransport()->close();},
        10);
    }
    channel1_->getEventBase()->runAfterDelay(
      [&](){
        channel1_->setCallback(nullptr);
      },
    20);
    channel0_->getEventBase()->runAfterDelay(
      [&](){
        channel0_->setCloseCallback(nullptr);
      },
    20);
  }

  void postLoop() {
    EXPECT_EQ(reply_, 0);
    EXPECT_EQ(replyError_, 0);
    EXPECT_EQ(replyBytes_, 0);
    EXPECT_EQ(closed_, true);
    EXPECT_EQ(serverClosed_, !halfClose_);
    EXPECT_EQ(request_, 0);
    EXPECT_EQ(requestBytes_, 0);
    EXPECT_EQ(oneway_, 0);
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

  void preLoop() {
    TestRequestCallback::reset();
    channel1_->setCallback(this);
    channel0_->setCloseCallback(this);
    if (halfClose_) {
      channel0_->getEventBase()->runAfterDelay(
        [&](){channel0_->getTransport()->shutdownWrite();

        },
        10);
    } else {
      channel0_->getEventBase()->runAfterDelay(
        [&](){channel0_->getTransport()->close();},
        10);
    }
    channel1_->getEventBase()->runAfterDelay(
      [&](){
        channel1_->setCallback(nullptr);
      },
    20);
    channel0_->getEventBase()->runAfterDelay(
      [&](){
        channel0_->setCloseCallback(nullptr);
      },
    20);
  }

  void postLoop() {
    EXPECT_EQ(reply_, 0);
    EXPECT_EQ(replyError_, 0);
    EXPECT_EQ(replyBytes_, 0);
    EXPECT_EQ(closed_, !halfClose_);
    EXPECT_EQ(serverClosed_, true);
    EXPECT_EQ(request_, 0);
    EXPECT_EQ(requestBytes_, 0);
    EXPECT_EQ(oneway_, 0);
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
  void setReadCallback(ReadCallback* callback) { cb_ = callback; }
  ReadCallback* getReadCallback() const { return cb_; }
  void write(WriteCallback* c, const void* v, size_t s, WriteFlags flags) { }
  void writev(WriteCallback* c, const iovec* v, size_t s, WriteFlags flags) { }
  void writeChain(WriteCallback* c,
                  std::unique_ptr<folly::IOBuf>&& i,
                  WriteFlags flags) { }
  void close() { }
  void closeNow() { }
  void shutdownWrite() { }
  void shutdownWriteNow() { }
  bool good() const { return true; }
  bool readable() const { return false; }
  bool connecting() const { return false; }
  bool error() const { return false; }
  void attachEventBase(TEventBase* e) { }
  void detachEventBase() { }
  bool isDetachable() const { return true; }
  TEventBase* getEventBase() const { return nullptr; }
  void setSendTimeout(uint32_t ms) { }
  uint32_t getSendTimeout() const { return 0; }
  void getLocalAddress(folly::SocketAddress* a) const { }
  void getPeerAddress(folly::SocketAddress* a) const { }
  size_t getAppBytesWritten() const { return 0; }
  size_t getRawBytesWritten() const { return 0; }
  size_t getAppBytesReceived() const { return 0; }
  size_t getRawBytesReceived() const { return 0; }
  void setEorTracking(bool track) { }
  bool isEorTrackingEnabled() const { return false; }

  void invokeEOF() {
    cb_->readEOF();
  }
 private:
  ReadCallback* cb_;
};

class DestroyRecvCallback : public MessageChannel::RecvCallback {
 public:
  typedef std::unique_ptr<Cpp2Channel, TDelayedDestruction::Destructor>
      ChannelPointer;
  explicit DestroyRecvCallback(ChannelPointer&& channel)
      : channel_(std::move(channel)),
        invocations_(0) {
    channel_->setReceiveCallback(this);
  }
  void messageReceived(
    std::unique_ptr<folly::IOBuf>&&,
    std::unique_ptr<MessageChannel::RecvCallback::sample> sample) { }
  void messageChannelEOF() {
    EXPECT_EQ(invocations_, 0);
    invocations_++;
    channel_.reset();
  }
  void messageReceiveErrorWrapped(folly::exception_wrapper&&) { }
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
  void channelClosed() { }
};

TEST(Channel, SetKeepRegisteredForClose) {
  int lfd = socket(PF_INET, SOCK_STREAM, 0);
  int rc = listen(lfd, 10);
  CHECK(rc == 0);
  struct sockaddr_in addr;
  socklen_t addrlen = sizeof(addr);
  rc = getsockname(lfd, (struct sockaddr*)&addr, &addrlen);

  TEventBase base;
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
