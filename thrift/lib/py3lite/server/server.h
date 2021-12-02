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

#pragma once

#include <exception>
#include <map>
#include <memory>
#include <string>
#include <Python.h>
#include <glog/logging.h>
#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp2/async/AsyncProcessor.h>
#include <thrift/lib/cpp2/gen/service_tcc.h>

namespace thrift {
namespace py3lite {

constexpr size_t kMaxUexwSize = 1024;

class PythonUserException : public std::exception {
 public:
  PythonUserException(
      std::string type, std::string reason, std::unique_ptr<folly::IOBuf> buf)
      : type_(std::move(type)),
        reason_(std::move(reason)),
        buf_(std::move(buf)) {}
  PythonUserException(const PythonUserException& ex)
      : type_(ex.type_), reason_(ex.reason_), buf_(ex.buf_->clone()) {}

  const std::string& type() const { return type_; }
  const std::string& reason() const { return reason_; }
  const folly::IOBuf* buf() const { return buf_.get(); }
  const char* what() const noexcept override { return reason_.c_str(); }

 private:
  std::string type_;
  std::string reason_;
  std::unique_ptr<folly::IOBuf> buf_;
};

class Py3LiteAsyncProcessor : public apache::thrift::AsyncProcessor {
 public:
  Py3LiteAsyncProcessor(
      const std::map<std::string, PyObject*>& functions,
      folly::Executor::KeepAlive<> executor,
      std::string serviceName)
      : functions_(functions),
        executor(std::move(executor)),
        serviceName_(std::move(serviceName)) {}

  using ProcessFunc = void (Py3LiteAsyncProcessor::*)(
      apache::thrift::ResponseChannelRequest::UniquePtr,
      apache::thrift::SerializedCompressedRequest&&,
      apache::thrift::Cpp2RequestContext* context,
      folly::EventBase* eb,
      apache::thrift::concurrency::ThreadManager* tm);
  struct ProcessFuncs {
    ProcessFunc compact;
    ProcessFunc binary;
  };
  struct Py3LiteMetadata final
      : public apache::thrift::AsyncProcessorFactory::MethodMetadata {
    explicit Py3LiteMetadata(ProcessFuncs funcs) : processFuncs(funcs) {}

    ProcessFuncs processFuncs;
  };

  void handlePythonServerCallback(
      apache::thrift::ProtocolType protocol,
      apache::thrift::Cpp2RequestContext* context,
      folly::Promise<std::unique_ptr<folly::IOBuf>> promise,
      apache::thrift::SerializedRequest serializedRequest);

  void handlePythonServerCallbackOneway(
      apache::thrift::ProtocolType protocol,
      apache::thrift::Cpp2RequestContext* context,
      folly::Promise<folly::Unit> promise,
      apache::thrift::SerializedRequest serializedRequest);

  void processSerializedRequest(
      apache::thrift::ResponseChannelRequest::UniquePtr /* req */,
      apache::thrift::SerializedRequest&& /* serializedRequest */,
      apache::thrift::protocol::PROTOCOL_TYPES /* protType */,
      apache::thrift::Cpp2RequestContext* /* context */,
      folly::EventBase* /* eb */,
      apache::thrift::concurrency::ThreadManager* /* tm */) override {
    LOG(FATAL) << "shouldn't be called";
  }

  void processSerializedCompressedRequestWithMetadata(
      apache::thrift::ResponseChannelRequest::UniquePtr req,
      apache::thrift::SerializedCompressedRequest&& serializedRequest,
      const apache::thrift::AsyncProcessorFactory::MethodMetadata&
          untypedMethodMetadata,
      apache::thrift::protocol::PROTOCOL_TYPES protType,
      apache::thrift::Cpp2RequestContext* context,
      folly::EventBase* eb,
      apache::thrift::concurrency::ThreadManager* tm) override {
    const auto& methodMetadata =
        apache::thrift::AsyncProcessorHelper::expectMetadataOfType<
            Py3LiteMetadata>(untypedMethodMetadata);
    ProcessFunc pfn;
    switch (protType) {
      case apache::thrift::protocol::T_BINARY_PROTOCOL: {
        pfn = methodMetadata.processFuncs.binary;
        break;
      }
      case apache::thrift::protocol::T_COMPACT_PROTOCOL: {
        pfn = methodMetadata.processFuncs.compact;
        break;
      }
      default:
        LOG(ERROR) << "invalid protType: " << folly::to_underlying(protType);
        return;
    }
    (this->*pfn)(std::move(req), std::move(serializedRequest), context, eb, tm);
  }

