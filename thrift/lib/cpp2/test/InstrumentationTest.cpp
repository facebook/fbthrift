/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <folly/ThreadLocal.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/Request.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/async/FutureRequest.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/RequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <thrift/lib/cpp2/server/Cpp2Worker.h>
#include <thrift/lib/cpp2/server/ServerInstrumentation.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/test/gen-cpp2/DebugTestService.h>
#include <thrift/lib/cpp2/test/gen-cpp2/InstrumentationTestService.h>
#include <thrift/lib/cpp2/transport/rsocket/server/RSRoutingHandler.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include <atomic>
#include <thread>

using namespace apache::thrift;
using namespace apache::thrift::concurrency;
using namespace apache::thrift::test;

class RequestPayload : public folly::RequestData {
 public:
  static const folly::RequestToken& getRequestToken() {
    static folly::RequestToken token("InstrumentationTest::RequestPayload");
    return token;
  }
  explicit RequestPayload(std::unique_ptr<folly::IOBuf> buf)
      : buf_(std::move(buf)) {}
  bool hasCallback() override {
    return false;
  }
  const folly::IOBuf* getPayload() {
    return buf_.get();
  }

 private:
  std::unique_ptr<folly::IOBuf> buf_;
};

class InstrumentationTestProcessor
    : public InstrumentationTestServiceAsyncProcessor {
 public:
  explicit InstrumentationTestProcessor(InstrumentationTestServiceSvIf* iface)
      : InstrumentationTestServiceAsyncProcessor(iface) {}

  /**
   * Intercepts and clones incoming payload buffers, then passes them down to
   * method handlers.
   */
  void process(
      ResponseChannelRequest::UniquePtr req,
      std::unique_ptr<folly::IOBuf> buf,
      apache::thrift::protocol::PROTOCOL_TYPES protType,
      apache::thrift::Cpp2RequestContext* context,
      folly::EventBase* eb,
      apache::thrift::concurrency::ThreadManager* tm) override {
    folly::IOBufQueue queue;
    queue.append(buf->clone());
    queue.trimStart(context->getMessageBeginSize());
    folly::RequestContext::get()->setContextData(
        RequestPayload::getRequestToken(),
        std::make_unique<RequestPayload>(queue.move()));
    InstrumentationTestServiceAsyncProcessor::process(
        std::move(req), std::move(buf), protType, context, eb, tm);
  }
};

class TestInterface : public InstrumentationTestServiceSvIf {
 public:
  auto requestGuard() {
    reqCount_++;
    return folly::makeGuard([&] { --reqCount_; });
  }

  folly::coro::Task<void> co_sendRequest() override {
    auto rg = requestGuard();
    auto ctx = folly::RequestContext::get();
    EXPECT_TRUE(
        ctx->setContextDataIfAbsent("TestInterface::co_sendRequest", nullptr));
    co_await finished_;
  }

  folly::coro::Task<apache::thrift::ServerStream<int32_t>>
  co_sendStreamingRequest() override {
    auto rg = requestGuard();
    co_await finished_;
    co_return folly::coro::co_invoke(
        []() -> folly::coro::AsyncGenerator<int32_t&&> { co_yield 0; });
  }

  folly::coro::Task<apache::thrift::test::IOBuf> co_sendPayload(
      int32_t id,
      const std::string& /* str */) override {
    auto rg = requestGuard();
    auto payload = dynamic_cast<RequestPayload*>(
        folly::RequestContext::get()->getContextData(
            RequestPayload::getRequestToken()));
    EXPECT_NE(payload, nullptr);
    co_await finished_;
    co_return * payload->getPayload()->clone();
  }

  void stopRequests() {
    finished_.post();
    while (reqCount_ > 0) {
      std::this_thread::yield();
    }
  }

  void waitForRequests(int32_t num) {
    while (reqCount_ != num) {
      std::this_thread::yield();
    }
  }

  folly::coro::Task<void> co_wait(int value, bool busyWait, bool shallowRC)
      override {
    std::unique_ptr<folly::ShallowCopyRequestContextScopeGuard> g;
    if (shallowRC) {
      g = std::make_unique<folly::ShallowCopyRequestContextScopeGuard>();
    }
    auto rg = requestGuard();
    if (busyWait) {
      while (!finished_.ready()) {
        std::this_thread::yield();
      }
    }
    co_await finished_;
  }

