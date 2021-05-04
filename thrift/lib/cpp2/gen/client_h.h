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

#include <folly/fibers/FiberManager.h>
#include <folly/futures/Future.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/async/AsyncClient.h>
#include <thrift/lib/cpp2/async/FutureRequest.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>

#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/Result.h>
#include <folly/experimental/coro/Task.h>
#endif

namespace folly {

class IOBuf;
class IOBufQueue;

} // namespace folly

namespace apache {
namespace thrift {
namespace detail {
namespace ac {

struct ClientRequestContext {
  ClientRequestContext(
      uint16_t protocolId,
      std::map<std::string, std::string> headers,
      const std::shared_ptr<
          std::vector<std::shared_ptr<TProcessorEventHandler>>>& handlers,
      const char* service_name,
      const char* fn_name)
      : header(protocolId, std::move(headers)),
        reqContext(initReqContext(&header)),
        ctx(ContextStack::create(
            handlers, service_name, fn_name, &reqContext)) {}

  struct THeaderWrapper : public transport::THeader {
    THeaderWrapper(
        uint16_t protocolId, std::map<std::string, std::string> headers)
        : transport::THeader(transport::THeader::ALLOW_BIG_FRAMES) {
      this->setProtocolId(protocolId);
      this->setHeaders(std::move(headers));
    }
  };

  THeaderWrapper header;
  Cpp2ClientRequestContext reqContext;
  std::unique_ptr<ContextStack> ctx;

 private:
  static Cpp2ClientRequestContext initReqContext(transport::THeader* header) {
    Cpp2ClientRequestContext reqContext;
    reqContext.setRequestHeader(header);
    return reqContext;
  }
};

} // namespace ac
} // namespace detail
} // namespace thrift
} // namespace apache
