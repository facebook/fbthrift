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

#include <folly/Portability.h>
#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/AsyncGenerator.h>
#endif // FOLLY_HAS_COROUTINES
#include <folly/Try.h>
#include <thrift/lib/cpp2/async/ClientBufferedStream.h>
#include <thrift/lib/cpp2/async/SemiStream.h>
#include <thrift/lib/cpp2/async/ServerGeneratorStream.h>
#include <thrift/lib/cpp2/async/ServerPublisherStream.h>
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace apache {
namespace thrift {

template <typename T>
class ServerStream {
 public:
#if FOLLY_HAS_COROUTINES
  /* implicit */ ServerStream(folly::coro::AsyncGenerator<T&&>&& gen)
      : fn_(detail::ServerGeneratorStream::fromAsyncGenerator(std::move(gen))) {
  }
#endif

  /* implicit */ ServerStream(Stream<T>&& stream)
      : fn_([stream = std::move(stream)](
                folly::Executor::KeepAlive<>,
                folly::Try<StreamPayload> (*encode)(folly::Try<T>&&)) mutable {
          return [stream = std::move(stream).map(
                      [encode](T&& item) mutable {
                        return encode(folly::Try<T>(std::move(item)))
                            .value()
                            .payload;
                      },
                      [encode](folly::exception_wrapper&& ew) mutable {
                        return encode(folly::Try<T>(std::move(ew))).exception();
                      })](
                     FirstResponsePayload&& payload,
                     StreamClientCallback* callback,
                     folly::EventBase* clientEb) mutable {
            auto streamPtr =
                SemiStream<std::unique_ptr<folly::IOBuf>>(std::move(stream))
                    .toStreamServerCallbackPtr(*clientEb);
            streamPtr->resetClientCallback(*callback);
            callback->onFirstResponse(std::move(payload), clientEb, streamPtr);
          };
        }) {}

  static std::pair<ServerStream<T>, ServerStreamPublisher<T>> createPublisher(
      folly::Function<void()> onStreamCompleteOrCancel) {
    auto pair = detail::ServerPublisherStream<T>::create(
        std::move(onStreamCompleteOrCancel));
    return std::make_pair<ServerStream<T>, ServerStreamPublisher<T>>(
        ServerStream<T>(std::move(pair.first)), std::move(pair.second));
  }

  // Helper for unit testing your service handler
  // Blocks until the stream completes, calling the provided function
  // on each value and on the error / completion event.
  void consumeInline(folly::Function<void(folly::Try<T>&&)> consumer) && {
    folly::EventBase eb;
    struct : public detail::ClientStreamBridge::FirstResponseCallback {
      void onFirstResponse(
          FirstResponsePayload&&,
          detail::ClientStreamBridge::ClientPtr clientStreamBridge) override {
        ptr = std::move(clientStreamBridge);
      }
      void onFirstResponseError(folly::exception_wrapper) override {}
      detail::ClientStreamBridge::ClientPtr ptr;
    } firstResponseCallback;
    auto streamBridge =
        detail::ClientStreamBridge::create(&firstResponseCallback);

    auto encode = [](folly::Try<T>&& in) {
      if (in.hasValue()) {
        folly::IOBufQueue buf;
        apache::thrift::CompactSerializer::serialize(*in, &buf);
        return folly::Try<StreamPayload>({buf.move(), {}});
      } else if (in.hasException()) {
        return folly::Try<StreamPayload>(in.exception());
      } else {
        return folly::Try<StreamPayload>();
      }
    };
    auto decode = [](folly::Try<StreamPayload>&& in) {
      if (in.hasValue()) {
        T out;
        apache::thrift::CompactSerializer::deserialize<T>(
            in.value().payload.get(), out);
        return folly::Try<T>(std::move(out));
      } else if (in.hasException()) {
        return folly::Try<T>(in.exception());
      } else {
        return folly::Try<T>();
      }
    };

    (*this)(&eb, encode)({nullptr, {}}, streamBridge, &eb);
    eb.loopOnce();
    auto sub =
        ClientBufferedStream<T>(std::move(firstResponseCallback.ptr), decode, 0)
            .subscribeExTry(&eb, std::move(consumer));
    eb.loop();
    std::move(sub).join();
  }

  detail::ServerStreamFactory operator()(
      folly::Executor::KeepAlive<> serverExecutor,
      folly::Try<StreamPayload> (*encode)(folly::Try<T>&&)) {
    return fn_(std::move(serverExecutor), encode);
  }

 private:
  explicit ServerStream(detail::ServerStreamFn<T> fn) : fn_(std::move(fn)) {}

  detail::ServerStreamFn<T> fn_;
};

template <typename Response, typename StreamElement>
struct ResponseAndServerStream {
  using ResponseType = Response;
  using StreamElementType = StreamElement;

  Response response;
  ServerStream<StreamElement> stream;
};
struct ResponseAndServerStreamFactory {
  folly::IOBufQueue response;
  detail::ServerStreamFactory stream;
};

} // namespace thrift
} // namespace apache