  std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() override {
    return std::make_unique<InstrumentationTestProcessor>(this);
  }

 private:
  folly::coro::Baton finished_;
  std::atomic<int32_t> reqCount_{0};
};

class DebugInterface : public DebugTestServiceSvIf {
 public:
  void echo(std::string& r, const std::string& s) override {
    r = folly::format("{}:{}", s, folly::getCurrentThreadName().value()).str();
  }
};

class DebuggingFrameHandler : public rocket::SetupFrameHandler {
 public:
  explicit DebuggingFrameHandler(ThriftServer& server)
      : origServer_(server), reqRegistry_([] {
          auto p = new ActiveRequestsRegistry(0, 0, 0);
          return new std::shared_ptr<ActiveRequestsRegistry>(p);
        }) {
    auto tf =
        std::make_shared<PosixThreadFactory>(PosixThreadFactory::ATTACHED);
    tm_ = std::make_shared<SimpleThreadManager<folly::LifoSem>>(1, false);
    tm_->setNamePrefix("DebugInterface");
    tm_->threadFactory(move(tf));
    tm_->start();
  }
  folly::Optional<rocket::ProcessorInfo> tryHandle(
      const RequestSetupMetadata& meta) override {
    if (meta.interfaceKind_ref().has_value() &&
        meta.interfaceKind_ref().value() == InterfaceKind::DEBUGGING) {
      auto info = rocket::ProcessorInfo{
          debug_.getProcessor(), tm_, origServer_, *reqRegistry_.get()};
      return folly::Optional<rocket::ProcessorInfo>(std::move(info));
    }
    return folly::Optional<rocket::ProcessorInfo>();
  }

 private:
  ThriftServer& origServer_;
  DebugInterface debug_;
  std::shared_ptr<ThreadManager> tm_;
  folly::ThreadLocal<std::shared_ptr<ActiveRequestsRegistry>> reqRegistry_;
};

class RequestInstrumentationTest : public testing::Test {
 protected:
  RequestInstrumentationTest() {}

  void SetUp() override {
    impl_ = std::make_unique<Impl>();
  }

  std::shared_ptr<TestInterface> handler() {
    return impl_->handler_;
  }
  apache::thrift::ScopedServerInterfaceThread& server() {
    return impl_->server_;
  }
  ThriftServer* thriftServer() {
    return impl_->thriftServer_;
  }

  std::vector<ThriftServer::RequestSnapshot> getRequestSnapshots(
      size_t reqNum) {
    SCOPE_EXIT {
      handler()->stopRequests();
    };
    handler()->waitForRequests(reqNum);

    auto serverReqSnapshots = thriftServer()->snapshotActiveRequests().get();

    EXPECT_EQ(serverReqSnapshots.size(), reqNum);
    return serverReqSnapshots;
  }

  std::vector<intptr_t> getRootIdsOnThreads() {
    std::vector<intptr_t> results;
    for (auto& root : folly::RequestContext::getRootIdsFromAllThreads()) {
      if (!root.id) {
        continue;
      }
      results.push_back(root.id);
    }
    return results;
  }

  auto makeHeaderClient() {
    return server().newClient<InstrumentationTestServiceAsyncClient>(
        nullptr, [&](auto socket) mutable {
          return HeaderClientChannel::newChannel(std::move(socket));
        });
  }
  auto makeRocketClient() {
    return server().newClient<InstrumentationTestServiceAsyncClient>(
        nullptr, [&](auto socket) mutable {
          return RocketClientChannel::newChannel(std::move(socket));
        });
  }

  auto makeDebugClient() {
    return server().newClient<DebugTestServiceAsyncClient>(
        nullptr, [](auto socket) mutable {
          RequestSetupMetadata meta;
          meta.set_interfaceKind(InterfaceKind::DEBUGGING);
          return apache::thrift::RocketClientChannel::newChannel(
              std::move(socket), std::move(meta));
        });
  }

