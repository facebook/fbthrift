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

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <folly/Conv.h>
#include <folly/Overload.h>

#include <thrift/lib/cpp2/async/AsyncProcessorHelper.h>
#include <thrift/lib/cpp2/async/MultiplexAsyncProcessor.h>
#include <thrift/lib/cpp2/protocol/DebugProtocol.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <thrift/lib/cpp2/test/gen-cpp2/Conflicts.h>
#include <thrift/lib/cpp2/test/gen-cpp2/ConflictsInteraction1.h>
#include <thrift/lib/cpp2/test/gen-cpp2/First.h>
#include <thrift/lib/cpp2/test/gen-cpp2/Interaction1.h>
#include <thrift/lib/cpp2/test/gen-cpp2/Interaction2.h>
#include <thrift/lib/cpp2/test/gen-cpp2/Second.h>
#include <thrift/lib/cpp2/test/gen-cpp2/SomeService.h>
#include <thrift/lib/cpp2/test/gen-cpp2/Third.h>
#include <thrift/lib/thrift/gen-cpp2/metadata_types_custom_protocol.h>

namespace apache::thrift::test {

using namespace ::testing;

using MethodMetadata = AsyncProcessorFactory::MethodMetadata;
using MethodMetadataMap = AsyncProcessorFactory::MethodMetadataMap;
using MetadataNotImplemented = AsyncProcessorFactory::MetadataNotImplemented;
using WildcardMethodMetadata = AsyncProcessorFactory::WildcardMethodMetadata;
using WildcardMethodMetadataMap =
    AsyncProcessorFactory::WildcardMethodMetadataMap;
using CreateMethodMetadataResult =
    AsyncProcessorFactory::CreateMethodMetadataResult;

namespace {
class First : public FirstSvIf {
  int one() override { return 1; }
  int two() override { return 2; }
};

class Second : public SecondSvIf {
  int three() override { return 3; }
  int four() override { return 4; }
};

class Third : public ThirdSvIf {
  int five() override { return 5; }
  int six() override { return 6; }
};

class Conflicts : public ConflictsSvIf {
  int four() override { return 444; }
  int five() override { return 555; }
};
} // namespace

TEST(MultiplexAsyncProcessorTest, getServiceHandlers) {
  std::vector<std::shared_ptr<AsyncProcessorFactory>> services = {
      std::make_shared<First>(),
      std::make_shared<Second>(),
      std::make_shared<Third>(),
      std::make_shared<Conflicts>(),
  };
  auto processorFactory =
      std::make_shared<MultiplexAsyncProcessorFactory>(std::move(services));
  // Generated service handlers are one per service
  EXPECT_EQ(processorFactory->getServiceHandlers().size(), 4);
}

TEST(MultiplexAsyncProcessorTest, getServiceMetadata) {
  auto getMetadataFromService = [](AsyncProcessorFactory& service) {
    metadata::ThriftServiceMetadataResponse response;
    service.getProcessor()->getServiceMetadata(response);
    return response;
  };
  std::vector<std::shared_ptr<AsyncProcessorFactory>> servicesToMultiplex = {
      std::make_shared<First>(),
      std::make_shared<Second>(),
      std::make_shared<SomeServiceSvIf>(),
      std::make_shared<Conflicts>(),
      std::make_shared<Third>(),
  };
  auto processorFactory = std::make_shared<MultiplexAsyncProcessorFactory>(
      std::move(servicesToMultiplex));
  auto response = getMetadataFromService(*processorFactory);

  LOG(INFO) << "ServiceMetadata: " << debugString(response);

  EXPECT_EQ(
      *response.context_ref()->service_info_ref()->name_ref(),
      "MultiplexAsyncProcessor.First");

  auto& services = *response.services_ref();
  EXPECT_EQ(services.size(), 6);
  EXPECT_EQ(*services[0].service_name_ref(), "MultiplexAsyncProcessor.First");
  EXPECT_EQ(*services[1].service_name_ref(), "MultiplexAsyncProcessor.Second");
  EXPECT_EQ(
      *services[2].service_name_ref(), "MultiplexAsyncProcessor.SomeService");
  // Base service of SomeService
  EXPECT_EQ(*services[3].service_name_ref(), "MultiplexAsyncProcessor.Third");
  EXPECT_EQ(
      *services[4].service_name_ref(), "MultiplexAsyncProcessor.Conflicts");
  EXPECT_EQ(*services[5].service_name_ref(), "MultiplexAsyncProcessor.Third");

  const auto& metadata = *response.metadata_ref();
  EXPECT_EQ(metadata.structs_ref()->size(), 1);
  EXPECT_EQ(
      metadata.structs_ref()->begin()->first,
      "MultiplexAsyncProcessor.SomeStruct");
  // All composed services are referred to
  EXPECT_EQ(metadata.services_ref()->size(), 5);
}

namespace {
class MultiplexAsyncProcessorServerTest : public Test {
 public:
  std::shared_ptr<AsyncProcessorFactory> multiplex(
      std::vector<std::shared_ptr<AsyncProcessorFactory>> services) {
    return std::make_shared<MultiplexAsyncProcessorFactory>(
        std::move(services));
  }

