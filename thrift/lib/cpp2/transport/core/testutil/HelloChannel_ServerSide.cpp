/*
 * Copyright 2017-present Facebook, Inc.
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
#include <thrift/lib/cpp2/transport/core/testutil/TestServiceExtension.h>

// TODO: Do we really want this dependency?
#include <thrift/lib/cpp2/transport/core/ThriftRequest.h>

#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

namespace testutil {
namespace testservice {

using TestService_helloChannel_pargs = apache::thrift::ThriftPresult<false>;
using TestService_helloChannel_presult = apache::thrift::ThriftPresult<true>;

std::unique_ptr<apache::thrift::AsyncProcessor>
TestServiceExtensionSvIf::getProcessor() {
  return std::make_unique<TestServiceAsyncProcessorExtension>(this);
}

template <typename ProtocolIn_, typename ProtocolOut_>
void TestServiceAsyncProcessorExtension::_processInThread_helloChannel(
    std::unique_ptr<apache::thrift::ResponseChannel::Request> req,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<ProtocolIn_> iprot,
    apache::thrift::Cpp2RequestContext* ctx,
    folly::EventBase* eb,
    apache::thrift::concurrency::ThreadManager* tm) {
  auto pri =
      iface_->getRequestPriority(ctx, apache::thrift::concurrency::NORMAL);
  processInThread<ProtocolIn_, ProtocolOut_>(
      std::move(req),
      std::move(buf),
      std::move(iprot),
      ctx,
      eb,
      tm,
      pri,
      false,
      &TestServiceAsyncProcessorExtension::
          process_helloChannel<ProtocolIn_, ProtocolOut_>,
      this);
}

template <typename ProtocolIn_, typename ProtocolOut_>
void TestServiceAsyncProcessorExtension::process_helloChannel(
    std::unique_ptr<apache::thrift::ResponseChannel::Request> req,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<ProtocolIn_> iprot,
    apache::thrift::Cpp2RequestContext* ctx,
    folly::EventBase* eb,
    apache::thrift::concurrency::ThreadManager* tm) {
  // make sure getConnectionContext is null
  // so async calls don't accidentally use it
  iface_->setConnectionContext(nullptr);
  TestService_helloChannel_pargs args;
  std::unique_ptr<apache::thrift::ContextStack> c(this->getContextStack(
      this->getServiceName(), "TestService.helloChannel", ctx));
  try {
    deserializeRequest(args, buf.get(), iprot.get(), c.get());
  } catch (const std::exception& ex) {
    ProtocolOut_ prot;
    if (req) {
      LOG(ERROR) << ex.what() << " in function helloChannel";
      apache::thrift::TApplicationException x(
          apache::thrift::TApplicationException::TApplicationExceptionType::
              PROTOCOL_ERROR,
          ex.what());
      folly::IOBufQueue queue = serializeException(
          "helloChannel", &prot, ctx->getProtoSeqId(), nullptr, x);
      queue.append(apache::thrift::transport::THeader::transform(
          queue.move(),
          ctx->getHeader()->getWriteTransforms(),
          ctx->getHeader()->getMinCompressBytes()));
      eb->runInEventBaseThread(
          [queue = std::move(queue), req = std::move(req)]() mutable {
            req->sendReply(queue.move());
          });
      return;
    } else {
      LOG(ERROR) << ex.what() << " in oneway function helloChannel";
    }
  }

  auto callback = std::make_unique<apache::thrift::HandlerCallback<void>>(
      std::move(req),
      std::move(c),
      return_helloChannel<ProtocolIn_, ProtocolOut_>,
      throw_wrapped_helloChannel<ProtocolIn_, ProtocolOut_>,
      ctx->getProtoSeqId(),
      eb,
      tm,
      ctx);
  if (!callback->isRequestActive()) {
    callback.release()->deleteInThread();
    return;
  }
  ctx->setStartedProcessing();
  iface_->async_tm_helloChannel(std::move(callback));
}

template <class ProtocolIn_, class ProtocolOut_>
folly::IOBufQueue TestServiceAsyncProcessorExtension::return_helloChannel(
    int32_t protoSeqId,
    apache::thrift::ContextStack* ctx) {
  ProtocolOut_ prot;
  TestService_helloChannel_presult result;
  return serializeResponse("helloChannel", &prot, protoSeqId, ctx, result);
}

template <class ProtocolIn_, class ProtocolOut_>
void TestServiceAsyncProcessorExtension::throw_wrapped_helloChannel(
    std::unique_ptr<apache::thrift::ResponseChannel::Request> req,
    int32_t protoSeqId,
    apache::thrift::ContextStack* ctx,
    folly::exception_wrapper ew,
    apache::thrift::Cpp2RequestContext* reqCtx) {
  if (!ew) {
    return;
  }
  ProtocolOut_ prot;
  {
    if (req) {
      LOG(ERROR) << ew.what().toStdString() << " in function helloChannel";
      apache::thrift::TApplicationException x(ew.what().toStdString());
      ctx->userExceptionWrapped(false, ew);
      ctx->handlerErrorWrapped(ew);
      folly::IOBufQueue queue =
          serializeException("helloChannel", &prot, protoSeqId, ctx, x);
      queue.append(apache::thrift::transport::THeader::transform(
          queue.move(),
          reqCtx->getHeader()->getWriteTransforms(),
          reqCtx->getHeader()->getMinCompressBytes()));
      req->sendReply(queue.move());
      return;
    } else {
      LOG(ERROR) << ew.what().toStdString()
                 << " in oneway function helloChannel";
    }
  }
}

yarpl::Reference<yarpl::flowable::Flowable<std::string>>
TestServiceExtensionSvIf::helloChannel(
    yarpl::Reference<yarpl::flowable::Flowable<std::string>>) {
  throw apache::thrift::TApplicationException(
      "Function helloChannel is unimplemented");
}

folly::Future<yarpl::Reference<yarpl::flowable::Flowable<std::string>>>
TestServiceExtensionSvIf::future_helloChannel(
    yarpl::Reference<yarpl::flowable::Flowable<std::string>> input) {
  return apache::thrift::detail::si::future_returning(
      [&](yarpl::Reference<yarpl::flowable::Flowable<std::string>>& output) {
        output = helloChannel(input);
        VLOG(1) << "Output stream: " << (output != nullptr);
      });
}

std::unique_ptr<folly::IOBuf> TestServiceExtensionSvIf::encodeString(
    const std::string& result) {
  apache::thrift::CompactProtocolWriter writer;
  size_t bufSize = writer.serializedSizeString(result);
  folly::IOBufQueue queue;
  writer.setOutput(&queue, bufSize);
  writer.writeString(result);
  std::unique_ptr<folly::IOBuf> buf = queue.move();

  std::unique_ptr<folly::IOBuf> body = std::make_unique<folly::IOBuf>();
  body->prependChain(std::move(buf));

  return body;
}

std::string TestServiceExtensionSvIf::decodeString(
    std::unique_ptr<folly::IOBuf> buffer) {
  std::string result;
  apache::thrift::CompactProtocolReader reader;
  reader.setInput(buffer.get());
  reader.readString(result);
  return result;
}

void TestServiceExtensionSvIf::async_tm_helloChannel(
    std::unique_ptr<apache::thrift::HandlerCallback<void>> callback) {
  // Reach to the Channel object!
  auto request = callback->getRequest();
  auto thriftRequest = dynamic_cast<apache::thrift::ThriftRequest*>(request);
  CHECK_NOTNULL(thriftRequest);
  auto channel = thriftRequest->getChannel();

  apache::thrift::detail::si::async_tm_oneway(
      this, std::move(callback), [&, this] {
        auto input = yarpl::flowable::Flowables::fromPublisher<
            std::unique_ptr<folly::IOBuf>>(
            [&channel](yarpl::Reference<yarpl::flowable::Subscriber<
                           std::unique_ptr<folly::IOBuf>>> subscriber) {
              // TODO: If we want, we need to pass seqId here!
              VLOG(1) << "channel->setInput";
              channel->setInput(0, subscriber);
            });

        auto mapped = input->map([this](std::unique_ptr<folly::IOBuf> buffer) {
          auto result = decodeString(std::move(buffer));
          return result;
        });

        return future_helloChannel(mapped).then(
            [&](folly::Future<yarpl::Reference<
                    yarpl::flowable::Flowable<std::string>>> result) {
              auto prev = result.get();
              auto resultMapped = prev->map(
                  [this](std::string value) { return encodeString(value); });

              // If we want, we need to pass seqId here!
              auto subscriber = channel->getOutput(0);
              resultMapped->subscribe(subscriber);

              return folly::Unit();
            });
      });
}

// Inject the methods to the method map
void TestServiceAsyncProcessorExtension::injectNewMethods() {
  auto binaryProcessMap =
      const_cast<BinaryProtocolProcessMap*>(&getBinaryProtocolProcessMap());
  using BinaryFunction = ProcessFunc<
      TestServiceAsyncProcessor,
      apache::thrift::BinaryProtocolReader>;
  BinaryFunction binaryFunction = static_cast<BinaryFunction>(
      &TestServiceAsyncProcessorExtension::_processInThread_helloChannel<
          apache::thrift::BinaryProtocolReader,
          apache::thrift::BinaryProtocolWriter>);
  binaryProcessMap->emplace("helloChannel", binaryFunction);

  auto compactProcessMap =
      const_cast<CompactProtocolProcessMap*>(&getCompactProtocolProcessMap());
  using CompactFunction = ProcessFunc<
      TestServiceAsyncProcessor,
      apache::thrift::CompactProtocolReader>;
  CompactFunction compactFunction = static_cast<CompactFunction>(
      &TestServiceAsyncProcessorExtension::_processInThread_helloChannel<
          apache::thrift::CompactProtocolReader,
          apache::thrift::CompactProtocolWriter>);
  compactProcessMap->emplace("helloChannel", compactFunction);

  VLOG(1) << "Extra functions are injected!";

  VLOG(1) << "Number of methods: " << getBinaryProtocolProcessMap().size()
          << ", " << getCompactProtocolProcessMap().size();
}
} // namespace testservice
} // namespace testutil
