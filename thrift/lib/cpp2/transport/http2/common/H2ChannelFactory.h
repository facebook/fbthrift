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

#pragma once

#include <thrift/lib/cpp2/transport/http2/common/H2ChannelIf.h>

#include <folly/FixedString.h>
#include <proxygen/lib/http/codec/SettingsId.h>

namespace apache {
namespace thrift {

// We send the channel version in the header of each stream to the
// server using this key.  The server uses this to construct the
// correct channel object.
// TODO: Once we figure out how to pass the negotiated channel version
// to the RequestHandler object, we can stop using this strategy (of
// using headers).
constexpr auto kChannelVersionKey = folly::makeFixedString("cv");

// The maximum channel version supported.  During negotiation, the
// smaller of these two values of the client and server is selected.
// So the largest channel version is preferred.
//
// Channel version 1:
// Provides legacy functionality (compatible with HTTPClientChannel)
// using SingleRpcChannel.
//
// Channel version 2:
// The standard functionality supported by SingleRpcChannel.
constexpr uint32_t kMaxSupportedChannelVersion = 2;

// The settings id used to send kMaxSupportedChannelVersion to the
// peer.
constexpr auto kChannelSettingId = static_cast<proxygen::SettingsId>(100);

class H2ChannelFactory {
 public:
  // Creates a channel on the server based on negotiated version.
  static std::shared_ptr<H2ChannelIf> createChannel(
      int32_t version,
      proxygen::ResponseHandler* toHttp2,
      ThriftProcessor* processor);

  // Creates a channel on the client based on negotiated version.
  static std::shared_ptr<H2ChannelIf> createChannel(
      int32_t version,
      H2ClientConnection* toHttp2,
      const std::string& httpHost,
      const std::string& httpUrl);
};

} // namespace thrift
} // namespace apache
