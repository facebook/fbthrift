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

#include <thrift/conformance/rpcclient/GTestHarnessRPCClient.h>

#include <algorithm>
#include <chrono>
#include <memory>
#include <stdexcept>
#include <string_view>

// Sanitizers add 2-5x overhead to startup; use a longer timeout to avoid flakes
#if __has_feature(address_sanitizer) || defined(__SANITIZE_ADDRESS__) || \
    __has_feature(thread_sanitizer) || defined(__SANITIZE_THREAD__)
constexpr auto kClientTimeout = std::chrono::seconds(60);
#else
constexpr auto kClientTimeout = std::chrono::seconds(30);
#endif

#include <fmt/core.h>
#include <folly/CancellationToken.h>
#include <folly/ScopeGuard.h>
#include <folly/Subprocess.h>
#include <folly/Synchronized.h>
#include <folly/base64.h>
#include <folly/coro/AsyncGenerator.h>
#include <folly/coro/BlockingWait.h>
#include <folly/coro/CurrentExecutor.h>
#include <folly/coro/Sleep.h>
#include <folly/coro/WithCancellation.h>
#include <folly/futures/Future.h>
#include <folly/synchronization/Baton.h>
#include <thrift/conformance/PluggableFunctions.h>
#include <thrift/conformance/RpcStructComparator.h>
#include <thrift/conformance/Utils.h>
#include <thrift/conformance/if/gen-cpp2/RPCConformanceService.h>
#include <thrift/lib/cpp2/async/BiDiStream.h>
#include <thrift/lib/cpp2/async/Sink.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/server/PreprocessParams.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/transport/http2/common/HTTP2RoutingHandler.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>
#include <wangle/ssl/SSLContextConfig.h>

