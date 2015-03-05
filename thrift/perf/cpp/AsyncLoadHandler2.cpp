/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "thrift/perf/cpp/AsyncLoadHandler2.h"

#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/concurrency/Util.h>

#include <unistd.h>

using apache::thrift::async::TEventBase;
using apache::thrift::concurrency::Util;

namespace apache { namespace thrift {

void AsyncLoadHandler2::async_eb_noop(
  std::unique_ptr<HandlerCallback<void>> callback) {
  // Note that we could have done this with a sync function,
  // but an inline async op is faster, and we want to maintain
  // parity with the old loadgen for comparison testing
  callback.release()->doneInThread();
}

void AsyncLoadHandler2::async_eb_onewayNoop(
  std::unique_ptr<HandlerCallbackBase> callback) {
  callback.release()->deleteInThread();
}

void AsyncLoadHandler2::async_eb_asyncNoop(
  std::unique_ptr<HandlerCallback<void>> callback) {
  callback.release()->doneInThread();
}

void AsyncLoadHandler2::async_eb_sleep(
  std::unique_ptr<HandlerCallback<void>> callback,
  int64_t microseconds) {
  // May leak if task never finishes
  HandlerCallback<void>* callbackp = callback.release();
  callbackp->getEventBase()->runInEventBaseThread([=]() {
    callbackp->getEventBase()->tryRunAfterDelay([=](){
      std::unique_ptr<HandlerCallback<void>> cb(callbackp);
      cb->done();
    },
    microseconds / Util::US_PER_MS);
  });
}

void AsyncLoadHandler2::async_eb_onewaySleep(
  std::unique_ptr<HandlerCallbackBase> callback,
  int64_t microseconds) {
  auto callbackp = callback.release();
  // May leak if task never finishes
  auto eb = callbackp->getEventBase();
  eb->runInEventBaseThread([=]() {
    eb->tryRunAfterDelay([=](){
      delete callbackp;
    },
    microseconds / Util::US_PER_MS);
  });
}

void AsyncLoadHandler2::sync_burn(int64_t microseconds) {
  // Slightly different from thrift1, this happens in a
  // thread pool.
  int64_t start = Util::currentTimeUsec();
  int64_t end = start + microseconds;
  int64_t now;
  do {
    now = Util::currentTimeUsec();
  } while (now < end);
}

folly::Future<void>
AsyncLoadHandler2::future_burn(int64_t microseconds) {
  folly::MoveWrapper<folly::Promise<void>> promise;
  auto future = promise->getFuture();

  sync_burn(microseconds);
  promise->setValue();

  return future;
}

void AsyncLoadHandler2::sync_onewayBurn(int64_t microseconds) {
  sync_burn(microseconds);
}

folly::Future<void>
AsyncLoadHandler2::future_onewayBurn(int64_t microseconds) {
  folly::MoveWrapper<folly::Promise<void>> promise;
  auto future = promise->getFuture();

  sync_onewayBurn(microseconds);
  promise->setValue();

  return future;
}

void AsyncLoadHandler2::async_eb_badSleep(
  std::unique_ptr<HandlerCallback<void>> callback,
  int64_t microseconds) {
  usleep(microseconds);
  callback->done();
}

void AsyncLoadHandler2::async_eb_badBurn(
  std::unique_ptr<HandlerCallback<void>> callback,
  int64_t microseconds) {
  // This is a true (bad) async call.
  sync_burn(microseconds);
  callback->done();
}

void AsyncLoadHandler2::async_eb_throwError(
  std::unique_ptr<HandlerCallback<void>> callback,
  int32_t code) {

  LoadError error;
  error.code = code;
  callback.release()->exceptionInThread(error);
}

void AsyncLoadHandler2::async_eb_throwUnexpected(
  std::unique_ptr<HandlerCallback<void>> callback,
  int32_t code) {

  // FIXME: it isn't possible to implement this behavior with the async code
  //
  // Actually throwing an exception from the handler is bad, and TEventBase
  // should probably be changed to fatal the entire program if that happens.
  callback.release()->doneInThread();
}

void AsyncLoadHandler2::async_eb_onewayThrow(
  std::unique_ptr<HandlerCallbackBase> callback,
  int32_t code) {

  LoadError error;
  error.code = code;
  callback.release()->exceptionInThread(error);
}

void AsyncLoadHandler2::async_eb_send(
  std::unique_ptr<HandlerCallback<void>> callback,
  std::unique_ptr<std::string> data) {
  callback.release()->doneInThread();
}

void AsyncLoadHandler2::async_eb_onewaySend(
  std::unique_ptr<HandlerCallbackBase> callback,
  std::unique_ptr<std::string> data) {
  callback.release()->deleteInThread();
}

void AsyncLoadHandler2::async_eb_recv(
  std::unique_ptr<HandlerCallback<std::unique_ptr<std::string>>> callback,
  int64_t bytes) {
  std::unique_ptr<std::string> ret(new std::string(bytes, 'a'));
  callback.release()->resultInThread(std::move(ret));
}

void AsyncLoadHandler2::async_eb_sendrecv(
  std::unique_ptr<HandlerCallback<std::unique_ptr<std::string>>> callback,
  std::unique_ptr<std::string> data, int64_t recvBytes) {
  std::unique_ptr<std::string> ret(new std::string(recvBytes, 'a'));
  callback.release()->resultInThread(std::move(ret));
}

void AsyncLoadHandler2::sync_echo(
  // Slightly different from thrift1, this happens in a
  // thread pool.
  std::string& output,
  std::unique_ptr<std::string> data) {
  output = std::move(*data);
}

folly::Future<std::unique_ptr<std::string>>
AsyncLoadHandler2::future_echo(
    std::unique_ptr<std::string> data) {
  folly::MoveWrapper<
    folly::Promise<std::unique_ptr<std::string>>> promise;
  auto future = promise->getFuture();
  auto wrapped_data =
    folly::MoveWrapper<std::unique_ptr<std::string>>(std::move(data));

  folly::via(folly::RequestEventBase::get()).then(
    [this, promise, wrapped_data]() mutable {
      std::string output;
      sync_echo(output, std::move(*wrapped_data));
      promise->setValue(folly::make_unique<std::string>(std::move(output)));
    });

  return future;
}

void AsyncLoadHandler2::async_eb_add(
  std::unique_ptr<HandlerCallback<int64_t>> callback,
  int64_t a,
  int64_t b){
  callback.release()->resultInThread(a+b);
}

}} // apache::thrift
