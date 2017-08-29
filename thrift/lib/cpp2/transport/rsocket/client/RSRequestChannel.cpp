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
#include <thrift/lib/cpp2/transport/rsocket/client/RSRequestChannel.h>

#include <rsocket/transports/tcp/TcpConnectionFactory.h>

namespace apache {
namespace thrift {

using namespace rsocket;
using namespace yarpl;
using namespace yarpl::single;

RSRequestChannel::RSRequestChannel(
    std::shared_ptr<ThriftClient> thriftClient,
    folly::SocketAddress address,
    folly::EventBase* evb)
    : thriftClient_(thriftClient), evb_(evb) {
  rsClient_ = RSocket::createConnectedClient(
                  std::make_unique<TcpConnectionFactory>(
                      *rsClientWorker_.getEventBase(), std::move(address)))
                  .get();
  rsRequester_ = rsClient_->getRequester();
}

uint32_t RSRequestChannel::sendRequest(
    RpcOptions&,
    std::unique_ptr<RequestCallback> cb,
    std::unique_ptr<apache::thrift::ContextStack> ctx,
    std::unique_ptr<folly::IOBuf> buf,
    std::shared_ptr<apache::thrift::transport::THeader>) {
  auto protocolId = getProtocolId();
  std::shared_ptr<apache::thrift::ContextStack> ctx_{std::move(ctx)};
  std::shared_ptr<apache::thrift::RequestCallback> cb_{std::move(cb)};
  auto func = [protocolId, cb_, ctx_](Payload payload) mutable {
    VLOG(3) << "Received: '"
            << folly::humanify(payload.data->clone()->moveToFbString());

    cb_->replyReceived(ClientReceiveState(
        protocolId,
        std::move(payload.data),
        nullptr,
        std::move(ctx_),
        false // isSecurityActive_
        ));
  };

  auto err = [cb_, ctx_](folly::exception_wrapper ew) mutable {
    VLOG(3) << "Error: " << ew.what();
    cb_->replyReceived(ClientReceiveState(
        std::move(ew),
        std::move(ctx_),
        false // isSecurityActive_
        ));
  };

  auto singleObserver = yarpl::single::SingleObservers::create<Payload>(
      std::move(func), std::move(err));

  rsRequester_->requestResponse(rsocket::Payload(std::move(buf)))
      ->subscribe(singleObserver);
  return 0;
}

uint32_t RSRequestChannel::sendOnewayRequest(
    RpcOptions&,
    std::unique_ptr<RequestCallback>,
    std::unique_ptr<apache::thrift::ContextStack>,
    std::unique_ptr<folly::IOBuf>,
    std::shared_ptr<apache::thrift::transport::THeader>) {
  LOG(ERROR) << "Not yet implemented";
  return 0;
}
}
}
