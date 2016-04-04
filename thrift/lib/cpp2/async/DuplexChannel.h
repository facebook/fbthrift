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

  std::shared_ptr<Cpp2Channel> getCpp2Channel() {
    return cpp2Channel_;
  }

 private:
  class DuplexClientChannel : public HeaderClientChannel {
   public:
    DuplexClientChannel(DuplexChannel& duplex,
                        const std::shared_ptr<Cpp2Channel>& cpp2Channel)
      : HeaderClientChannel(cpp2Channel)
      , duplex_(duplex)
    {}
    void sendMessage(Cpp2Channel::SendCallback* callback,
                     std::unique_ptr<transport::THeaderBody> buf,
                     apache::thrift::transport::THeader* header) override {
      duplex_.lastSender_.set(Who::CLIENT);
      HeaderClientChannel::sendMessage(callback, std::move(buf), header);
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
    void sendMessage(Cpp2Channel::SendCallback* callback,
                     std::unique_ptr<transport::THeaderBody> buf,
                     apache::thrift::transport::THeader* header) override {
      duplex_.lastSender_.set(Who::SERVER);
      HeaderServerChannel::sendMessage(callback, std::move(buf), header);
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
        std::unique_ptr<FramingHandler> framingHandler,
        std::unique_ptr<ProtectionHandler> protectionHandler,
        std::unique_ptr<SaslNegotiationHandler> saslNegotiationHandler)
      : Cpp2Channel(transport,
                    std::move(framingHandler),
                    std::move(protectionHandler),
                    std::move(saslNegotiationHandler))
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

  class DuplexFramingHandler : public FramingHandler {
   public:
    explicit DuplexFramingHandler(DuplexChannel& duplex)
        : duplex_(duplex)
    {}

    std::tuple<std::unique_ptr<apache::thrift::transport::THeaderBody>,
               size_t,
               std::unique_ptr<apache::thrift::transport::THeader>>
    removeFrame(folly::IOBufQueue* q) override;

    std::unique_ptr<folly::IOBuf>
    addFrame(std::unique_ptr<apache::thrift::transport::THeaderBody> buf,
        apache::thrift::transport::THeader* header) override;
   private:
    DuplexChannel& duplex_;

    FramingHandler& getHandler(DuplexChannel::Who::WhoEnum who);
  };

  class DuplexProtectionHandler : public ProtectionHandler {
   public:
    explicit DuplexProtectionHandler(DuplexChannel& duplex)
        : ProtectionHandler()
        , duplex_(duplex)
    {}

    void protectionStateChanged() override {
      if (getProtectionState() != ProtectionState::VALID) {
        return;
      }

      CLIENT_TYPE srcClientType;
      std::bitset<CLIENT_TYPES_LEN> supportedClients;

      switch (duplex_.mainChannel_.get()) {
      case Who::CLIENT:
        srcClientType = duplex_.clientChannel_->getClientType();
        supportedClients[srcClientType] = true;
        duplex_.serverChannel_->setSupportedClients(&supportedClients);
        duplex_.serverChannel_->setClientType(srcClientType);
        break;
      case Who::SERVER:
        srcClientType = duplex_.serverChannel_->getClientType();
        supportedClients[srcClientType] = true;
        duplex_.clientChannel_->setSupportedClients(&supportedClients);
        duplex_.clientChannel_->setClientType(srcClientType);
        break;
      case Who::UNKNOWN:
        CHECK(false);
      }

      if (getContext() && !inputQueue_.empty()) {
        read(getContext(), inputQueue_);
      }
    }
   private:
    DuplexChannel& duplex_;
  };

  class DuplexSaslNegotiationHandler : public SaslNegotiationHandler {
   public:
    explicit DuplexSaslNegotiationHandler(DuplexChannel& duplex)
        : SaslNegotiationHandler()
        , duplex_(duplex)
    {}

    void read(Context* ctx,
              std::pair<std::unique_ptr<transport::THeaderBody>,
                        std::unique_ptr<apache::thrift::transport::THeader>> bufAndHeader) override {
      if (!serverHandler_) {
        initializeHandlers(*(duplex_.getServerChannel()));
      }

      switch (duplex_.mainChannel_.get()) {
      case Who::CLIENT:
        clientHandler_->read(ctx, std::move(bufAndHeader));
        break;
      case Who::SERVER:
        serverHandler_->read(ctx, std::move(bufAndHeader));
        break;
      case Who::UNKNOWN:
        CHECK(false);
      }
    }

    bool handleSecurityMessage(
        std::unique_ptr<apache::thrift::transport::THeaderBody>&& buf,
        std::unique_ptr<apache::thrift::transport::THeader>&& header) override {
      return false;
    }

    void initializeHandlers(HeaderServerChannel& serverChannel) {
      serverHandler_ = folly::make_unique<HeaderServerChannel::ServerSaslNegotiationHandler>(serverChannel);
      serverHandler_->setProtectionHandler(duplex_.getCpp2Channel()->getProtectionHandler());
      clientHandler_ = folly::make_unique<DummySaslNegotiationHandler>();
      clientHandler_->setProtectionHandler(duplex_.getCpp2Channel()->getProtectionHandler());
    }

   private:
    DuplexChannel& duplex_;
    std::unique_ptr<HeaderServerChannel::ServerSaslNegotiationHandler>
      serverHandler_{nullptr};
    std::unique_ptr<DummySaslNegotiationHandler> clientHandler_{nullptr};
  };
};

}} // apache::thrift
