/*
 * Copyright 2017-present Facebook, Inc.
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
#include "thrift/lib/cpp2/transport/rsocket/client/RSClientThriftChannel.h"

namespace apache {
namespace thrift {

using namespace rsocket;

RSClientThriftChannel::RSClientThriftChannel(
    std::shared_ptr<RSocketRequester> rsRequester)
    : rsRequester_(std::move(rsRequester)) {}

void RSClientThriftChannel::sendThriftRequest(
    std::unique_ptr<FunctionInfo> functionInfo,
    std::unique_ptr<std::map<std::string, std::string>> headers,
    std::unique_ptr<folly::IOBuf> payload,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  // TODO: callback also has the protocolId, check if this is a duplicate info
  // auto protocolId = functionInfo->getProtocolId();

  switch (functionInfo->kind) {
    case FunctionKind::SINGLE_REQUEST_SINGLE_RESPONSE:
      sendSingleRequestResponse(
          std::move(headers), std::move(payload), std::move(callback));
      break;
    default:
      LOG(FATAL) << "not implemented";
  }
}

void RSClientThriftChannel::sendSingleRequestResponse(
    std::unique_ptr<std::map<std::string, std::string>>,
    std::unique_ptr<folly::IOBuf> buf,
    std::unique_ptr<ThriftClientCallback> callback) noexcept {
  std::shared_ptr<ThriftClientCallback> spCallback{std::move(callback)};
  auto func = [spCallback](Payload payload) mutable {
    VLOG(3) << "Received: '"
            << folly::humanify(
                   payload.data->cloneCoalescedAsValue().moveToFbString());

    // TODO: extract headers from the payload.metadata
    auto headers = std::make_unique<std::map<std::string, std::string>>();
    auto evb_ = spCallback->getEventBase();
    evb_->runInEventBaseThread([
      headers = std::move(headers),
      spCallback = std::move(spCallback),
      payload = std::move(payload)
    ]() mutable {
      VLOG(3) << "Pass data to callback: '"
              << folly::humanify(
                     payload.data->cloneCoalescedAsValue().moveToFbString());

      spCallback->onThriftResponse(std::move(headers), std::move(payload.data));
    });
  };

  auto err = [spCallback](folly::exception_wrapper ex) mutable {
    LOG(FATAL) << "This method should never be called: " << ex;

    // TODO: Inspect the cases where might we end up in this function.
  };

  auto singleObserver = yarpl::single::SingleObservers::create<Payload>(
      std::move(func), std::move(err));

  // TODO: send `headers` too
  rsRequester_->requestResponse(rsocket::Payload(std::move(buf)))
      ->subscribe(singleObserver);
}
}
}
