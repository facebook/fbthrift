/*
 * Copyright 2016 Facebook, Inc.
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

#include <wangle/channel/Handler.h>
#include <wangle/channel/StaticPipeline.h>
#include <folly/SocketAddress.h>

namespace apache { namespace thrift {

class PcapLoggingHandler : public wangle::BytesToBytesHandler {
 public:
  enum Peer {CLIENT, SERVER};
  enum class EncryptionType : uint8_t {NONE = 0, KRB = 1, SSL = 2};

  explicit PcapLoggingHandler(std::function<bool()> isKrbEncrypted);

  void transportActive(Context* ctx) override;
  void readEOF(Context* ctx) override;
  folly::Future<folly::Unit> close(Context* ctx) override;

  void read(Context* ctx, folly::IOBufQueue& q) override;
  folly::Future<folly::Unit> write(Context* ctx,
      std::unique_ptr<folly::IOBuf> buf) override;

  void readException(Context* ctx, folly::exception_wrapper e) override;
  folly::Future<folly::Unit> writeException(Context* ctx,
      folly::exception_wrapper e) override;

 private:
  EncryptionType getEncryptionType();
  void maybeCheckSsl(Context* ctx);

  bool enabled_ = false;
  Peer peer_;
  folly::Optional<bool> ssl_;
  std::function<bool()> isKrbEncrypted_;
  int snaplen_;
  folly::SocketAddress local_;
  folly::SocketAddress remote_;
};

class PcapLoggingConfig {
 public:
  static void set(std::shared_ptr<const PcapLoggingConfig> config);
  static std::shared_ptr<const PcapLoggingConfig> get() {
    auto p = config_.try_get();
    return p ? p : std::make_shared<const PcapLoggingConfig>();
  }

  // Disables logging
  PcapLoggingConfig()
    : enabled_(false)
  {}

  // Enables logging
  PcapLoggingConfig(
      const char* prefix,
      int snaplen,
      int numMessagesConnStart,
      int numMessagesConnEnd,
      int sampleConnectionPct,
      int rotateAfterMB)
    : enabled_(true)
    , prefix_(prefix)
    , snaplen_(snaplen > 0 && snaplen <= 65000 ? snaplen : 65000)
    , numMessagesConnStart_(numMessagesConnStart)
    , numMessagesConnEnd_(numMessagesConnEnd)
    , sampleConnectionPct_(sampleConnectionPct)
    , rotateAfterMB_(rotateAfterMB)
  {}

  bool enabled() const { return enabled_; }
  std::string prefix() const { return prefix_; }
  int snaplen() const { return snaplen_; }
  int numMessagesConnStart() const { return numMessagesConnStart_; }
  int numMessagesConnEnd() const { return numMessagesConnEnd_; }
  int sampleConnectionPct() const { return sampleConnectionPct_;}
  int rotateAfterMB() const { return rotateAfterMB_; }
 private:
  static folly::Singleton<PcapLoggingConfig> config_;

  bool enabled_;
  std::string prefix_;
  int snaplen_;
  int numMessagesConnStart_;
  int numMessagesConnEnd_;
  int sampleConnectionPct_;
  int rotateAfterMB_;
};

}} // namespace