namespace apache::thrift::conformance {

class ConformanceVerificationServer
    : public apache::thrift::ServiceHandler<RPCConformanceService> {
 public:
  explicit ConformanceVerificationServer(const RpcTestCase& testCase)
      : testCase_(testCase) {}

  void getTestCase(RpcTestCase& testCase) override {
    testCase = testCase_;
    getTestReceivedPromise_.setValue();
  }

  void sendTestResult(std::unique_ptr<ClientTestResult> result) override {
    if (serverResult_.getType() == ServerTestResult::Type::streamCancel) {
      streamCancelled_.try_wait_for(std::chrono::seconds(5));
      serverResult_.streamCancel()->cancelled() = cancelled_.load();
    }
    if (serverResult_.getType() == ServerTestResult::Type::sinkChunkTimeout) {
      sinkConsumerDone_.try_wait_for(std::chrono::seconds(5));
    }
    if (serverResult_.getType() == ServerTestResult::Type::sinkAbort) {
      if (sinkConsumerDone_.try_wait_for(std::chrono::seconds(5))) {
        serverResult_.sinkAbort()->aborted() = sinkAborted_;
      }
    }
    if (serverResult_.getType() == ServerTestResult::Type::bidiAbort) {
      if (sinkConsumerDone_.try_wait_for(std::chrono::seconds(5))) {
        serverResult_.bidiAbort()->sinkAborted() = sinkAborted_;
      }
    }
    if (testCase_.serverInstruction()->requestResponseConcurrent()) {
      auto& concurrent = serverResult_.requestResponseConcurrent().emplace();
      concurrent.requests() = *concurrentRequests_.rlock();
      std::sort(
          concurrent.requests()->begin(),
          concurrent.requests()->end(),
          [](const Request& a, const Request& b) {
            return *a.data() < *b.data();
          });
      concurrent.allInFlight() = !concurrentGaveUp_.load();
    }
    clientResultPromise_.setValue(*result);
  }

  // =================== Request-Response ===================
  folly::coro::Task<std::unique_ptr<Response>> co_requestResponseBasic(
      std::unique_ptr<Request> req) override {
    if (auto serverError =
            testCase_.serverInstruction()->requestResponseServerError()) {
      serverResult_.requestResponseServerError().emplace().request() = *req;
      co_return co_await answerServerError(*serverError->error());
    }
    if (auto concurrent =
            testCase_.serverInstruction()->requestResponseConcurrent()) {
      co_return co_await answerLastToFirst(*concurrent, *req);
    }
    if (auto metadata =
            testCase_.serverInstruction()->requestResponseMetadata()) {
      auto& result = serverResult_.requestResponseMetadata().emplace();
      result.request() = *req;
      recordMetadata(result);
      co_return std::make_unique<Response>(*metadata->response());
    }
    serverResult_.requestResponseBasic().emplace().request() = *req;
    co_return std::make_unique<Response>(
        *testCase_.serverInstruction()->requestResponseBasic()->response());
  }

  void requestResponseDeclaredException(std::unique_ptr<Request> req) override {
    serverResult_.requestResponseDeclaredException().emplace().request() = *req;
    throw can_throw(*testCase_.serverInstruction()
                         ->requestResponseDeclaredException()
                         ->userException());
  }

  void requestResponseUndeclaredException(
      std::unique_ptr<Request> req) override {
    if (auto anyException =
            testCase_.serverInstruction()->requestResponseAnyException()) {
      serverResult_.requestResponseAnyException().emplace().request() = *req;
      throwAnyException(*anyException);
    }
    serverResult_.requestResponseUndeclaredException().emplace().request() =
        *req;
    throw std::runtime_error(*testCase_.serverInstruction()
                                  ->requestResponseUndeclaredException()
                                  ->exceptionMessage());
  }

  void requestResponseNoArgVoidResponse() override {
    serverResult_.requestResponseNoArgVoidResponse().emplace();
  }

  void requestResponseTimeout(
      Response&, std::unique_ptr<Request> req) override {
    serverResult_.requestResponseTimeout().emplace().request() = *req;
    folly::coro::blockingWait([&]() -> folly::coro::Task<void> {
      co_await folly::coro::sleep(
          std::chrono::milliseconds(*testCase_.serverInstruction()
                                         ->requestResponseTimeout()
                                         ->timeoutMs()));
    }());
  }

  // =================== Stream ===================
  apache::thrift::ServerStream<Response> streamBasic(
      std::unique_ptr<Request> req) override {
    if (testCase_.serverInstruction()->streamServerError()) {
      serverResult_.streamServerError().emplace().request() = *req;
      co_return;
    }
    if (testCase_.serverInstruction()->streamCancel()) {
      serverResult_.streamCancel().emplace().request() = *req;
      folly::CancellationCallback onCancel(
          co_await folly::coro::co_current_cancellation_token, [this] {
            if (!cancelled_.exchange(true)) {
              streamCancelled_.post();
            }
          });
      for (int64_t i = 0;; ++i) {
        Response payload;
        payload.data() = folly::to<std::string>(i);
        co_yield std::move(payload);
      }
    }
    serverResult_.streamBasic().emplace().request() = *req;
    for (auto payload :
         *testCase_.serverInstruction()->streamBasic()->streamPayloads()) {
      co_yield std::move(payload);
    }
  }

  apache::thrift::ServerStream<Response> streamChunkTimeout(
      std::unique_ptr<Request> req) override {
    serverResult_.streamChunkTimeout().emplace().request() = *req;
    for (auto payload : *testCase_.serverInstruction()
                             ->streamChunkTimeout()
                             ->streamPayloads()) {
      co_yield std::move(payload);
    }
    co_await folly::coro::sleep(
        std::chrono::milliseconds(*testCase_.serverInstruction()
                                       ->streamChunkTimeout()
                                       ->chunkTimeoutMs()));
  }

  apache::thrift::ResponseAndServerStream<Response, Response>
  streamInitialResponse(std::unique_ptr<Request> req) override {
    serverResult_.streamInitialResponse().emplace().request() = *req;
    auto stream = folly::coro::co_invoke(
        [&]() -> folly::coro::AsyncGenerator<Response&&> {
          for (auto payload : *testCase_.serverInstruction()
                                   ->streamInitialResponse()
                                   ->streamPayloads()) {
            co_yield std::move(payload);
          }
        });

    return {
        *testCase_.serverInstruction()
             ->streamInitialResponse()
             ->initialResponse(),
        std::move(stream)};
  }

  apache::thrift::ServerStream<Response> streamCreditTimeout(
      std::unique_ptr<Request> req) override {
    serverResult_.streamCreditTimeout().emplace().request() = *req;
    for (auto payload : *testCase_.serverInstruction()
                             ->streamCreditTimeout()
                             ->streamPayloads()) {
      co_yield std::move(payload);
    }
  }

  apache::thrift::ServerStream<Response> streamDeclaredException(
      std::unique_ptr<Request> req) override {
    serverResult_.streamDeclaredException().emplace().request() = *req;
    throw *testCase_.serverInstruction()
        ->streamDeclaredException()
        ->userException();
    co_return;
  }

  apache::thrift::ServerStream<Response> streamUndeclaredException(
      std::unique_ptr<Request> req) override {
    serverResult_.streamUndeclaredException().emplace().request() = *req;
    throw std::runtime_error(*testCase_.serverInstruction()
                                  ->streamUndeclaredException()
                                  ->exceptionMessage());
    co_return;
  }

  apache::thrift::ServerStream<Response> streamInitialDeclaredException(
      std::unique_ptr<Request> req) override {
    serverResult_.streamInitialDeclaredException().emplace().request() = *req;
    throw *testCase_.serverInstruction()
        ->streamInitialDeclaredException()
        ->userException();
  }

  apache::thrift::ServerStream<Response> streamInitialUndeclaredException(
      std::unique_ptr<Request> req) override {
    serverResult_.streamInitialUndeclaredException().emplace().request() = *req;
    throw std::runtime_error(*testCase_.serverInstruction()
                                  ->streamInitialUndeclaredException()
                                  ->exceptionMessage());
  }

  apache::thrift::ServerStream<Response> streamInitialTimeout(
      std::unique_ptr<Request> req) override {
    serverResult_.streamInitialTimeout().emplace().request() = *req;
    std::this_thread::sleep_for(
        std::chrono::milliseconds(*testCase_.serverInstruction()
                                       ->streamInitialTimeout()
                                       ->timeoutMs()));
    return ServerStream<Response>::createEmpty();
  }

  // =================== Sink ===================
  apache::thrift::SinkConsumer<Request, Response> sinkBasic(
      std::unique_ptr<Request> req) override {
    if (testCase_.serverInstruction()->sinkAbort()) {
      serverResult_.sinkAbort().emplace().request() = *req;
      return consumeUntilAborted();
    }
    serverResult_.sinkBasic().emplace().request() = *req;
    return apache::thrift::SinkConsumer<Request, Response>{
        [&](folly::coro::AsyncGenerator<Request&&> gen)
            -> folly::coro::Task<Response> {
          const auto& instruction = *testCase_.serverInstruction()->sinkBasic();
          auto& payloads = *serverResult_.sinkBasic()->sinkPayloads();
          while (!instruction.consumeCount().has_value() ||
                 static_cast<int64_t>(payloads.size()) <
                     *instruction.consumeCount()) {
            auto item = co_await gen.next();
            if (!item) {
              break;
            }
            payloads.push_back(std::move(*item));
          }
          co_return *instruction.finalResponse();
        },
        static_cast<uint64_t>(
            *testCase_.serverInstruction()->sinkBasic()->bufferSize())};
  }

  apache::thrift::SinkConsumer<Request, Response> consumeUntilAborted() {
    return apache::thrift::SinkConsumer<Request, Response>{
        [&](folly::coro::AsyncGenerator<Request&&> gen)
            -> folly::coro::Task<Response> {
          SCOPE_EXIT {
            sinkConsumerDone_.post();
          };
          try {
            while (co_await gen.next()) {
            }
          } catch (const std::exception&) {
            sinkAborted_ = true;
          }
          co_return Response{};
        },
        1};
  }

  apache::thrift::SinkConsumer<Request, Response> sinkChunkTimeout(
      std::unique_ptr<Request> req) override {
    serverResult_.sinkChunkTimeout().emplace().request() = *req;
    return apache::thrift::SinkConsumer<Request, Response>{
        [&](folly::coro::AsyncGenerator<Request&&> gen)
            -> folly::coro::Task<Response> {
          SCOPE_EXIT {
            sinkConsumerDone_.post();
          };
          try {
            while (auto item = co_await gen.next()) {
              serverResult_.sinkChunkTimeout()->sinkPayloads()->push_back(
                  std::move(*item));
            }
          } catch (const TApplicationException& e) {
            if (e.getType() == TApplicationException::TIMEOUT) {
              serverResult_.sinkChunkTimeout()->chunkTimeoutException() = true;
            }
          }
          co_return *testCase_.serverInstruction()
              ->sinkChunkTimeout()
              ->finalResponse();
        }}
        .setChunkTimeout(
            std::chrono::milliseconds{*testCase_.serverInstruction()
                                           ->sinkChunkTimeout()
                                           ->chunkTimeoutMs()});
  }

  apache::thrift::ResponseAndSinkConsumer<Response, Request, Response>
  sinkInitialResponse(std::unique_ptr<Request> req) override {
    serverResult_.sinkInitialResponse().emplace().request() = *req;

    return {
        *testCase_.serverInstruction()
             ->sinkInitialResponse()
             ->initialResponse(),
        apache::thrift::SinkConsumer<Request, Response>{
            [&](folly::coro::AsyncGenerator<Request&&> gen)
                -> folly::coro::Task<Response> {
              while (auto item = co_await gen.next()) {
                serverResult_.sinkInitialResponse()->sinkPayloads()->push_back(
                    std::move(*item));
              }
              co_return *testCase_.serverInstruction()
                  ->sinkInitialResponse()
                  ->finalResponse();
            },
            static_cast<uint64_t>(*testCase_.serverInstruction()
                                       ->sinkInitialResponse()
                                       ->bufferSize())}};
  }

  apache::thrift::SinkConsumer<Request, Response> sinkDeclaredException(
      std::unique_ptr<Request> req) override {
    auto& result = serverResult_.sinkDeclaredException().emplace();
    result.request() = *req;
    return {
        [&](folly::coro::AsyncGenerator<Request&&> gen)
            -> folly::coro::Task<Response> {
          try {
            std::ignore = co_await gen.next();
            throw std::logic_error("Publisher didn't throw");
          } catch (const UserException& e) {
            result.userException() = e;
            throw;
          } catch (...) {
            throw std::logic_error(
                fmt::format(
                    "Publisher threw undeclared exception: {}",
                    folly::exception_wrapper(std::current_exception()).what()));
          }
        },
        static_cast<uint64_t>(*testCase_.serverInstruction()
                                   ->sinkDeclaredException()
                                   ->bufferSize())};
  }

  apache::thrift::SinkConsumer<Request, Response> sinkUndeclaredException(
      std::unique_ptr<Request> req) override {
    auto& result = serverResult_.sinkUndeclaredException().emplace();
    result.request() = *req;
    return {
        [&](folly::coro::AsyncGenerator<Request&&> gen)
            -> folly::coro::Task<Response> {
          try {
            std::ignore = co_await gen.next();
            throw std::logic_error("Publisher didn't throw");
          } catch (const TApplicationException& e) {
            result.exceptionMessage() = e.getMessage();
            throw;
          }
        },
        static_cast<uint64_t>(*testCase_.serverInstruction()
                                   ->sinkUndeclaredException()
                                   ->bufferSize())};
  }

  apache::thrift::SinkConsumer<Request, Response> sinkInitialDeclaredException(
      std::unique_ptr<Request> req) override {
    serverResult_.sinkInitialDeclaredException().emplace().request() = *req;
    // TODO: use ->userException() once IDL change lands in generated code
    UserException ex;
    ex.msg() = "initial declared exception";
    folly::throw_exception(std::move(ex));
  }

  apache::thrift::SinkConsumer<Request, Response> sinkServerDeclaredException(
      std::unique_ptr<Request> req) override {
    auto& result = serverResult_.sinkServerDeclaredException().emplace();
    result.request() = *req;
    return {
        [&](folly::coro::AsyncGenerator<Request&&> gen)
            -> folly::coro::Task<Response> {
          while (auto item = co_await gen.next()) {
            // consume sink payloads
          }
          // TODO: use ->userException() once IDL change lands in generated code
          UserException ex;
          ex.msg() = "server declared exception";
          folly::throw_exception(std::move(ex));
        },
        static_cast<uint64_t>(*testCase_.serverInstruction()
                                   ->sinkServerDeclaredException()
                                   ->bufferSize())};
  }
  // =================== BiDi Streaming ===================
  folly::coro::Task<apache::thrift::StreamTransformation<Request, Response>>
  co_bidiBasic(std::unique_ptr<Request> req) override {
    if (auto instruction = testCase_.serverInstruction()->bidiAbort()) {
      serverResult_.bidiAbort().emplace().request() = *req;
      if (auto delayMs = instruction->initialResponseDelayMs()) {
        co_await folly::coro::sleep(std::chrono::milliseconds(*delayMs));
      }
      co_return transformUntilAborted();
    }
    serverResult_.bidiBasic().emplace().request() = *req;
    co_return apache::thrift::StreamTransformation<Request, Response>{
        [this](folly::coro::AsyncGenerator<Request&&> input)
            -> folly::coro::AsyncGenerator<Response&&> {
          for (auto payload :
               *testCase_.serverInstruction()->bidiBasic()->streamPayloads()) {
            co_yield std::move(payload);
          }
          while (auto item = co_await input.next()) {
            serverResult_.bidiBasic()->sinkPayloads()->push_back(
                std::move(*item));
          }
        }};
  }

  apache::thrift::StreamTransformation<Request, Response>
  transformUntilAborted() {
    return apache::thrift::StreamTransformation<Request, Response>{
        [this](folly::coro::AsyncGenerator<Request&&> input)
            -> folly::coro::AsyncGenerator<Response&&> {
          SCOPE_EXIT {
            sinkConsumerDone_.post();
          };
          try {
            while (co_await folly::coro::co_withCancellation(
                folly::CancellationToken{}, input.next())) {
            }
          } catch (const std::exception&) {
            sinkAborted_ = true;
          }
        }};
  }

  folly::coro::Task<
      apache::thrift::
          ResponseAndStreamTransformation<Response, Request, Response>>
  co_bidiInitialResponse(std::unique_ptr<Request> req) override {
    serverResult_.bidiInitialResponse().emplace().request() = *req;
    co_return apache::thrift::
        ResponseAndStreamTransformation<Response, Request, Response>{
            *testCase_.serverInstruction()
                 ->bidiInitialResponse()
                 ->initialResponse(),
            {[this](folly::coro::AsyncGenerator<Request&&> input)
                 -> folly::coro::AsyncGenerator<Response&&> {
              for (auto payload : *testCase_.serverInstruction()
                                       ->bidiInitialResponse()
                                       ->streamPayloads()) {
                co_yield std::move(payload);
              }
              while (auto item = co_await input.next()) {
                serverResult_.bidiInitialResponse()->sinkPayloads()->push_back(
                    std::move(*item));
              }
            }}};
  }

  folly::coro::Task<apache::thrift::StreamTransformation<Request, Response>>
  co_bidiStreamDeclaredException(std::unique_ptr<Request> req) override {
    serverResult_.bidiStreamDeclaredException().emplace().request() = *req;
    co_return apache::thrift::StreamTransformation<Request, Response>{
        [this](folly::coro::AsyncGenerator<Request&&> input)
            -> folly::coro::AsyncGenerator<Response&&> {
          for (auto payload : *testCase_.serverInstruction()
                                   ->bidiStreamDeclaredException()
                                   ->streamPayloads()) {
            co_yield std::move(payload);
          }
          // Read any sink payloads received before throwing
          while (auto item = co_await input.next()) {
            serverResult_.bidiStreamDeclaredException()
                ->sinkPayloads()
                ->push_back(std::move(*item));
          }
          throw *testCase_.serverInstruction()
              ->bidiStreamDeclaredException()
              ->userException();
        }};
  }

  folly::coro::Task<apache::thrift::StreamTransformation<Request, Response>>
  co_bidiStreamUndeclaredException(std::unique_ptr<Request> req) override {
    serverResult_.bidiStreamUndeclaredException().emplace().request() = *req;
    co_return apache::thrift::StreamTransformation<Request, Response>{
        [this](folly::coro::AsyncGenerator<Request&&> input)
            -> folly::coro::AsyncGenerator<Response&&> {
          for (auto payload : *testCase_.serverInstruction()
                                   ->bidiStreamUndeclaredException()
                                   ->streamPayloads()) {
            co_yield std::move(payload);
          }
          // Read any sink payloads received before throwing
          while (auto item = co_await input.next()) {
            serverResult_.bidiStreamUndeclaredException()
                ->sinkPayloads()
                ->push_back(std::move(*item));
          }
          throw std::runtime_error(*testCase_.serverInstruction()
                                        ->bidiStreamUndeclaredException()
                                        ->exceptionMessage());
        }};
  }

  folly::coro::Task<apache::thrift::StreamTransformation<Request, Response>>
  co_bidiSinkDeclaredException(std::unique_ptr<Request> req) override {
    serverResult_.bidiSinkDeclaredException().emplace().request() = *req;
    co_return apache::thrift::StreamTransformation<Request, Response>{
        [this](folly::coro::AsyncGenerator<Request&&> input)
            -> folly::coro::AsyncGenerator<Response&&> {
          for (auto payload : *testCase_.serverInstruction()
                                   ->bidiSinkDeclaredException()
                                   ->streamPayloads()) {
            co_yield std::move(payload);
          }
          try {
            while (auto item = co_await input.next()) {
              serverResult_.bidiSinkDeclaredException()
                  ->sinkPayloads()
                  ->push_back(std::move(*item));
            }
          } catch (const UserException& e) {
            serverResult_.bidiSinkDeclaredException()->userException() = e;
          }
        }};
  }

  folly::coro::Task<apache::thrift::StreamTransformation<Request, Response>>
  co_bidiSinkUndeclaredException(std::unique_ptr<Request> req) override {
    serverResult_.bidiSinkUndeclaredException().emplace().request() = *req;
    co_return apache::thrift::StreamTransformation<Request, Response>{
        [this](folly::coro::AsyncGenerator<Request&&> input)
            -> folly::coro::AsyncGenerator<Response&&> {
          for (auto payload : *testCase_.serverInstruction()
                                   ->bidiSinkUndeclaredException()
                                   ->streamPayloads()) {
            co_yield std::move(payload);
          }
          try {
            while (auto item = co_await input.next()) {
              serverResult_.bidiSinkUndeclaredException()
                  ->sinkPayloads()
                  ->push_back(std::move(*item));
            }
          } catch (const TApplicationException& e) {
            // BiDi transport prefixes exception type to message
            auto msg = e.getMessage();
            auto pos = msg.find(": ");
            serverResult_.bidiSinkUndeclaredException()->exceptionMessage() =
                (pos != std::string::npos) ? msg.substr(pos + 2) : msg;
          } catch (const std::exception& e) {
            serverResult_.bidiSinkUndeclaredException()->exceptionMessage() =
                e.what();
          }
        }};
  }

  folly::coro::Task<apache::thrift::StreamTransformation<Request, Response>>
  co_bidiMethodDeclaredException(std::unique_ptr<Request> req) override {
    serverResult_.bidiMethodDeclaredException().emplace().request() = *req;
    throw *testCase_.serverInstruction()
        ->bidiMethodDeclaredException()
        ->userException();
  }

  folly::coro::Task<apache::thrift::StreamTransformation<Request, Response>>
  co_bidiMethodUndeclaredException(std::unique_ptr<Request> req) override {
    serverResult_.bidiMethodUndeclaredException().emplace().request() = *req;
    throw std::runtime_error(*testCase_.serverInstruction()
                                  ->bidiMethodUndeclaredException()
                                  ->exceptionMessage());
  }

  // =================== Interactions ===================
  class BasicInteraction : public BasicInteractionIf {
   public:
    BasicInteraction(
        const RpcTestCase& testCase,
        ServerTestResult& result,
        int32_t initialSum = 0)
        : testCase_(testCase), serverResult_(result), sum_(initialSum) {}
    void init() override {}
    int32_t add(int32_t toAdd) override {
      sum_ += toAdd;
      return sum_;
    }
    folly::coro::Task<void> co_onTermination() override {
      switch (testCase_.serverInstruction()->getType()) {
        case ServerInstruction::Type::interactionTermination:
          serverResult_.interactionTermination()
              .ensure()
              .terminationReceived() = true;
          break;
        default:; // do nothing
      }
      co_return;
    }

   private:
    const RpcTestCase& testCase_;
    ServerTestResult& serverResult_;
    int32_t sum_;
  };

  std::unique_ptr<BasicInteractionIf> createBasicInteraction() override {
    switch (testCase_.serverInstruction()->getType()) {
      case ServerInstruction::Type::interactionConstructor:
        serverResult_.interactionConstructor().emplace().constructorCalled() =
            true;
        break;
      case ServerInstruction::Type::interactionPersistsState:
        serverResult_.interactionPersistsState().emplace();
        break;
      case ServerInstruction::Type::interactionTermination:
        serverResult_.interactionTermination().emplace();
        break;
      default:
        throw std::runtime_error(
            "BasicInteraction constructor called unexpectedly");
    }
    return std::make_unique<BasicInteraction>(testCase_, serverResult_);
  }

  apache::thrift::TileAndResponse<BasicInteractionIf, void>
  basicInteractionFactoryFunction(int32_t initialSum) override {
    switch (testCase_.serverInstruction()->getType()) {
      case ServerInstruction::Type::interactionFactoryFunction:
        serverResult_.interactionFactoryFunction().emplace().initialSum() =
            initialSum;
        break;
      case ServerInstruction::Type::interactionPersistsState:
        serverResult_.interactionPersistsState().emplace();
        break;
      case ServerInstruction::Type::interactionTermination:
        serverResult_.interactionTermination().emplace();
        break;
      default:
        throw std::runtime_error(
            "BasicInteraction factory function called unexpectedly");
    }
    return {std::make_unique<BasicInteraction>(
        testCase_, serverResult_, initialSum)};
  }

  // What SRProxy does with an error the caller's IDL has no name for: leave
  // it, compact-serialized, in the anyext/anyex response headers
  // (detail::kHeaderAnyexType and detail::kHeaderAnyex) and throw. The Rocket
  // server then sends it as PayloadExceptionMetadata::anyException, inside a
  // SemiAnyStruct.
  [[noreturn]] void throwAnyException(
      const RequestResponseAnyExceptionServerInstruction& instruction) {
    auto* header = getRequestContext()->getHeader();
    header->setHeader("anyext", *instruction.typeUri());
    header->setHeader(
        "anyex",
        folly::base64Encode(
            CompactSerializer::serialize<std::string>(
                *instruction.userException())));
    throw TApplicationException(*instruction.exceptionMessage());
  }

  void recordMetadata(RequestResponseMetadataServerTestResult& result) {
    const auto* header = getRequestContext()->getHeader();
    for (const auto& [key, value] : header->getHeaders()) {
      if (key.starts_with("conformance-")) {
        result.headers()[key] = value;
      }
    }
    const auto& sent =
        *testCase_.clientInstruction()->requestResponseMetadata();
    if (sent.timeoutMs().has_value() || sent.queueTimeoutMs().has_value()) {
      result.clientTimeoutMs() = header->getClientTimeout().count();
      result.queueTimeoutMs() = header->getClientQueueTimeout().count();
    }
    if (sent.priority().has_value()) {
      result.priority() = static_cast<int32_t>(header->getCallPriority());
    }
  }

  folly::coro::Task<std::unique_ptr<Response>> answerLastToFirst(
      const RequestResponseConcurrentServerInstruction& instruction,
      Request req) {
    const auto count = static_cast<size_t>(*instruction.requestCount());
    const auto arrival = concurrentRequests_.withWLock([&](auto& requests) {
      requests.push_back(req);
      return requests.size() - 1;
    });
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::seconds(5);
    while (concurrentRequests_.rlock()->size() < count) {
      if (std::chrono::steady_clock::now() >= deadline) {
        concurrentGaveUp_ = true;
        break;
      }
      co_await folly::coro::sleep(std::chrono::milliseconds(10));
    }
    if (arrival < count) {
      co_await folly::coro::sleep(
          std::chrono::milliseconds(100 * (count - 1 - arrival)));
    }
    auto res = std::make_unique<Response>();
    res->data() = *req.data();
    co_return res;
  }

  // The handler's part in a server error. Whatever it answers is thrown away.
  folly::coro::Task<std::unique_ptr<Response>> answerServerError(
      const ServerError& error) {
    auto res = std::make_unique<Response>();
    res->data() = "the server should not send this";
    switch (error.getType()) {
      case ServerError::Type::taskExpired:
        co_await folly::coro::sleep(
            std::chrono::milliseconds(*error.taskExpired()->handlerTimeMs()));
        break;
      case ServerError::Type::responseTooBig:
        res->data() = std::string(*error.responseTooBig()->responseSize(), 'x');
        break;
      case ServerError::Type::undeclaredException:
        throw std::runtime_error(*error.undeclaredException());
      default:
        break;
    }
    co_return res;
  }

  folly::SemiFuture<folly::Unit> getTestReceived() {
    return getTestReceivedPromise_.getSemiFuture();
  }

  folly::SemiFuture<ClientTestResult> clientResult() {
    return clientResultPromise_.getSemiFuture();
  }

  const ServerTestResult& serverResult() { return serverResult_; }

 private:
  const RpcTestCase& testCase_;
  folly::Promise<folly::Unit> getTestReceivedPromise_;
  folly::Promise<ClientTestResult> clientResultPromise_;
  ServerTestResult serverResult_;
  std::atomic<bool> cancelled_{false};
  folly::Baton<> streamCancelled_;
  folly::Baton<> sinkConsumerDone_;
  bool sinkAborted_{false};
  folly::Synchronized<std::vector<Request>> concurrentRequests_;
  std::atomic<bool> concurrentGaveUp_{false};
};

std::unique_ptr<apache::thrift::HTTP2RoutingHandler> http2RoutingHandler(
    apache::thrift::ThriftServer& server) {
  auto options = std::make_unique<proxygen::HTTPServerOptions>();
  options->threads = static_cast<size_t>(server.getNumIOWorkerThreads());
  options->idleTimeout = server.getIdleTimeout();
  return std::make_unique<apache::thrift::HTTP2RoutingHandler>(
      std::move(options), server.getThriftProcessor(), server);
}

void configureTls(apache::thrift::ThriftServer& server) {
  const char* cert = std::getenv("THRIFT_CONFORMANCE_SERVER_TLS_CERT");
  if (cert == nullptr) {
    return;
  }
  auto ssl = std::make_shared<wangle::SSLContextConfig>();
  ssl->setCertificate(
      cert, getEnvOrThrow("THRIFT_CONFORMANCE_SERVER_TLS_KEY"), "");
  ssl->clientCAFiles = std::vector<std::string>{
      getEnvOrThrow("THRIFT_CONFORMANCE_SERVER_TLS_CLIENT_CA")};
  ssl->sessionContext = "thrift_conformance";
  ssl->setNextProtocols(**apache::thrift::ThriftServer::defaultNextProtocols());
  server.setSSLConfig(std::move(ssl));
  server.setSSLPolicy(apache::thrift::SSLPolicy::REQUIRED);
  server.setAllowPlaintextOnLoopback(false);
}

// What some cases need set on the server before it starts.
void configureServer(
    apache::thrift::ThriftServer& server,
    const ServerInstruction& instruction) {
  if (instruction.bidiAbort()) {
    // sendTestResult holds one CPU thread while it waits on the bidi stream,
    // whose delayed initial response needs another.
    server.setNumCPUWorkerThreads(2);
  }
  const ServerError* serverError = nullptr;
  std::string method;
  if (const auto requestResponse = instruction.requestResponseServerError()) {
    serverError = &*requestResponse->error();
    method = "requestResponseBasic";
  } else if (const auto stream = instruction.streamServerError()) {
    serverError = &*stream->error();
    method = "streamBasic";
  }
  if (serverError != nullptr) {
    const auto& error = *serverError;
    switch (error.getType()) {
      case ServerError::Type::loadshedding:
        // Only the call under test: the client still fetches its test case and
        // reports its result through this server.
        server.addPreprocessFunc(
            "conformance",
            [message = *error.loadshedding(), method](
                const server::PreprocessParams& params) -> PreprocessResult {
              if (params.method == method) {
                return AppOverloadedException(
                    std::string(params.method), std::string(message));
              }
              return {};
            });
        break;
      case ServerError::Type::taskExpired:
        server.setUseClientTimeout(false);
        server.setTaskExpireTime(
            std::chrono::milliseconds(
                *error.taskExpired()->taskExpireTimeMs()));
        break;
      case ServerError::Type::responseTooBig:
        server.setMaxResponseSize(*error.responseTooBig()->maxResponseSize());
        break;
      default:
        break;
    }
  }
}

std::vector<std::string> clientArgs() {
  std::vector<std::string> args;
  if (const char* joined = std::getenv("THRIFT_CONFORMANCE_CLIENT_ARGS")) {
    folly::split(' ', joined, args, /* ignoreEmpty */ true);
  }
  return args;
}

void createClient(
    std::string_view serviceName, std::string ipAddress, std::string port) {
  auto client = create_rpc_conformance_setup_service_client_(serviceName);
  folly::coro::blockingWait(
      client->co_createRPCConformanceServiceClient(ipAddress, port));
}

class RPCClientConformanceTest : public testing::Test {
 public:
  RPCClientConformanceTest(
      std::string_view clientCmd,
      const TestSuite& suite,
      const conformance::Test& test,
      const TestCase& testCase,
      bool conforming,
      bool connectViaServer)
      : suite_(suite),
        test_(test),
        testCase_(testCase),
        conforming_(conforming),
        handler_(
            std::make_shared<ConformanceVerificationServer>(*testCase_.rpc())),
        server_(
            handler_,
            connectViaServer ? get_server_ip_addr_() : "::1",
            0,
            apache::thrift::ScopedServerInterfaceThread::ServerConfigCb(
                [&](apache::thrift::ThriftServer& server) {
                  if (connectViaServer) {
                    std::ignore = update_server_props_(server);
                  }
                  // Disable queue timeout for conformance tests. Test clients
                  // (especially Erlang/BEAM VM) may be slow to start and should
                  // never be rejected by load shedding in a test environment.
                  server.setQueueTimeout(std::chrono::milliseconds(0));
                  server.addRoutingHandler(http2RoutingHandler(server));
                  configureTls(server);
                  configureServer(
                      server, *testCase_.rpc()->serverInstruction());
                })),
        connectViaServer_(connectViaServer) {
    try {
      auto port = folly::to<std::string>(server_.getPort());
      if (testCase_.rpc()->serverInstruction()->streamCreditTimeout()) {
        server_.getThriftServer().setStreamExpireTime(
            std::chrono::milliseconds{*testCase_.rpc()
                                           ->serverInstruction()
                                           ->streamCreditTimeout()
                                           ->streamExpireTime()});
      }
      if (connectViaServer_) {
        createClient(clientCmd, server_.getAddress().getAddressStr(), port);
      } else {
        auto cmd = std::string(clientCmd);
        auto args = cmd.ends_with(".jar") ? javaLaunchArgs(cmd)
                                          : std::vector<std::string>{cmd};
        args.emplace_back("--port");
        args.push_back(port);
        for (auto& arg : clientArgs()) {
          args.push_back(std::move(arg));
        }
        clientProcess_ = launch_client_process_(args);
      }
    } catch (const std::exception& e) {
      verifyConformanceResult(
          testing::AssertionFailure() << "Unexpected Error " << e.what());
    }
  }