  std::unique_ptr<ScopedServerInterfaceThread> runMultiplexedServices(
      std::vector<std::shared_ptr<AsyncProcessorFactory>> services) {
    return std::make_unique<ScopedServerInterfaceThread>(
        multiplex(std::move(services)));
  }
};
} // namespace

TEST_F(MultiplexAsyncProcessorServerTest, Basic) {
  auto runner = runMultiplexedServices(
      {std::make_shared<First>(), std::make_shared<Second>()});

  auto client1 = runner->newClient<FirstAsyncClient>();
  auto client2 = runner->newClient<SecondAsyncClient>();

  EXPECT_EQ(client1->semifuture_one().get(), 1);
  EXPECT_EQ(client1->semifuture_two().get(), 2);
  EXPECT_EQ(client2->semifuture_three().get(), 3);
  EXPECT_EQ(client2->semifuture_four().get(), 4);
}

TEST_F(MultiplexAsyncProcessorServerTest, ConflictPrecedence) {
  auto runner = runMultiplexedServices(
      {std::make_shared<Second>(),
       std::make_shared<Conflicts>(),
       std::make_shared<Third>()});

  auto client2 = runner->newClient<SecondAsyncClient>();
  auto client3 = runner->newClient<ThirdAsyncClient>();

  EXPECT_EQ(client2->semifuture_three().get(), 3);
  // Second takes precedence
  EXPECT_EQ(client2->semifuture_four().get(), 4);
  // Conflicts takes precedence
  EXPECT_EQ(client3->semifuture_five().get(), 555);
  EXPECT_EQ(client3->semifuture_six().get(), 6);
}

TEST_F(MultiplexAsyncProcessorServerTest, Nested) {
  auto runner = runMultiplexedServices(
      {multiplex({std::make_shared<Second>(), std::make_shared<Conflicts>()}),
       std::make_shared<Third>()});

  auto client2 = runner->newClient<SecondAsyncClient>();
  auto client3 = runner->newClient<ThirdAsyncClient>();

  EXPECT_EQ(client2->semifuture_three().get(), 3);
  // Second takes precedence
  EXPECT_EQ(client2->semifuture_four().get(), 4);
  // Conflicts takes precedence
  EXPECT_EQ(client3->semifuture_five().get(), 555);
  EXPECT_EQ(client3->semifuture_six().get(), 6);
}

namespace {

class ContextData : public folly::RequestData {
 public:
  static const folly::RequestToken& getRequestToken() {
    static folly::RequestToken token(
        "MultiplexAsyncProcessorTest - ContextData");
    return token;
  }
  explicit ContextData(int data) : data_(data) {}
  int data() const { return data_; }

  bool hasCallback() override { return false; }

  static int readFromCurrent() {
    return readFrom(*folly::RequestContext::get());
  }

 private:
  static int readFrom(const folly::RequestContext& ctx) {
    auto ctxData =
        dynamic_cast<const ContextData*>(ctx.getContextData(getRequestToken()));
    CHECK(ctxData != nullptr);
    return ctxData->data();
  }

  int data_;
};

struct FromCurrentContextData {};
/**
 * AsyncProcessorFactory where WildcardMethodMetadata always causes an internal
 * error with the provided message (or optionally read it from
 * folly::RequestContext).
 */
template <typename TProcessorFactory>
class WildcardThrowsInternalError : public TProcessorFactory {
 public:
  explicit WildcardThrowsInternalError(FromCurrentContextData)
      : message_{FromCurrentContextData{}} {}
  explicit WildcardThrowsInternalError(std::string message)
      : message_{std::move(message)} {}

