/*
 * Copyright 2018-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/rocket/server/RocketThriftRequests.h>

#include <functional>
#include <memory>
#include <utility>

#include <folly/ExceptionWrapper.h>
#include <folly/Function.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>

#include <thrift/lib/cpp2/async/SemiStream.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/transport/rocket/Types.h>
#include <thrift/lib/cpp2/transport/rocket/framing/Flags.h>
#include <thrift/lib/thrift/gen-cpp2/RpcMetadata_types.h>

namespace apache {
namespace thrift {
namespace rocket {

namespace {
Payload makePayload(
    const ResponseRpcMetadata& responseMetadata,
    std::unique_ptr<folly::IOBuf> data) {
  CompactProtocolWriter writer;
  // Default is to leave some headroom for rsocket headers
  size_t serSize = responseMetadata.serializedSizeZC(&writer);
  constexpr size_t kHeadroomBytes = 16;

  folly::IOBufQueue queue;

  // If possible, serialize metadata into the headeroom of data.
  if (!data->isChained() && data->headroom() >= serSize + kHeadroomBytes &&
      !data->isSharedOne()) {
    // Store previous state of the buffer pointers and rewind it.
    auto startBuffer = data->buffer();
    auto start = data->data();
    auto origLen = data->length();
    data->trimEnd(origLen);
    data->retreat(start - startBuffer);

    queue.append(std::move(data), false);
    writer.setOutput(&queue);
    auto metadataLen = responseMetadata.write(&writer);

    // Move the new data to come right before the old data and restore the
    // old tail pointer.
    data = queue.move();
    data->advance(start - data->tail());
    data->append(origLen);

    return Payload::makeCombined(std::move(data), metadataLen);
  } else {
    constexpr size_t kMinAllocBytes = 1024;
    auto buf = folly::IOBuf::create(
        std::max(kHeadroomBytes + serSize, kMinAllocBytes));
    buf->advance(kHeadroomBytes);
    queue.append(std::move(buf));
    writer.setOutput(&queue);
    auto metadataLen = responseMetadata.write(&writer);
    queue.append(std::move(data));
    return Payload::makeCombined(queue.move(), metadataLen);
  }
}
} // namespace

ThriftServerRequestResponse::ThriftServerRequestResponse(
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    RocketServerFrameContext&& context)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)) {
  scheduleTimeouts();
}

void ThriftServerRequestResponse::sendThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data) noexcept {
  Payload responsePayload;
  std::move(context_).sendPayload(
      makePayload(metadata, std::move(data)),
      Flags::none().next(true).complete(true));
}

void ThriftServerRequestResponse::sendStreamThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>,
    SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept {
  LOG(FATAL) << "Single-response requests cannot send stream responses";
}

ThriftServerRequestFnf::ThriftServerRequestFnf(
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    Cpp2ConnContext& connContext,
    RocketServerFrameContext&& context,
    folly::Function<void()> onComplete)
    : ThriftRequestCore(serverConfigs, std::move(metadata), connContext),
      evb_(evb),
      context_(std::move(context)),
      onComplete_(std::move(onComplete)) {
  scheduleTimeouts();
}

ThriftServerRequestFnf::~ThriftServerRequestFnf() {
  if (auto f = std::move(onComplete_)) {
    f();
  }
}

void ThriftServerRequestFnf::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>) noexcept {
  LOG(FATAL) << "One-way requests cannot send responses";
}

void ThriftServerRequestFnf::sendStreamThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>,
    SemiStream<std::unique_ptr<folly::IOBuf>>) noexcept {
  LOG(FATAL) << "One-way requests cannot send stream responses";
}

ThriftServerRequestStream::ThriftServerRequestStream(
    folly::EventBase& evb,
    server::ServerConfigs& serverConfigs,
    RequestRpcMetadata&& metadata,
    std::shared_ptr<Cpp2ConnContext> connContext,
    StreamClientCallback* clientCallback,
    std::shared_ptr<AsyncProcessor> cpp2Processor)
    : ThriftRequestCore(serverConfigs, std::move(metadata), *connContext),
      evb_(evb),
      clientCallback_(clientCallback),
      connContext_(std::move(connContext)),
      cpp2Processor_(std::move(cpp2Processor)) {
  scheduleTimeouts();
}

void ThriftServerRequestStream::sendThriftResponse(
    ResponseRpcMetadata&&,
    std::unique_ptr<folly::IOBuf>) noexcept {
  LOG(FATAL) << "Stream requests must respond via sendStreamThriftResponse";
}

StreamServerCallback* toStreamServerCallbackPtr(
    SemiStream<std::unique_ptr<folly::IOBuf>> stream,
    StreamClientCallback* clientCallback,
    folly::EventBase& evb);

void ThriftServerRequestStream::sendStreamThriftResponse(
    ResponseRpcMetadata&& metadata,
    std::unique_ptr<folly::IOBuf> data,
    SemiStream<std::unique_ptr<folly::IOBuf>> stream) noexcept {
  if (stream) {
    auto* serverCallback = toStreamServerCallbackPtr(
        std::move(stream), clientCallback_, *getEventBase());
    // Note that onSubscribe will run after onFirstResponse
    clientCallback_->onFirstResponse(
        FirstResponsePayload{std::move(data), std::move(metadata)},
        nullptr /* evb */,
        serverCallback);
  } else {
    std::exchange(clientCallback_, nullptr)
        ->onFirstResponseError(
            folly::make_exception_wrapper<thrift::detail::EncodedError>(
                std::move(data)));
  }
}

