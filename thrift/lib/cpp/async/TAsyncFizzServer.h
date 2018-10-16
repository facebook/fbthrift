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

#pragma once

#include <thrift/lib/cpp/async/TAsyncTransport.h>
#include <fizz/server/AsyncFizzServer.h>

namespace apache {
namespace thrift {
namespace async {

class TAsyncFizzServer : public virtual fizz::server::AsyncFizzServer,
                         public TAsyncTransport {
 public:
  TAsyncFizzServer(
      folly::AsyncTransportWrapper::UniquePtr sock,
      const std::shared_ptr<fizz::server::FizzServerContext>& ctx,
      const std::shared_ptr<fizz::ServerExtensions>& ext = nullptr)
    : fizz::server::AsyncFizzServer(std::move(sock), ctx, ext) {}

  void setReadCB(AsyncTransportWrapper::ReadCallback* callback) override {
    fizz::server::AsyncFizzServer::setReadCB(callback);
  }

  folly::AsyncTransportWrapper::ReadCallback* getReadCallback() const override {
    return fizz::server::AsyncFizzServer::getReadCallback();
  }

  void write(
      AsyncTransportWrapper::WriteCallback* callback,
      const void* buf,
      size_t bytes,
      folly::WriteFlags flags = folly::WriteFlags::NONE) override {
    fizz::server::AsyncFizzServer::write(
        callback, buf, bytes, flags);
  }
  void writev(
      AsyncTransportWrapper::WriteCallback* callback,
      const iovec* vec,
      size_t count,
      folly::WriteFlags flags = folly::WriteFlags::NONE) override {
    fizz::server::AsyncFizzServer::writev(
      callback, vec, count, flags);
  }
  void writeChain(
      AsyncTransportWrapper::WriteCallback* callback,
      std::unique_ptr<folly::IOBuf>&& buf,
      folly::WriteFlags flags = folly::WriteFlags::NONE) override {
    fizz::server::AsyncFizzServer::writeChain(
      callback, std::move(buf), flags);
  }
};
} // namespace async
} // namespace thrift
} // namespace apache
