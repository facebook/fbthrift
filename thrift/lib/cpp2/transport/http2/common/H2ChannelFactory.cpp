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
#include <thrift/lib/cpp2/transport/http2/common/H2ChannelFactory.h>

#include <gflags/gflags.h>
#include <thrift/lib/cpp2/transport/http2/common/SingleRpcChannel.h>
#include <cstdlib>

DEFINE_int32(
    timeout_fudge_factor_ms,
    5,
    "Fudge factor allowed for timeout expiration");

namespace apache {
namespace thrift {

using std::chrono::duration_cast;
using std::chrono::milliseconds;
using std::chrono::steady_clock;

std::shared_ptr<H2Channel> H2ChannelFactory::createChannel(
    int32_t version,
    proxygen::ResponseHandler* toHttp2,
    ThriftProcessor* processor) {
  if (version == 3) {
    return std::make_shared<MultiRpcChannel>(toHttp2, processor);
  } else if (version == 2) {
    return std::make_shared<SingleRpcChannel>(toHttp2, processor, false);
  } else {
    DCHECK(version == 0 || version == 1);
    return std::make_shared<SingleRpcChannel>(toHttp2, processor, true);
  }
}

std::shared_ptr<H2Channel> H2ChannelFactory::getChannel(
    int32_t version,
    H2ClientConnection* toHttp2,
    RequestRpcMetadata* metadata) {
  if (version == 3) {
    DCHECK(metadata->__isset.clientTimeoutMs);
    milliseconds timeout(metadata->clientTimeoutMs);
    auto currentTime =
        duration_cast<milliseconds>(steady_clock::now().time_since_epoch());
    auto expiration = currentTime + timeout;
    if (!(multiRpcChannel_ &&
          std::abs(expiration.count() - multiRpcChannelExpiration_.count()) <=
              FLAGS_timeout_fudge_factor_ms &&
          multiRpcChannel_->canDoRpcs())) {
      if (multiRpcChannel_) {
        multiRpcChannel_->closeClientSide();
      }
      multiRpcChannel_.reset(new MultiRpcChannel(toHttp2));
      multiRpcChannel_->initialize(timeout);
      multiRpcChannelExpiration_ = expiration;
    }
    return multiRpcChannel_;
  } else if (version == 2) {
    return std::make_shared<SingleRpcChannel>(toHttp2, false);
  } else {
    DCHECK(version == 0 || version == 1);
    auto ch = std::make_shared<SingleRpcChannel>(toHttp2, true);
    if (version == 0) {
      ch->setNotYetStable();
    }
    return ch;
  }
}

} // namespace thrift
} // namespace apache
