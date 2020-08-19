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

#ifndef THRIFT_ASYNC_RESPONSECHANNEL_H_
#define THRIFT_ASYNC_RESPONSECHANNEL_H_ 1

#include <chrono>
#include <limits>
#include <memory>

#include <folly/Portability.h>

#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/server/TServerObserver.h>
#include <thrift/lib/cpp2/async/MessageChannel.h>
#include <thrift/lib/cpp2/async/ServerStream.h>
#include <thrift/lib/cpp2/server/RequestsRegistry.h>
#if FOLLY_HAS_COROUTINES
#include <thrift/lib/cpp2/async/Sink.h>
#endif
#include <thrift/lib/cpp2/async/StreamCallbacks.h>
#include <thrift/lib/cpp2/server/AdmissionController.h>

namespace folly {
class IOBuf;
}

extern const std::string kUnknownErrorCode;
extern const std::string kOverloadedErrorCode;
extern const std::string kAppOverloadedErrorCode;
extern const std::string kAppClientErrorCode;
extern const std::string kAppServerErrorCode;
extern const std::string kTaskExpiredErrorCode;
extern const std::string kProxyTransportExceptionErrorCode;
extern const std::string kProxyClientProtocolExceptionErrorCode;
extern const std::string kQueueOverloadedErrorCode;
extern const std::string kProxyHeaderParseExceptionErrorCode;
extern const std::string kProxyAuthExceptionErrorCode;
extern const std::string kProxyLookupTransportExceptionErrorCode;
extern const std::string kProxyLookupAppExceptionErrorCode;
extern const std::string kProxyWhitelistExceptionErrorCode;
extern const std::string kProxyClientAppExceptionErrorCode;
extern const std::string kProxyProtocolMismatchExceptionErrorCode;
extern const std::string kProxyQPSThrottledExceptionErrorCode;
extern const std::string kProxyResponseSizeThrottledExceptionErrorCode;
extern const std::string kInjectedFailureErrorCode;
extern const std::string kServerQueueTimeoutErrorCode;
extern const std::string kResponseTooBigErrorCode;
extern const std::string kProxyAclCheckExceptionErrorCode;
extern const std::string kProxyOverloadedErrorCode;
extern const std::string kProxyLoopbackErrorCode;
extern const std::string kRequestTypeDoesntMatchServiceFunctionType;
extern const std::string kMethodUnknownErrorCode;

namespace apache {
namespace thrift {

class ResponseChannelRequest {
 public:
  using UniquePtr =
      std::unique_ptr<ResponseChannelRequest, RequestsRegistry::Deleter>;

  folly::IOBuf* getBuf() {
    return buf_.get();
  }
  std::unique_ptr<folly::IOBuf> extractBuf() {
    return std::move(buf_);
  }

  virtual bool isActive() const = 0;

  virtual void cancel() = 0;

  virtual bool isOneway() const = 0;

  virtual bool isStream() const {
    return false;
  }

  virtual bool isSink() const {
    return false;
  }

  apache::thrift::RpcKind rpcKind() const {
    if (isStream()) {
      return apache::thrift::RpcKind::SINGLE_REQUEST_STREAMING_RESPONSE;
    }
    if (isSink()) {
      return apache::thrift::RpcKind::SINK;
    }
    if (isOneway()) {
      return apache::thrift::RpcKind::SINGLE_REQUEST_NO_RESPONSE;
    }
    return apache::thrift::RpcKind::SINGLE_REQUEST_SINGLE_RESPONSE;
  }

  virtual bool isReplyChecksumNeeded() const {
    return false;
  }

  virtual void sendReply(
      std::unique_ptr<folly::IOBuf>&&,
      MessageChannel::SendCallback* cb = nullptr,
      folly::Optional<uint32_t> crc32 = folly::none) = 0;

  virtual void sendStreamReply(
      std::unique_ptr<folly::IOBuf>&&,
      detail::ServerStreamFactory&&,
      folly::Optional<uint32_t> = folly::none) {
    throw std::logic_error("unimplemented");
  }

  FOLLY_NODISCARD virtual bool sendStreamReply(
      std::unique_ptr<folly::IOBuf>,
      StreamServerCallbackPtr,
      folly::Optional<uint32_t> = folly::none) {
    throw std::logic_error("unimplemented");
  }

#if FOLLY_HAS_COROUTINES
  virtual void sendSinkReply(
      std::unique_ptr<folly::IOBuf>&&,
      apache::thrift::detail::SinkConsumerImpl&&,
      folly::Optional<uint32_t> = folly::none) {
    throw std::logic_error("unimplemented");
  }
#endif

  virtual void sendErrorWrapped(
      folly::exception_wrapper ex,
      std::string exCode) = 0;

  virtual ~ResponseChannelRequest() {
    if (admissionController_ != nullptr) {
      if (!startedProcessing_) {
        admissionController_->dequeue();
      } else {
        auto latency = std::chrono::steady_clock::now() - creationTimestamps_;
        admissionController_->returnedResponse(latency);
      }
    }
  }

  virtual void setStartedProcessing() {
    startedProcessing_ = true;
    if (admissionController_ != nullptr) {
      admissionController_->dequeue();
      creationTimestamps_ = std::chrono::steady_clock::now();
    }
  }

  virtual bool getStartedProcessing() {
    return startedProcessing_;
  }

  void setAdmissionController(
      std::shared_ptr<AdmissionController> admissionController) {
    admissionController_ = std::move(admissionController);
  }

  std::shared_ptr<AdmissionController> getAdmissionController() const {
    return admissionController_;
  }

 protected:
  std::unique_ptr<folly::IOBuf> buf_;
  std::shared_ptr<apache::thrift::AdmissionController> admissionController_;
  bool startedProcessing_{false};
  std::chrono::steady_clock::time_point creationTimestamps_;
};

/**
 * ResponseChannel defines an asynchronous API for servers.
 */
class ResponseChannel : virtual public folly::DelayedDestruction {
 public:
  static const uint32_t ONEWAY_REQUEST_ID =
      std::numeric_limits<uint32_t>::max();

  class Callback {
   public:
    /**
     * reason is empty if closed due to EOF, or a pointer to an exception
     * if closed due to some sort of error.
     */
    virtual void channelClosed(folly::exception_wrapper&&) = 0;

    virtual ~Callback() {}
  };

  /**
   * The callback will be invoked on each new request.
   * It will remain installed until explicitly uninstalled, or until
   * channelClosed() is called.
   */
  virtual void setCallback(Callback*) = 0;

 protected:
  ~ResponseChannel() override {}
};

} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_ASYNC_RESPONSECHANNEL_H_