  struct Impl {
    Impl(ScopedServerInterfaceThread::ServerConfigCb&& serverCfgCob = {})
        : handler_(std::make_shared<TestInterface>()),
          server_(handler_, "::1", 0, std::move(serverCfgCob)),
          thriftServer_(
              dynamic_cast<ThriftServer*>(&server_.getThriftServer())) {}
    std::shared_ptr<TestInterface> handler_;
    apache::thrift::ScopedServerInterfaceThread server_;
    ThriftServer* thriftServer_;
  };
  std::unique_ptr<Impl> impl_;
};

TEST_F(RequestInstrumentationTest, simpleRocketRequestTest) {
  size_t reqNum = 5;
  auto client = server().newClient<InstrumentationTestServiceAsyncClient>(
      nullptr, [](auto socket) mutable {
        return apache::thrift::RocketClientChannel::newChannel(
            std::move(socket));
      });

  for (size_t i = 0; i < reqNum; i++) {
    client->semifuture_sendStreamingRequest();
    client->semifuture_sendRequest();
  }
  handler()->waitForRequests(2 * reqNum);

  // we expect all requests to get off the thread, eventually
  int attempt = 0;
  while (attempt < 5 && getRootIdsOnThreads().size() > 0) {
    sleep(1);
    attempt++;
  }
  EXPECT_LT(attempt, 5);

  for (auto& reqSnapshot : getRequestSnapshots(2 * reqNum)) {
    auto methodName = reqSnapshot.getMethodName();
    EXPECT_NE(reqSnapshot.getRootRequestContextId(), 0);
    EXPECT_TRUE(
        methodName == "sendRequest" || methodName == "sendStreamingRequest");
  }
}

TEST_F(RequestInstrumentationTest, threadSnapshot) {
  auto client = server().newClient<InstrumentationTestServiceAsyncClient>(
      nullptr, [](auto socket) mutable {
        return apache::thrift::RocketClientChannel::newChannel(
            std::move(socket));
      });

  client->semifuture_sendRequest();
  handler()->waitForRequests(1);
  client->semifuture_wait(0, true, false);

  handler()->waitForRequests(2);
  auto onThreadReqs = getRootIdsOnThreads();
  auto allReqs = getRequestSnapshots(2);
  EXPECT_EQ(1, onThreadReqs.size());
  EXPECT_EQ(2, allReqs.size());

  for (const auto& req : allReqs) {
    if (req.getMethodName() == "wait") {
      EXPECT_EQ(req.getRootRequestContextId(), onThreadReqs[0]);
    } else {
      EXPECT_NE(req.getRootRequestContextId(), onThreadReqs[0]);
    }
  }
}

TEST_F(RequestInstrumentationTest, threadSnapshotWithShallowRC) {
  auto client = makeRocketClient();

  client->semifuture_sendRequest();
  handler()->waitForRequests(1);
  client->semifuture_wait(0, true, true);
  handler()->waitForRequests(2);

  auto onThreadReqs = getRootIdsOnThreads();
  auto allReqs = getRequestSnapshots(2);
  EXPECT_EQ(1, onThreadReqs.size());
  EXPECT_EQ(2, allReqs.size());

  for (const auto& req : allReqs) {
    if (req.getMethodName() == "wait") {
      EXPECT_EQ(req.getRootRequestContextId(), onThreadReqs.front());
    } else {
      EXPECT_NE(req.getRootRequestContextId(), onThreadReqs.front());
    }
  }
}

TEST_F(RequestInstrumentationTest, debugInterfaceTest) {
  size_t reqNum = 5;

  // inject our setup frame handler for rocket connections
  for (const auto& rh : *thriftServer()->getRoutingHandlers()) {
    auto rs = dynamic_cast<RSRoutingHandler*>(rh.get());
    if (rs != nullptr) {
      rs->addSetupFrameHandler(
          std::make_unique<DebuggingFrameHandler>(*thriftServer()));
      break;
    }
  }

  auto client = makeRocketClient();
  auto debugClient = makeDebugClient();
  for (size_t i = 0; i < reqNum; i++) {
    client->semifuture_sendStreamingRequest();
    client->semifuture_sendRequest();
  }

  auto echoed = debugClient->semifuture_echo("echome").get();
  EXPECT_TRUE(folly::StringPiece(echoed).startsWith("echome:DebugInterface-"));

  for (auto& reqSnapshot : getRequestSnapshots(2 * reqNum)) {
    auto methodName = reqSnapshot.getMethodName();
    EXPECT_TRUE(
        methodName == "sendRequest" || methodName == "sendStreamingRequest");
  }
}

