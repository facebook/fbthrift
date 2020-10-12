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
#include <memory>
#include <utility>

#include <glog/logging.h>

#include <folly/Portability.h>

#include <folly/ExceptionString.h>
#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/IOBufQueue.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/ContextStack.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/GeneratedCodeHelper.h>
#include <thrift/lib/cpp2/async/ResponseChannel.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/protocol/detail/protocol_methods.h>
#include <thrift/lib/cpp2/server/Cpp2ConnContext.h>

namespace apache {
namespace thrift {
namespace detail {

namespace ap {

template <typename Prot>
std::unique_ptr<folly::IOBuf> process_serialize_xform_app_exn(
    TApplicationException const& x,
    Cpp2RequestContext* const ctx,
    char const* const method) {
  Prot prot;
  size_t bufSize = x.serializedSizeZC(&prot);
  bufSize += prot.serializedMessageSize(method);
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  prot.setOutput(&queue, bufSize);
  prot.writeMessageBegin(
      method, apache::thrift::T_EXCEPTION, ctx->getProtoSeqId());
  x.write(&prot);
  prot.writeMessageEnd();
  queue.append(transport::THeader::transform(
      queue.move(), ctx->getHeader()->getWriteTransforms()));
  return queue.move();
}

template <typename Prot>
void process_handle_exn_deserialization(
    std::exception const& ex,
    ResponseChannelRequest::UniquePtr req,
    Cpp2RequestContext* const ctx,
    folly::EventBase* const eb,
    char const* const method) {
  LOG(ERROR) << folly::exceptionStr(ex) << " in function " << method;
  TApplicationException x(
      TApplicationException::TApplicationExceptionType::PROTOCOL_ERROR,
      ex.what());
  auto buf = process_serialize_xform_app_exn<Prot>(x, ctx, method);
  eb->runInEventBaseThread(
      [buf = std::move(buf), req = std::move(req)]() mutable {
        if (req->isStream()) {
          std::ignore = req->sendStreamReply(
              std::move(buf), StreamServerCallbackPtr(nullptr));
        } else if (req->isSink()) {
#if FOLLY_HAS_COROUTINES
          req->sendSinkReply(std::move(buf), {});
#else
          DCHECK(false);
#endif
        } else {
          req->sendReply(std::move(buf));
        }
      });
}

template <typename Prot>
void process_throw_wrapped_handler_error(
    folly::exception_wrapper const& ew,
    apache::thrift::ResponseChannelRequest::UniquePtr req,
    Cpp2RequestContext* const ctx,
    ContextStack* const stack,
    char const* const method) {
  LOG(ERROR) << ew << " in function " << method;
  stack->userExceptionWrapped(false, ew);
  stack->handlerErrorWrapped(ew);
  auto xp = ew.get_exception<TApplicationException>();
  auto x = xp ? std::move(*xp) : TApplicationException(ew.what().toStdString());
  auto buf = process_serialize_xform_app_exn<Prot>(x, ctx, method);
  if (req->isStream()) {
    std::ignore =
        req->sendStreamReply(std::move(buf), StreamServerCallbackPtr(nullptr));
  } else if (req->isSink()) {
#if FOLLY_HAS_COROUTINES
    req->sendSinkReply(std::move(buf), {});
#else
    DCHECK(false);
#endif
  } else {
    req->sendReply(std::move(buf));
  }
}

} // namespace ap

} // namespace detail
} // namespace thrift
} // namespace apache
