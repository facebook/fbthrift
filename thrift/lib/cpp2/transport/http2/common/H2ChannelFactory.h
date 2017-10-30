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

#include <thrift/lib/cpp2/transport/http2/common/H2Channel.h>

#include <folly/FixedString.h>
#include <proxygen/lib/http/codec/SettingsId.h>

namespace apache {
namespace thrift {

// We support multiple strategies for HTTP2 communication.  Each
// strategy has an associated version id, the greater the value of the
// version id, the more preferred it is.  The base strategy is the
// legacy approach - that the previous implementations support.  This
// is assigned version 1.  We use the HTTP2 SETTINGS frame exchanged
// during connection setup to handshake on the channel version for the
// connection.
//
// Both the client and the server send the maximum version it can
// handle in its SETTINGS frame to the other side
// (kMaxSupportedChannelVersion).  kChannelSettingId is used as the
// SETTINGS id to send this version number.
//
// When the SETTINGS frame is received, the lesser of the maximum
// version values of the client and server is chosen as the negotiated
// version.  If the SETTINGS frame does not contain the maximum
// version number, it is assumed to be a legacy implementation and
// version 1 is assumed for this case.
//
// On the server side, the negotiated version is determined before it
// receives the first RPC.  However, on the client side, it may send
// RPCs to the server before it determines the negotiated version.
// These RPCs are sent using version number 1.  This version number is
// also included as a HTTP2 header with key kChannelVersionKey.
// Legacy clients will send version 1 RPCs also, but without the HTTP2
// header.
//
// The client moves to the negotiated version immediately after
// negotiation and includes the negotiated version in the HTTP2 header
// (with key kChannelVersionKey).  The client stops including the
// negotiated version in the header after it receives a response from
// the server for a RPC sent using the negotiated version.  On the
// server side, once it receives the first RPC with the negotiated
// version, it registers the stream id for this RPC and assumes that
// all future RPCs (with greater stream ids) will be performed using
// the negotiated version.  It does not inspect the header for these
// future RPCs.
//
// If the flag "force_channel_version" is set to a positive value,
// that version is used for all RPCs and the HTTP2 header is not
// set/inspected.

constexpr auto kChannelVersionKey = folly::makeFixedString("cv");
constexpr auto kChannelSettingId = static_cast<proxygen::SettingsId>(100);

// The maximum channel version supported.
//
// Channel version 1:
// Provides legacy functionality (compatible with HTTPClientChannel)
// using SingleRpcChannel.
//
// Channel version 2:
// The metadata is serialized into the body instead of placing them
// in headers as is the case for version 1.  Implemented by
// MetadataInBodySingleRpcChannel.
constexpr uint32_t kMaxSupportedChannelVersion = 2;

class H2ChannelFactory {
 public:
  // Creates a channel on the server based on negotiated version.
  // This is called from the RequestHandler object when a new stream
  // is received from the client.
  static std::shared_ptr<H2Channel> createChannel(
      int32_t version,
      proxygen::ResponseHandler* toHttp2,
      ThriftProcessor* processor);

  // Gets a channel on the client based on negotiated version.  This
  // is called at the beginning of each RPC.  Depending on the
  // strategy used, it is possible that a previously created channel
  // will be reused.
  std::shared_ptr<H2Channel> getChannel(
      int32_t version,
      H2ClientConnection* toHttp2,
      const std::string& httpHost,
      const std::string& httpUrl);
};

} // namespace thrift
} // namespace apache