  template <typename ProtocolIn_, typename ProtocolOut_>
  void genericProcessor(
      apache::thrift::ResponseChannelRequest::UniquePtr req,
      apache::thrift::SerializedCompressedRequest&& serializedRequest,
      apache::thrift::Cpp2RequestContext* ctx,
      folly::EventBase* eb,
      apache::thrift::concurrency::ThreadManager* tm) {
    const std::string funcName =
        fmt::format("{}.{}", serviceName_, ctx->getMethodName());
    std::unique_ptr<apache::thrift::ContextStack> ctxStack(
        this->getContextStack(serviceName_.c_str(), funcName.c_str(), ctx));
    static_assert(ProtocolIn_::protocolType() == ProtocolOut_::protocolType());
    ProtocolIn_ prot;

    auto callback = std::make_unique<
        apache::thrift::HandlerCallback<std::unique_ptr<::folly::IOBuf>>>(
        std::move(req),
        std::move(ctxStack),
        return_serialized<ProtocolIn_, ProtocolOut_>,
        throw_wrapped<ProtocolIn_, ProtocolOut_>,
        ctx->getProtoSeqId(),
        eb,
        tm,
        ctx);
    folly::via(
        this->executor,
        [this,
         prot,
         ctx,
         callback = std::move(callback),
         serializedRequest = std::move(serializedRequest)]() mutable {
          auto [promise, future] =
              folly::makePromiseContract<std::unique_ptr<folly::IOBuf>>();
          handlePythonServerCallback(
              prot.protocolType(),
              ctx,
              std::move(promise),
              std::move(serializedRequest).uncompress());
          std::move(future)
              .via(this->executor)
              .thenTry([callback = std::move(callback)](
                           folly::Try<std::unique_ptr<folly::IOBuf>>&& t) {
                callback->complete(std::move(t));
              });
        });
  }

  template <typename ProtocolIn_, typename ProtocolOut_>
  void onewayProcessor(
      apache::thrift::ResponseChannelRequest::UniquePtr req,
      apache::thrift::SerializedCompressedRequest&& serializedRequest,
      apache::thrift::Cpp2RequestContext* ctx,
      folly::EventBase* eb,
      apache::thrift::concurrency::ThreadManager* tm) {
    static_assert(ProtocolIn_::protocolType() == ProtocolOut_::protocolType());
    ProtocolIn_ prot;
    const std::string funcName =
        fmt::format("{}.{}", serviceName_, ctx->getMethodName());
    std::unique_ptr<apache::thrift::ContextStack> ctxStack(
        this->getContextStack(serviceName_.c_str(), funcName.c_str(), ctx));
    auto callback = std::make_unique<apache::thrift::HandlerCallbackBase>(
        std::move(req), std::move(ctxStack), nullptr, eb, tm, ctx);

    folly::via(
        this->executor,
        [this,
         prot,
         ctx,
         callback = std::move(callback),
         serializedRequest = std::move(serializedRequest)]() mutable {
          auto [promise, future] = folly::makePromiseContract<folly::Unit>();
          handlePythonServerCallbackOneway(
              prot.protocolType(),
              ctx,
              std::move(promise),
              std::move(serializedRequest).uncompress());
          std::move(future)
              .via(this->executor)
              .thenTry([callback = std::move(callback)](
                           folly::Try<folly::Unit>&& /* t */) {});
        });
  }

  static const Py3LiteAsyncProcessor::ProcessFuncs getSingleFunc() {
    return singleFunc_;
  }

  static const Py3LiteAsyncProcessor::ProcessFuncs getOnewayFunc() {
    return onewayFunc_;
  }

 private:
  const std::map<std::string, PyObject*>& functions_;
  folly::Executor::KeepAlive<> executor;
  std::string serviceName_;
  static inline const Py3LiteAsyncProcessor::ProcessFuncs singleFunc_{
      &Py3LiteAsyncProcessor::genericProcessor<
          apache::thrift::CompactProtocolReader,
          apache::thrift::CompactProtocolWriter>,
      &Py3LiteAsyncProcessor::genericProcessor<
          apache::thrift::BinaryProtocolReader,
          apache::thrift::BinaryProtocolWriter>};
  static inline const Py3LiteAsyncProcessor::ProcessFuncs onewayFunc_{
      &Py3LiteAsyncProcessor::onewayProcessor<
          apache::thrift::CompactProtocolReader,
          apache::thrift::CompactProtocolWriter>,
      &Py3LiteAsyncProcessor::onewayProcessor<
          apache::thrift::BinaryProtocolReader,
          apache::thrift::BinaryProtocolWriter>};

