/*
 * Copyright 2014 Facebook, Inc.
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

#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/async/HeaderServerChannel.h>

#include <memory>

namespace apache { namespace thrift {

class DuplexChannel {
 public:
  class Who {
   public:
    enum WhoEnum {UNKNOWN, CLIENT, SERVER};
    explicit Who(WhoEnum who = UNKNOWN) : who_(who) {}
    void set(WhoEnum who) {
      who_ = who;
    }
    WhoEnum get() const {
      DCHECK(who_ != UNKNOWN);
      return who_;
    }
    WhoEnum getOther() const {
      DCHECK(who_ != UNKNOWN);
      return who_ == CLIENT ? SERVER : CLIENT;
    }
   private:
    WhoEnum who_;
  };

  explicit DuplexChannel(
      Who::WhoEnum mainChannel,
      const std::shared_ptr<async::TAsyncTransport>& transport);

  std::shared_ptr<HeaderClientChannel> getClientChannel() {
    return clientChannel_;
  }

  std::shared_ptr<HeaderServerChannel> getServerChannel() {
    return serverChannel_;
  }

 private:
  class DuplexClientChannel : public HeaderClientChannel {
   public:
    DuplexClientChannel(DuplexChannel& duplex,
                        const std::shared_ptr<Cpp2Channel>& cpp2Channel)
      : HeaderClientChannel(cpp2Channel)
      , duplex_(duplex)
    {}
    virtual void sendMessage(Cpp2Channel::SendCallback* callback,
                     std::unique_ptr<folly::IOBuf> buf) override {
      duplex_.lastSender_.set(Who::CLIENT);
      HeaderClientChannel::sendMessage(callback, std::move(buf));
    }
    void messageChannelEOF() override {
      if (duplex_.mainChannel_.get() == Who::CLIENT) {
        HeaderClientChannel::messageChannelEOF();
      } else {
        duplex_.serverChannel_->messageChannelEOF();
      }
    }
   private:
    DuplexChannel& duplex_;
  };

  class DuplexServerChannel : public HeaderServerChannel {
   public:
    DuplexServerChannel(DuplexChannel& duplex,
                        const std::shared_ptr<Cpp2Channel>& cpp2Channel)
      : HeaderServerChannel(cpp2Channel)
      , duplex_(duplex)
    {}
    virtual void sendMessage(Cpp2Channel::SendCallback* callback,
                       std::unique_ptr<folly::IOBuf> buf) override {
      duplex_.lastSender_.set(Who::SERVER);
      HeaderServerChannel::sendMessage(callback, std::move(buf));
    }
    void messageChannelEOF() override {
      if (duplex_.mainChannel_.get() == Who::SERVER) {
        HeaderServerChannel::messageChannelEOF();
      } else {
        duplex_.clientChannel_->messageChannelEOF();
      }
    }
   private:
    DuplexChannel& duplex_;
  };

  class DuplexCpp2Channel : public Cpp2Channel {
   public:
    DuplexCpp2Channel(DuplexChannel& duplex,
        const std::shared_ptr<async::TAsyncTransport>& transport,
        std::unique_ptr<FramingChannelHandler> framingHandler,
        std::unique_ptr<ProtectionChannelHandler> protectionHandler)
      : Cpp2Channel(transport, std::move(framingHandler), std::move(protectionHandler))
      , duplex_(duplex)
      , client_(nullptr)
      , server_(nullptr)
    {}

    void setReceiveCallback(RecvCallback* cb) override {
      // the magic happens in primeCallbacks and useCallback
    }

    void primeCallbacks(RecvCallback* client, RecvCallback* server) {
      DCHECK(client != nullptr);
      DCHECK(server != nullptr);
      DCHECK(client_ == nullptr);
      DCHECK(server_ == nullptr);
      client_ = client;
      server_ = server;
      Cpp2Channel::setReceiveCallback(
          duplex_.mainChannel_.get() == Who::CLIENT ? client_: server_);
    }

    void useCallback(Who::WhoEnum who) {
      switch (who) {
      case Who::CLIENT:
        Cpp2Channel::setReceiveCallback(client_);
        break;
      case Who::SERVER:
        Cpp2Channel::setReceiveCallback(server_);
        break;
      default:
        DCHECK(false);
      }
    }
   private:
    DuplexChannel& duplex_;
    RecvCallback* client_;
    RecvCallback* server_;
  };

  std::shared_ptr<DuplexCpp2Channel> cpp2Channel_;

  std::shared_ptr<DuplexClientChannel> clientChannel_;
  HeaderClientChannel::ClientFramingHandler clientFramingHandler_;

  std::shared_ptr<DuplexServerChannel> serverChannel_;
  HeaderServerChannel::ServerFramingHandler serverFramingHandler_;

  Who mainChannel_;
  Who lastSender_;

  class FramingHandler : public FramingChannelHandler {
   public:
    explicit FramingHandler(DuplexChannel& duplex)
        : duplex_(duplex)
    {}

    std::pair<std::unique_ptr<folly::IOBuf>, size_t>
    removeFrame(folly::IOBufQueue* q) override;

    std::unique_ptr<folly::IOBuf>
    addFrame(std::unique_ptr<folly::IOBuf> buf) override;
   private:
    DuplexChannel& duplex_;

    FramingChannelHandler& getHandler(DuplexChannel::Who::WhoEnum who);
  };

  class ProtectionHandler : public ProtectionChannelHandler {
   public:
    explicit ProtectionHandler(DuplexChannel& duplex)
        : ProtectionChannelHandler()
        , duplex_(duplex)
    {}

    void protectionStateChanged() override {
      if (getProtectionState() != ProtectionState::VALID) {
        return;
      }

      apache::thrift::transport::THeader* src;
      apache::thrift::transport::THeader* dst;

      switch (duplex_.mainChannel_.get()) {
      case Who::CLIENT:
        src = duplex_.clientChannel_->getHeader();
        dst = duplex_.serverChannel_->getHeader();
        break;
      case Who::SERVER:
        src = duplex_.serverChannel_->getHeader();
        dst = duplex_.clientChannel_->getHeader();
        break;
      case Who::UNKNOWN:
        CHECK(false);
      }

      CLIENT_TYPE type = src->getClientType();

      std::bitset<CLIENT_TYPES_LEN> supportedClients;
      supportedClients[type] = true;

      dst->setSupportedClients(&supportedClients);
      dst->setClientType(type);

      if (ctx_ && !inputQueue_.empty()) {
        read(ctx_, inputQueue_);
      }
    }
   private:
    DuplexChannel& duplex_;
  };
};

}} // apache::thrift