 protected:
  void TestBody() override { verifyConformanceResult(runTest()); }

  void TearDown() override {
    if (!connectViaServer_ && clientProcess_.returnCode().running()) {
      clientProcess_.sendSignal(SIGINT);
      clientProcess_.waitOrTerminateOrKill(kClientTimeout, kClientTimeout);
    }
  }

 private:
  void verifyConformanceResult(testing::AssertionResult conforming) {
    if (conforming_) {
      EXPECT_TRUE(conforming) << "For more detail see:"
                              << std::endl
                              // Most specific to least specific.
                              << genTagLinks(testCase_) << genTagLinks(test_)
                              << genTagLinks(suite_);
    } else {
      EXPECT_FALSE(conforming)
          << "If intentional, please remove the associated entry from:"
          << std::endl
          // TODO: create separate nonconforming.txt file for client rpc tests
          << "    thrift/conformance/data/nonconforming.txt" << std::endl;
    }
  }
  testing::AssertionResult runTest() {
    try { // Wait for client to fetch test case
      bool getTestReceived = handler_->getTestReceived().wait(kClientTimeout);

      // End test if client was unable to fetch test case
      if (!getTestReceived) {
        return testing::AssertionFailure()
            << "client failed to fetch test case";
      }

      // Wait for result from client, polling subprocess health
      auto resultFuture = handler_->clientResult();
      auto deadline = std::chrono::steady_clock::now() + kClientTimeout;
      while (!resultFuture.isReady() &&
             std::chrono::steady_clock::now() < deadline) {
        if (!connectViaServer_) {
          auto rc = clientProcess_.poll();
          if (!rc.running()) {
            return testing::AssertionFailure()
                << "client process exited before sending test result";
          }
        }
        /* sleep override */ std::this_thread::sleep_for(
            std::chrono::milliseconds(100));
      }

      if (!resultFuture.isReady()) {
        return testing::AssertionFailure() << "Timed out waiting for client";
      }

      folly::Try<ClientTestResult> actualClientResult =
          std::move(resultFuture).getTry();

      // End test if result was not received
      if (actualClientResult.hasException()) {
        return testing::AssertionFailure() << actualClientResult.exception();
      }

      auto& expectedClientResult = *testCase_.rpc()->clientTestResult();
      if (!equal(*actualClientResult, expectedClientResult)) {
        return testing::AssertionFailure()
            << "\nExpected client result: " << jsonify(expectedClientResult)
            << "\nActual client result: " << jsonify(*actualClientResult);
      }

      auto& actualServerResult = handler_->serverResult();
      auto& expectedServerResult = *testCase_.rpc()->serverTestResult();
      if (!equal(actualServerResult, expectedServerResult)) {
        return testing::AssertionFailure()
            << "\nExpected server result: " << jsonify(expectedServerResult)
            << "\nActual server result: " << jsonify(actualServerResult);
      }

      return testing::AssertionSuccess();
    } catch (const std::exception& e) {
      return testing::AssertionFailure() << "Unexpected error " << e.what();
    }
  }

