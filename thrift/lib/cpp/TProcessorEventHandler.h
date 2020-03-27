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

#include <folly/ExceptionWrapper.h>

#include <thrift/lib/cpp/SerializedMessage.h>
#include <thrift/lib/cpp/server/TConnectionContext.h>
#include <thrift/lib/cpp/transport/THeader.h>

namespace apache {
namespace thrift {

using server::TConnectionContext;

/**
 * Virtual interface class that can handle events from the processor. To
 * use this you should subclass it and implement the methods that you care
 * about. Your subclass can also store local data that you may care about,
 * such as additional "arguments" to these methods (stored in the object
 * instance's state).
 */
class TProcessorEventHandler {
 public:
  virtual ~TProcessorEventHandler() {}

  /**
   * Called before calling other callback methods.
   * Expected to return some sort of context object.
   * The return value is passed to all other callbacks
   * for that function invocation.
   */
  virtual void* getServiceContext(
      const char* /*service_name*/,
      const char* fn_name,
      TConnectionContext* connectionContext) {
    return getContext(fn_name, connectionContext);
  }
  virtual void* getContext(
      const char* /*fn_name*/,
      TConnectionContext* /*connectionContext*/) {
    return nullptr;
  }

  /**
   * Expected to free resources associated with a context.
   */
  virtual void freeContext(void* /*ctx*/, const char* /*fn_name*/) {}

  /**
   * Called before reading arguments.
   */
  virtual void preRead(void* /*ctx*/, const char* /*fn_name*/) {}

  /**
   * Called before postRead, after reading arguments (server) / after reading
   * reply (client), with the actual (unparsed, serialized) data.
   *
   * The data is framed by message begin / end entries, call readMessageBegin /
   * readMessageEnd on the protocol.
   *
   * Only called for Cpp2.
   */
  virtual void onReadData(
      void* /*ctx*/,
      const char* /*fn_name*/,
      const SerializedMessage& /*msg*/) {}

  /**
   * Called between reading arguments and calling the handler.
   */
  virtual void postRead(
      void* /*ctx*/,
      const char* /*fn_name*/,
      apache::thrift::transport::THeader* /*header*/,
      uint32_t /*bytes*/) {}

  /**
   * Called between calling the handler and writing the response.
   */
  virtual void preWrite(void* /*ctx*/, const char* /*fn_name*/) {}

  /**
   * Called before postWrite, after serializing response (server) / after
   * serializing request (client), with the actual (serialized) data.
   *
   * The data is framed by message begin / end entries, call readMessageBegin /
   * readMessageEnd on the protocol.
   *
   * Only called for Cpp2.
   */
  virtual void onWriteData(
      void* /*ctx*/,
      const char* /*fn_name*/,
      const SerializedMessage& /*msg*/) {}

  /**
   * Called after writing the response.
   */
  virtual void
  postWrite(void* /*ctx*/, const char* /*fn_name*/, uint32_t /*bytes*/) {}

  /**
   * Called when an async function call completes successfully.
   */
  virtual void asyncComplete(void* /*ctx*/, const char* /*fn_name*/) {}

  /**
   * Called if the handler throws an undeclared exception.
   */
  virtual void handlerError(void* /*ctx*/, const char* /*fn_name*/) {}
  virtual void handlerErrorWrapped(
      void* ctx,
      const char* fn_name,
      const folly::exception_wrapper& /*ew*/) {
    handlerError(ctx, fn_name);
  }

  /**
   * Called if the handler throws an exception.
   *
   * Only called for Cpp2
   */
  virtual void userException(
      void* /*ctx*/,
      const char* /*fn_name*/,
      const std::string& /*ex*/,
      const std::string& /*ex_what*/) {}
  virtual void userExceptionWrapped(
      void* ctx,
      const char* fn_name,
      bool declared,
      const folly::exception_wrapper& ew_) {
    const auto type = ew_.class_name();
    const auto what = ew_.what();
    folly::StringPiece whatsp(what);
    if (declared) {
      CHECK(whatsp.removePrefix(type)) << "weird format: '" << what << "'";
      CHECK(whatsp.removePrefix(": ")) << "weird format: '" << what << "'";
    }
    return userException(ctx, fn_name, type.toStdString(), whatsp.str());
  }

 protected:
  TProcessorEventHandler() {}
};

} // namespace thrift
} // namespace apache