 private:
  using MessageVariant = std::variant<FromCurrentContextData, std::string>;

  CreateMethodMetadataResult createMethodMetadata() override {
    auto metadataResult = TProcessorFactory::createMethodMetadata();
    return folly::variant_match(
        metadataResult,
        [](MetadataNotImplemented) -> WildcardMethodMetadataMap {
          return WildcardMethodMetadataMap{};
        },
        [](MethodMetadataMap& knownMethods) -> WildcardMethodMetadataMap {
          return WildcardMethodMetadataMap{std::move(knownMethods)};
        },
        [](WildcardMethodMetadataMap& map) -> WildcardMethodMetadataMap {
          return std::move(map);
        });
  }

  std::unique_ptr<AsyncProcessor> getProcessor() override {
    class Processor : public AsyncProcessor {
     public:
      void processSerializedRequest(
          ResponseChannelRequest::UniquePtr,
          SerializedRequest&&,
          protocol::PROTOCOL_TYPES,
          Cpp2RequestContext*,
          folly::EventBase*,
          concurrency::ThreadManager*) override {
        ADD_FAILURE() << "Should never be called";
      }

      void processSerializedCompressedRequestWithMetadata(
          ResponseChannelRequest::UniquePtr req,
          SerializedCompressedRequest&& serializedRequest,
          const MethodMetadata& untypedMethodMetadata,
          protocol::PROTOCOL_TYPES protocolType,
          Cpp2RequestContext* context,
          folly::EventBase* eb,
          concurrency::ThreadManager* tm) override {
        if (AsyncProcessorHelper::isWildcardMethodMetadata(
                untypedMethodMetadata)) {
          std::string message = folly::variant_match(
              message_,
              [](const std::string& m) { return m; },
              [](FromCurrentContextData) {
                return folly::to<std::string>(ContextData::readFromCurrent());
              });
          req->sendErrorWrapped(
              folly::make_exception_wrapper<TApplicationException>(
                  TApplicationException::INTERNAL_ERROR, std::move(message)),
              "" /* errorCode */);
          return;
        }
        inner_->processSerializedCompressedRequestWithMetadata(
            std::move(req),
            std::move(serializedRequest),
            untypedMethodMetadata,
            protocolType,
            context,
            eb,
            tm);
      }

      void terminateInteraction(
          int64_t id,
          Cpp2ConnContext& ctx,
          folly::EventBase& eb) noexcept override {
        inner_->terminateInteraction(id, ctx, eb);
      }

      void destroyAllInteractions(
          Cpp2ConnContext& ctx, folly::EventBase& eb) noexcept override {
        inner_->destroyAllInteractions(ctx, eb);
      }

      explicit Processor(
          std::unique_ptr<AsyncProcessor>&& inner,
          const MessageVariant& message)
          : inner_(std::move(inner)), message_(message) {}

     private:
      std::unique_ptr<AsyncProcessor> inner_;
      const MessageVariant& message_;
    };

    return std::make_unique<Processor>(
        TProcessorFactory::getProcessor(), message_);
  }