  template <class ProtocolIn, class ProtocolOut>
  static apache::thrift::SerializedResponse return_serialized(
      apache::thrift::ContextStack* ctx, ::folly::IOBuf const& _return) {
    folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
    ProtocolOut prot;

    // Preallocate small buffer headroom for transports metadata & framing.
    constexpr size_t kHeadroomBytes = 128;
    auto buf = folly::IOBuf::create(kHeadroomBytes);
    buf->advance(kHeadroomBytes);
    queue.append(std::move(buf));

    prot.setOutput(&queue, 0);
    if (ctx) {
      ctx->preWrite();
    }
    queue.append(_return);
    if (ctx) {
      apache::thrift::SerializedMessage smsg;
      smsg.protocolType = prot.protocolType();
      smsg.methodName = "";
      smsg.buffer = queue.front();
      ctx->onWriteData(smsg);
    }
    DCHECK_LE(
        queue.chainLength(),
        static_cast<size_t>(std::numeric_limits<int>::max()));
    if (ctx) {
      ctx->postWrite(folly::to_narrow(queue.chainLength()));
    }
    return apache::thrift::SerializedResponse{queue.move()};
  }

  template <class ProtocolIn_, class ProtocolOut_>
  static void throw_wrapped(
      apache::thrift::ResponseChannelRequest::UniquePtr req,
      int32_t protoSeqId,
      apache::thrift::ContextStack* ctx,
      folly::exception_wrapper ew,
      apache::thrift::Cpp2RequestContext* reqCtx) {
    if (!ew) {
      return;
    }
    {
      if (ew.with_exception([&](const PythonUserException& e) {
            auto header = reqCtx->getHeader();
            if (!header) {
              return;
            }

            // TODO: (ffrancet) error kind overrides currently usupported,
            // by py3lite, add kHeaderExMeta header support when it is
            header->setHeader(
                std::string(apache::thrift::detail::kHeaderUex), e.type());
            const std::string reason = e.reason();
            header->setHeader(
                std::string(apache::thrift::detail::kHeaderUexw),
                reason.size() > kMaxUexwSize ? reason.substr(0, kMaxUexwSize)
                                             : reason);

            ProtocolOut_ prot;
            auto response =
                return_serialized<ProtocolIn_, ProtocolOut_>(ctx, *e.buf());
            auto payload = std::move(response).extractPayload(
                req->includeEnvelope(),
                prot.protocolType(),
                protoSeqId,
                apache::thrift::MessageType::T_REPLY,
                reqCtx->getMethodName().c_str());
            payload.transform(reqCtx->getHeader()->getWriteTransforms());
            return req->sendReply(std::move(payload));
          })) {
      } else {
        apache::thrift::detail::ap::process_throw_wrapped_handler_error<
            ProtocolOut_>(
            ew, std::move(req), reqCtx, ctx, reqCtx->getMethodName().c_str());
      }
    }
  }
};

class Py3LiteAsyncProcessorFactory
    : public apache::thrift::AsyncProcessorFactory {
 public:
  Py3LiteAsyncProcessorFactory(
      std::map<std::string, PyObject*> functions,
      std::unordered_set<std::string> oneways,
      folly::Executor::KeepAlive<> executor,
      std::string serviceName)
      : functions_(std::move(functions)),
        oneways_(std::move(oneways)),
        executor(std::move(executor)),
        serviceName_(std::move(serviceName)) {}

  std::unique_ptr<apache::thrift::AsyncProcessor> getProcessor() override {
    return std::make_unique<Py3LiteAsyncProcessor>(
        functions_, executor, serviceName_);
  }

  std::vector<apache::thrift::ServiceHandler*> getServiceHandlers() override {
    return {};
  }

  CreateMethodMetadataResult createMethodMetadata() override {
    AsyncProcessorFactory::MethodMetadataMap result;
    const auto processFunc =
        std::make_shared<Py3LiteAsyncProcessor::Py3LiteMetadata>(
            Py3LiteAsyncProcessor::getSingleFunc());
    const auto onewayFunc =
        std::make_shared<Py3LiteAsyncProcessor::Py3LiteMetadata>(
            Py3LiteAsyncProcessor::getOnewayFunc());

    for (const auto& [methodName, _] : functions_) {
      const auto& func = oneways_.find(methodName) != oneways_.end()
          ? onewayFunc
          : processFunc;
      result.emplace(methodName, func);
    }

    return result;
  }

 private:
  const std::map<std::string, PyObject*> functions_;
  const std::unordered_set<std::string> oneways_;
  folly::Executor::KeepAlive<> executor;
  std::string serviceName_;
};

} // namespace py3lite
} // namespace thrift