StreamServerCallback* toStreamServerCallbackPtr(
    SemiStream<std::unique_ptr<folly::IOBuf>> stream,
    StreamClientCallback* clientCallback,
    folly::EventBase& evb) {
  class StreamServerCallbackAdaptor final
      : public StreamServerCallback,
        public SubscriberIf<std::unique_ptr<folly::IOBuf>> {
   public:
    explicit StreamServerCallbackAdaptor(StreamClientCallback* clientCallback)
        : clientCallback_(clientCallback) {}
    ~StreamServerCallbackAdaptor() override = default;

    // StreamServerCallback implementation
    void onStreamRequestN(uint64_t tokens) override {
      if (!subscription_) {
        tokensBeforeSubscribe_ += tokens;
      } else {
        DCHECK_EQ(0, tokensBeforeSubscribe_);
        subscription_->request(tokens);
      }
    }
    void onStreamCancel() override {
      clientCallback_ = nullptr;
      if (auto subscription = std::move(subscription_)) {
        subscription->cancel();
      }
    }

    // SubscriberIf implementation
    void onSubscribe(std::unique_ptr<SubscriptionIf> subscription) override {
      if (!clientCallback_) {
        return subscription->cancel();
      }

      subscription_ = std::move(subscription);
      if (auto tokens = std::exchange(tokensBeforeSubscribe_, 0)) {
        subscription_->request(tokens);
      }
    }
    void onNext(std::unique_ptr<folly::IOBuf>&& next) override {
      if (clientCallback_) {
        clientCallback_->onStreamNext(StreamPayload{std::move(next), {}});
      }
    }
    void onError(folly::exception_wrapper ew) override {
      if (!clientCallback_) {
        return;
      }

      folly::exception_wrapper hijacked;
      ew.with_exception([&hijacked](thrift::detail::EncodedError& err) {
        hijacked = folly::make_exception_wrapper<RocketException>(
            ErrorCode::APPLICATION_ERROR, std::move(err.encoded));
      });
      if (hijacked) {
        ew = std::move(hijacked);
      }
      std::exchange(clientCallback_, nullptr)->onStreamError(std::move(ew));
    }
    void onComplete() override {
      if (auto* clientCallback = std::exchange(clientCallback_, nullptr)) {
        clientCallback->onStreamComplete();
      }
    }

   private:
    StreamClientCallback* clientCallback_;
    // TODO subscription_ and tokensBeforeSubscribe_ can be packed into a union
    std::unique_ptr<SubscriptionIf> subscription_;
    uint32_t tokensBeforeSubscribe_{0};
  };

  auto serverCallback =
      std::make_unique<StreamServerCallbackAdaptor>(clientCallback);
  auto* serverCallbackPtr = serverCallback.get();

  std::move(stream).via(&evb).subscribe(std::move(serverCallback));

  return serverCallbackPtr;
}

} // namespace rocket
} // namespace thrift
} // namespace apache