  MessageVariant message_;
};

template <typename TProcessorFactory>
class WithUnimplementedMethodMetadata : public TProcessorFactory {
  CreateMethodMetadataResult createMethodMetadata() override { return {}; }
};
} // namespace

TEST_F(MultiplexAsyncProcessorServerTest, BasicWildcard) {
  auto runner = runMultiplexedServices(
      {std::make_shared<First>(),
       std::make_shared<WildcardThrowsInternalError<Second>>("BasicWildcard")});

  auto client1 = runner->newClient<FirstAsyncClient>();
  auto client2 = runner->newClient<SecondAsyncClient>();
  auto client3 = runner->newClient<ThirdAsyncClient>();

  EXPECT_EQ(client1->semifuture_one().get(), 1);
  EXPECT_EQ(client1->semifuture_two().get(), 2);
  EXPECT_EQ(client2->semifuture_three().get(), 3);
  EXPECT_EQ(client2->semifuture_four().get(), 4);

  EXPECT_THAT(
      [&] { client3->semifuture_five().get(); },
      ThrowsMessage<TApplicationException>("BasicWildcard"));
}

TEST_F(MultiplexAsyncProcessorServerTest, WildcardSwallows) {
  auto runner = runMultiplexedServices(
      {std::make_shared<WildcardThrowsInternalError<First>>("WildcardSwallows"),
       std::make_shared<Second>(),
       std::make_shared<WildcardThrowsInternalError<Third>>("NeverReached")});

  auto client1 = runner->newClient<FirstAsyncClient>();
  auto client2 = runner->newClient<SecondAsyncClient>();

  EXPECT_EQ(client1->semifuture_one().get(), 1);
  EXPECT_EQ(client1->semifuture_two().get(), 2);

  EXPECT_THAT(
      [&] { client2->semifuture_three().get(); },
      ThrowsMessage<TApplicationException>("WildcardSwallows"));
}

TEST_F(MultiplexAsyncProcessorServerTest, WildcardConflicts) {
  auto runner = runMultiplexedServices(
      {std::make_shared<Second>(),
       std::make_shared<WildcardThrowsInternalError<Conflicts>>(
           "WildcardConflicts")});

  auto client2 = runner->newClient<SecondAsyncClient>();
  auto client3 = runner->newClient<ThirdAsyncClient>();

  // Known methods takes precedence
  EXPECT_EQ(client2->semifuture_three().get(), 3);
  EXPECT_EQ(client2->semifuture_four().get(), 4);
  EXPECT_EQ(client3->semifuture_five().get(), 555);
  EXPECT_THAT(
      [&] { client3->semifuture_six().get(); },
      ThrowsMessage<TApplicationException>("WildcardConflicts"));
}

TEST_F(MultiplexAsyncProcessorServerTest, UnimplementedMetadataActsAsWildcard) {
  auto runner = runMultiplexedServices(
      {std::make_shared<WithUnimplementedMethodMetadata<First>>(),
       std::make_shared<Second>()});

  auto client1 = runner->newClient<FirstAsyncClient>();
  auto client2 = runner->newClient<SecondAsyncClient>();

  EXPECT_EQ(client1->semifuture_one().get(), 1);
  EXPECT_EQ(client1->semifuture_two().get(), 2);

  EXPECT_THAT(
      [&] { client2->semifuture_three().get(); },
      ThrowsMessage<TApplicationException>("Method name three not found"));
}

namespace {

class RctxFirst : public FirstSvIf {
  int one() override { return ContextData::readFromCurrent(); }
  int two() override { return ContextData::readFromCurrent(); }
};

class RctxSecond : public SecondSvIf {
  int three() override { return ContextData::readFromCurrent(); }
  int four() override { return ContextData::readFromCurrent(); }
};

class RctxThird : public ThirdSvIf {
  int five() override { return ContextData::readFromCurrent(); }
  int six() override { return ContextData::readFromCurrent(); }
};

class RctxConflicts : public ConflictsSvIf {
  int four() override { return ContextData::readFromCurrent(); }
  int five() override { return ContextData::readFromCurrent(); }
};

template <typename TProcessorFactory, int kData>
class WithRequestContextData : public TProcessorFactory {
 public:
  using TProcessorFactory::TProcessorFactory;

