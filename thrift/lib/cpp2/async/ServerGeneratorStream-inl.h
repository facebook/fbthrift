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

namespace apache {
namespace thrift {
namespace detail {

#if FOLLY_HAS_COROUTINES
template <typename T>
ServerStreamFn<T> ServerGeneratorStream::fromAsyncGenerator(
    folly::coro::AsyncGenerator<T&&>&& gen) {
  return [gen = std::move(gen)](
             folly::Executor::KeepAlive<> serverExecutor,
             folly::Try<StreamPayload> (*encode)(folly::Try<T> &&)) mutable {
    return [gen = std::move(gen),
            serverExecutor = std::move(serverExecutor),
            encode](
               FirstResponsePayload&& payload,
               StreamClientCallback* callback,
               folly::EventBase* clientEb) mutable {
      DCHECK(clientEb->isInEventBaseThread());
      auto stream = new ServerGeneratorStream(callback, clientEb);
      auto streamPtr = stream->copy();
      folly::coro::co_invoke(
          [stream = std::move(streamPtr),
           encode,
           gen = std::move(gen)]() mutable -> folly::coro::Task<void> {
            int64_t credits = 0;
            class ReadyCallback
                : public apache::thrift::detail::ServerStreamConsumer {
             public:
              void consume() override {
                baton.post();
              }

              void canceled() override {
                std::terminate();
              }

              folly::coro::Baton baton;
            };
            SCOPE_EXIT {
              stream->serverClose();
            };

            while (true) {
              if (credits == 0) {
                ReadyCallback ready;
                if (stream->wait(&ready)) {
                  co_await ready.baton;
                }
              }

              {
                auto queue = stream->getMessages();
                while (!queue.empty()) {
                  auto next = queue.front();
                  queue.pop();
                  if (next == -1) {
                    co_return;
                  }
                  credits += next;
                }
              }

              try {
                auto&& next = co_await folly::coro::co_withCancellation(
                    stream->cancelSource_.getToken(), gen.next());
                if (next) {
                  stream->publish(encode(folly::Try<T>(std::move(*next))));
                  --credits;
                  continue;
                }
                stream->publish({});
              } catch (const std::exception& e) {
                stream->publish(encode(folly::Try<T>(
                    folly::exception_wrapper(std::current_exception(), e))));
              } catch (...) {
                stream->publish(encode(folly::Try<T>(
                    folly::exception_wrapper(std::current_exception()))));
              }
              co_return;
            }
          })
          .scheduleOn(std::move(serverExecutor))
          .start([](folly::Try<folly::Unit> t) {
            if (t.hasException()) {
              LOG(FATAL) << t.exception().what();
            }
          });
      std::ignore =
          callback->onFirstResponse(std::move(payload), clientEb, stream);
      stream->processPayloads();
    };
  };
}
#endif // FOLLY_HAS_COROUTINES

} // namespace detail
} // namespace thrift
} // namespace apache