TEST_F(RequestInstrumentationTest, simpleHeaderRequestTest) {
  size_t reqNum = 5;
  auto client = makeHeaderClient();

  for (size_t i = 0; i < reqNum; i++) {
    client->semifuture_sendRequest();
  }

  for (auto& reqSnapshot : getRequestSnapshots(reqNum)) {
    EXPECT_EQ(reqSnapshot.getMethodName(), "sendRequest");
    EXPECT_NE(reqSnapshot.getRootRequestContextId(), 0);
    EXPECT_EQ(
        reqSnapshot.getFinishedTimestamp(),
        std::chrono::steady_clock::time_point{
            std::chrono::steady_clock::duration::zero()});
  }
}

TEST_F(RequestInstrumentationTest, requestPayloadTest) {
  std::array<std::string, 4> strList = {
      "apache", "thrift", "test", "InstrumentationTest"};
  auto client = makeHeaderClient();

  std::vector<folly::SemiFuture<folly::IOBuf>> tasks;
  int32_t reqId = 0;
  for (const auto& testStr : strList) {
    auto task = client->semifuture_sendPayload(reqId, testStr)
                    .deferValue([](const apache::thrift::test::IOBuf& buf) {
                      return buf;
                    });
    tasks.emplace_back(std::move(task));
    reqId++;
  }
  auto reqSnapshots = getRequestSnapshots(strList.size());

  auto interceptedPayloadList =
      folly::collectSemiFuture(tasks.begin(), tasks.end()).get();

  std::set<const folly::IOBuf*, folly::IOBufLess> snapshotPayloadSet;
  for (const auto& reqSnapshot : reqSnapshots) {
    snapshotPayloadSet.insert(&reqSnapshot.getPayload());
  }

  EXPECT_EQ(interceptedPayloadList.size(), snapshotPayloadSet.size());
  for (const auto& interceptedPayload : interceptedPayloadList) {
    auto it = snapshotPayloadSet.find(&interceptedPayload);
    EXPECT_TRUE(it != snapshotPayloadSet.end());
  }
}

class ServerInstrumentationTest : public testing::Test {};

TEST_F(ServerInstrumentationTest, simpleServerTest) {
  ThriftServer server0;
  {
    auto handler = std::make_shared<TestInterface>();
    apache::thrift::ScopedServerInterfaceThread server1(handler);
    EXPECT_EQ(ServerInstrumentation::getServerCount(), 2);
  }
  EXPECT_EQ(ServerInstrumentation::getServerCount(), 1);
}

class RequestInstrumentationTestP
    : public RequestInstrumentationTest,
      public ::testing::WithParamInterface<std::tuple<int, int, bool>> {
 protected:
  size_t finishedRequestsLimit;
  size_t reqNum;
  bool rocket;
  void SetUp() override {
    std::tie(finishedRequestsLimit, reqNum, rocket) = GetParam();
    impl_ = std::make_unique<Impl>([&](auto& ts) {
      ts.setMaxFinishedDebugPayloadsPerWorker(finishedRequestsLimit);
    });
  }
};

TEST_P(RequestInstrumentationTestP, FinishedRequests) {
  auto client = rocket ? makeRocketClient() : makeHeaderClient();

  for (size_t i = 0; i < reqNum; i++) {
    client->semifuture_sendRequest();
  }

  handler()->waitForRequests(reqNum);
  handler()->stopRequests();

  auto serverReqSnapshots = thriftServer()->snapshotActiveRequests().get();
  EXPECT_EQ(serverReqSnapshots.size(), std::min(reqNum, finishedRequestsLimit));
  for (auto& req : serverReqSnapshots) {
    EXPECT_EQ(req.getMethodName(), "sendRequest");
    EXPECT_NE(req.getRootRequestContextId(), 0);
    EXPECT_NE(
        req.getFinishedTimestamp(),
        std::chrono::steady_clock::time_point{
            std::chrono::steady_clock::duration::zero()});
  }
}

INSTANTIATE_TEST_CASE_P(
    FinishedRequestsSequence,
    RequestInstrumentationTestP,
    testing::Combine(
        testing::Values(0, 3, 20),
        testing::Values(3, 10),
        testing::Values(true, false)));