  std::shared_ptr<folly::RequestContext> getBaseContextForRequest(
      const MethodMetadata&) override {
    auto ctx = std::make_shared<folly::RequestContext>();
    ctx->setContextData(
        ContextData::getRequestToken(), std::make_unique<ContextData>(kData));
    return ctx;
  }
};

} // namespace

TEST_F(MultiplexAsyncProcessorServerTest, RequestContext) {
  auto runner = runMultiplexedServices(
      {std::make_shared<WithRequestContextData<RctxFirst, 1>>(),
       std::make_shared<WithRequestContextData<RctxSecond, 2>>()});

  auto client1 = runner->newClient<FirstAsyncClient>();
  auto client2 = runner->newClient<SecondAsyncClient>();

  EXPECT_EQ(client1->semifuture_one().get(), 1);
  EXPECT_EQ(client1->semifuture_two().get(), 1);
  EXPECT_EQ(client2->semifuture_three().get(), 2);
  EXPECT_EQ(client2->semifuture_four().get(), 2);
}

TEST_F(MultiplexAsyncProcessorServerTest, RequestContextWildcard) {
  auto runner = runMultiplexedServices(
      {std::make_shared<
           WithRequestContextData<WildcardThrowsInternalError<RctxFirst>, 1>>(
           FromCurrentContextData{}),
       std::make_shared<WithRequestContextData<RctxSecond, 2>>()});

  auto client1 = runner->newClient<FirstAsyncClient>();
  auto client2 = runner->newClient<SecondAsyncClient>();

  EXPECT_EQ(client1->semifuture_one().get(), 1);
  EXPECT_EQ(client1->semifuture_two().get(), 1);
  EXPECT_THAT(
      [&] { client2->semifuture_three().get(); },
      ThrowsMessage<TApplicationException>("1"));
}

TEST_F(MultiplexAsyncProcessorServerTest, RequestContextConflictPrecedence) {
  auto runner = runMultiplexedServices(
      {std::make_shared<WithRequestContextData<RctxSecond, 2>>(),
       std::make_shared<WithRequestContextData<RctxConflicts, -1>>(),
       std::make_shared<WithRequestContextData<RctxThird, 3>>()});

  auto client2 = runner->newClient<SecondAsyncClient>();
  auto client3 = runner->newClient<ThirdAsyncClient>();

  EXPECT_EQ(client2->semifuture_three().get(), 2);
  // Second takes precedence
  EXPECT_EQ(client2->semifuture_four().get(), 2);
  // Conflicts takes precedence
  EXPECT_EQ(client3->semifuture_five().get(), -1);
  EXPECT_EQ(client3->semifuture_six().get(), 3);
}

namespace {
RequestChannel::Ptr makeRocketChannel(folly::AsyncSocket::UniquePtr socket) {
  return RocketClientChannel::newChannel(std::move(socket));
}
} // namespace

TEST_F(MultiplexAsyncProcessorServerTest, Interaction) {
  using Counter = std::atomic<int>;

  class TerminateInteractionTrackingProcessor : public AsyncProcessor {
   public:
    void processSerializedRequest(
        ResponseChannelRequest::UniquePtr req,
        SerializedRequest&& serializedRequest,
        protocol::PROTOCOL_TYPES protocolType,
        Cpp2RequestContext* context,
        folly::EventBase* eb,
        concurrency::ThreadManager* tm) override {
      delegate_->processSerializedRequest(
          std::move(req),
          std::move(serializedRequest),
          protocolType,
          context,
          eb,
          tm);
    }

    void processSerializedCompressedRequest(
        ResponseChannelRequest::UniquePtr req,
        SerializedCompressedRequest&& serializedRequest,
        protocol::PROTOCOL_TYPES protocolType,
        Cpp2RequestContext* context,
        folly::EventBase* eb,
        concurrency::ThreadManager* tm) override {
      delegate_->processSerializedCompressedRequest(
          std::move(req),
          std::move(serializedRequest),
          protocolType,
          context,
          eb,
          tm);
    }

    void processSerializedCompressedRequestWithMetadata(
        ResponseChannelRequest::UniquePtr req,
        SerializedCompressedRequest&& serializedRequest,
        const MethodMetadata& methodMetadata,
        protocol::PROTOCOL_TYPES protocolType,
        Cpp2RequestContext* context,
        folly::EventBase* eb,
        concurrency::ThreadManager* tm) override {
      delegate_->processSerializedCompressedRequestWithMetadata(
          std::move(req),
          std::move(serializedRequest),
          methodMetadata,
          protocolType,
          context,
          eb,
          tm);
    }

    virtual void terminateInteraction(
        int64_t id,
        Cpp2ConnContext& ctx,
        folly::EventBase& eb) noexcept override {
      ++numCalls_;
      delegate_->terminateInteraction(id, ctx, eb);
    }

    explicit TerminateInteractionTrackingProcessor(
        std::unique_ptr<AsyncProcessor>&& delegate, Counter& numCalls)
        : delegate_(std::move(delegate)), numCalls_(numCalls) {}

   private:
    std::unique_ptr<AsyncProcessor> delegate_;
    Counter& numCalls_;
  };

  class Interaction1 : public Interaction1SvIf {
   public:
    std::unique_ptr<Thing1If> createThing1() override {
      class Thing1 : public Thing1If {
       public:
        void foo() override { ++numCalls_; }

        explicit Thing1(Counter& numCalls, folly::Baton<>& destroyed)
            : numCalls_(numCalls), destroyed_(destroyed) {}
        ~Thing1() override { destroyed_.post(); }

       private:
        Counter& numCalls_;
        folly::Baton<>& destroyed_;
      };
      return std::make_unique<Thing1>(numCalls, destroyed);
    }

    std::unique_ptr<AsyncProcessor> getProcessor() override {
      return std::make_unique<TerminateInteractionTrackingProcessor>(
          Interaction1SvIf::getProcessor(), numTerminateInteractionCalls);
    }

    Counter numCalls{0};
    Counter numTerminateInteractionCalls{0};
    folly::Baton<> destroyed;
  };

  class Interaction2 : public Interaction2SvIf {
    std::unique_ptr<Thing2If> createThing2() override {
      class Thing2 : public Thing2If {
       public:
        void bar() override { ++numCalls_; }

        explicit Thing2(Counter& numCalls) : numCalls_(numCalls) {}

       private:
        Counter& numCalls_;
      };
      return std::make_unique<Thing2>(numCalls);
    }

   public:
    Counter numCalls{0};
  };

  auto interaction1 = std::make_shared<Interaction1>();
  auto interaction2 = std::make_shared<Interaction2>();
  auto runner = runMultiplexedServices({interaction1, interaction2});

  auto client1 = runner->newClient<Interaction1AsyncClient>(
      nullptr /* callbackExecutor */, makeRocketChannel);
  auto client2 = runner->newClient<Interaction2AsyncClient>(
      nullptr /* callbackExecutor */, makeRocketChannel);

  std::optional<Interaction1AsyncClient::Thing1> thing1 =
      client1->createThing1();
  thing1->semifuture_foo().get();

  std::optional<Interaction2AsyncClient::Thing2> thing2 =
      client2->createThing2();
  thing2->semifuture_bar().get();

  EXPECT_EQ(interaction1->numCalls.load(), 1);
  EXPECT_EQ(interaction2->numCalls.load(), 1);

  // Make sure interaction gets destroyed
  thing1.reset();
  interaction1->destroyed.wait();
  EXPECT_EQ(interaction1->numTerminateInteractionCalls.load(), 1);

  // Other interactions should not be destroyed
  thing2->semifuture_bar().get();
  EXPECT_EQ(interaction2->numCalls.load(), 2);
}

TEST_F(MultiplexAsyncProcessorServerTest, InteractionConflict) {
  class Interaction1 : public Interaction1SvIf {
   public:
    std::unique_ptr<Thing1If> createThing1() override {
      class Thing1 : public Thing1If {
       public:
        void foo() override {}
      };
      return std::make_unique<Thing1>();
    }
  };

  class ConflictsInteraction1
      : public apache::thrift::test2::ConflictsInteraction1SvIf {
   public:
    std::unique_ptr<Thing1If> createThing1() override {
      class Thing1 : public Thing1If {
       public:
        void foo() override { ADD_FAILURE() << "Should never be called"; }
        void bar() override { ADD_FAILURE() << "Should never be called"; }
      };
      return std::make_unique<Thing1>();
    }
  };

  auto runner = runMultiplexedServices(
      {std::make_shared<Interaction1>(),
       std::make_shared<ConflictsInteraction1>(),
       std::make_shared<WildcardThrowsInternalError<Second>>(
           "ConflictsInteraction1")});

  auto client1 = runner->newClient<Interaction1AsyncClient>(
      nullptr /* callbackExecutor */, makeRocketChannel);
  auto client2 =
      runner
          ->newClient<apache::thrift::test2::ConflictsInteraction1AsyncClient>(
              nullptr /* callbackExecutor */, makeRocketChannel);

  auto thing = client1->createThing1();
  thing.semifuture_foo().get();

  auto thing2 = client2->createThing1();
  // Thing1.bar from ConflictsInteraction1 should not be in MethodMetadataMap
  // because Interaction1 already added Thing1.foo.
  EXPECT_THAT(
      [&] { thing2.semifuture_bar().get(); },
      ThrowsMessage<TApplicationException>("ConflictsInteraction1"));
}

} // namespace apache::thrift::test