  const TestSuite& suite_;
  const conformance::Test& test_;
  const TestCase& testCase_;
  bool conforming_;
  std::shared_ptr<ConformanceVerificationServer> handler_;
  apache::thrift::ScopedServerInterfaceThread server_;
  folly::Subprocess clientProcess_;
  bool connectViaServer_ = false;
};

void registerTests(
    std::string_view category,
    const TestSuite& suite,
    const std::set<std::string>& nonconforming,
    std::pair<std::string_view, bool> clientCmd,
    const char* file,
    int line) {
  for (const auto& test : *suite.tests()) {
    for (const auto& testCase : *test.testCases()) {
      std::string suiteName =
          fmt::format("{}/{}/{}", category, *suite.name(), *testCase.name());
      std::string fullName = fmt::format("{}.{}", suiteName, *test.name());
      bool conforming = nonconforming.find(fullName) == nonconforming.end();
      registerTest(
          suiteName.c_str(),
          test.name()->c_str(),
          nullptr,
          conforming ? nullptr : "nonconforming",
          file,
          line,
          [clientCmd, &suite, &test, &testCase, conforming]() {
            return new RPCClientConformanceTest(
                clientCmd.first,
                suite,
                test,
                testCase,
                conforming,
                clientCmd.second);
          });
    }
  }
}

} // namespace apache::thrift::conformance
