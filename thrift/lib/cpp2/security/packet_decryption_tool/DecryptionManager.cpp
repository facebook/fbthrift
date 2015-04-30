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
#include <thrift/lib/cpp2/security/packet_decryption_tool/DecryptionManager.h>
#include <folly/Memory.h>
#include <folly/io/Cursor.h>
#include <thrift/lib/cpp2/security/packet_decryption_tool/krb5_internal.h>
#include <thrift/lib/cpp2/protocol/MessageSerializer.h>
#include <thrift/lib/cpp2/gen-cpp2/Sasl_types.h>
#include <thrift/lib/cpp2/gen-cpp2/SaslAuthService.tcc>

namespace apache { namespace thrift {

using folly::IOBuf;
using apache::thrift::krb5::Krb5Principal;
using apache::thrift::krb5::Krb5Credentials;

DecryptionManager::DecryptionManager(
  const string& ccache,
  const string& serverKeytab,
  const string& pcapInput,
  const string& pcapOutput)
    : count_(0) {
  ctx_ = folly::make_unique<Krb5Context>();

  if (serverKeytab == "") {
    initCCache(ccache);
  } else {
    initServerKeytab(serverKeytab);
  }

  capturer_ = folly::make_unique<Capturer>(pcapInput, pcapOutput);
}

void DecryptionManager::initCCache(const string& ccache) {
  Krb5CCache file_cache = Krb5CCache::makeResolve(ccache);
  Krb5Principal client = file_cache.getClientPrincipal();
  ccache_ = folly::make_unique<Krb5CCache>(
    Krb5CCache::makeNewUnique("MEMORY"));
  ccache_->setDestroyOnClose();
  ccache_->initialize(client.get());
  krb5_error_code code = krb5_cc_copy_creds(
    ctx_->get(), file_cache.get(), ccache_->get());
  if (code) {
    throw std::runtime_error("Cannot copy ccache to memory");
  }
}

void DecryptionManager::initServerKeytab(const string& serverKeytab) {
  serverKeytab_ = folly::make_unique<Krb5Keytab>(ctx_->get(), serverKeytab);
}

void DecryptionManager::decrypt() {
  capturer_->loop(pcapCallback, reinterpret_cast<unsigned char*>(this));
}

void DecryptionManager::printSummary() {
  int success = 0;
  int fail = 0;
  int unfinished = 0;
  for (auto& handler : packetHandlerMap_) {
    if (handler.second->state_ == ENCRYPTED_MESSAGE) {
      if (FLAGS_verbose) {
        LOG(INFO) << "Handler for client " << handler.second->clientAddr_
                  << " and server " << handler.second->serverAddr_
                  << " succeeded.";
      }
      success++;
    } else if (handler.second->state_ == ERROR) {
      if (FLAGS_verbose) {
        LOG(INFO) << "Handler for client " << handler.second->clientAddr_
                  << " and server " << handler.second->serverAddr_
                  << " failed: " << handler.second->errorReason_;
      }
      fail++;
    } else {
      if (FLAGS_verbose) {
        LOG(INFO) << "Handler for client " << handler.second->clientAddr_
                  << " and server " << handler.second->serverAddr_
                  << " is in state " << handler.second->state_;
      }
      unfinished++;
    }
  }

  LOG(INFO) << count_ << " packets processed.";
  if (success != 0) {
    LOG(INFO) << success << " handlers succeeded.";
  }
  if (fail != 0) {
    LOG(INFO) << fail << " handlers failed.";
  }
  if (unfinished != 0) {
    LOG(INFO) << unfinished << " handlers haven't finished security "
              << "negotiation message parsing.";
  }
}

void DecryptionManager::handlePacket(const struct pcap_pkthdr *hdr,
                                     const unsigned char *bytes) {
  count_++;
  auto p = folly::make_unique<Packet>(hdr, bytes);

  auto it = packetHandlerMap_.find(std::make_pair(p->srcAddr_, p->dstAddr_));
  if (it == packetHandlerMap_.end()) {
    it = packetHandlerMap_.find(std::make_pair(p->dstAddr_, p->srcAddr_));
    if (it == packetHandlerMap_.end()) {
      // First packet of a connection, create a new packet handler
      auto packetHandler = std::make_shared<PacketHandler>(p->srcAddr_,
          p->dstAddr_, this);
      packetHandlerMap_[std::make_pair(p->srcAddr_, p->dstAddr_)] =
        packetHandler;
      if (FLAGS_verbose) {
        LOG(INFO) << "New packet handler created, src: " << p->srcAddr_ <<
          " dst: " << p->dstAddr_;
      }
      packetHandler->handle(std::move(p));
    } else {
      it->second->handle(std::move(p));
    }
  } else {
   it->second->handle(std::move(p));
  }
}

DecryptionManager::PacketHandler::PacketHandler(
    folly::SocketAddress srcAddr,
    folly::SocketAddress dstAddr,
    DecryptionManager *manager)
  : clientAddr_(srcAddr)
  , serverAddr_(dstAddr)
  , state_(NOT_STARTED)
  , errorReason_("")
  , decryptionManager_(manager)
  , header_(new apache::thrift::transport::THeader)
  , ctx_(folly::make_unique<Krb5Context>())
  , subkey_(nullptr)
  , sessionKey_(nullptr)
  , remaining_(0)
  , cipher_(folly::make_unique<IOBufQueue>(IOBufQueue::cacheChainLength())) {
}

DecryptionManager::PacketHandler::~PacketHandler() {
  if (subkey_) {
    krb5_free_keyblock(ctx_->get(), subkey_);
  }
  if (sessionKey_) {
    krb5_free_keyblock(ctx_->get(), sessionKey_);
  }
}

void DecryptionManager::PacketHandler::handle(unique_ptr<Packet> packet) {
  if (packet->payloadSize_ != 0) {
    // In case of error or TCP handshake messages, just write packets back
    switch(state_)
    {
      case NOT_STARTED:
        state_ = INIT_SEC_AP_REQ;
        parseFirstMessage(packet.get());
        break;
      case INIT_SEC_AP_REQ:
        if (packet->srcAddr_ != serverAddr_) {
          onError("Expecting server side mutual auth reply");
        } else {
          state_ = MUTUAL_AUTH_AP_REP;
          parseSecondMessage(packet.get());
        }
        break;
      case MUTUAL_AUTH_AP_REP:
        if (packet->srcAddr_ != clientAddr_) {
          onError("Expecting client side first empty token");
        } else {
          state_ = FIRST_EMPTY_TOKEN;
        }
        break;
      case FIRST_EMPTY_TOKEN:
        if (packet->srcAddr_ != serverAddr_) {
          onError("Expecting server side security layer message");
        } else {
          state_ = SECURITY_LAYER_MESSAGE_SERVER;
        }
        break;
      case SECURITY_LAYER_MESSAGE_SERVER:
        if (packet->srcAddr_ != clientAddr_) {
          onError("Expecting client side security layer message");
        } else {
          state_ = SECURITY_LAYER_MESSAGE_CLIENT;
        }
        break;
      case SECURITY_LAYER_MESSAGE_CLIENT:
        if (packet->srcAddr_ != serverAddr_) {
          onError("Expecting server side second empty token");
        } else {
          state_ = SECOND_EMPTY_TOKEN;
        }
        break;
      case SECOND_EMPTY_TOKEN:
        if (packet->srcAddr_ != clientAddr_) {
          onError("Expecting first encrypted application message");
        } else {
          state_ = ENCRYPTED_MESSAGE;
          handleApplicationMessage(std::move(packet));
          return;
        }
        break;
      case ENCRYPTED_MESSAGE:
        handleApplicationMessage(std::move(packet));
        return;
      default:
        onError("Bug: wrong state");
    }
  }

  decryptionManager_->writePacket(std::move(packet));
}

void DecryptionManager::PacketHandler::parseSecondMessage(Packet* packet) {
  // Parse input token from thrift response.
  unique_ptr<IOBuf> msg = removeThriftHeader(packet);
  if (!msg) {
    return;
  }

  apache::thrift::sasl::SaslReply saslReply;
  apache::thrift::sasl::SaslAuthService_authFirstRequest_presult presult;
  std::get<0>(presult.fields).value = &saslReply;

  std::string methodName;
  try {
   methodName = PargsPresultProtoDeserialize(
        header_->getProtocolId(),
        presult,
        msg.get(),
        T_REPLY).first;
  } catch (const TProtocolException& e) {
    if (header_->getProtocolId() == protocol::T_BINARY_PROTOCOL &&
        e.getType() == TProtocolException::BAD_VERSION) {
      methodName = PargsPresultProtoDeserialize(
          protocol::T_COMPACT_PROTOCOL,
          presult,
          msg.get(),
          T_REPLY).first;
    } else {
      onError("Cannot parse second thrift security negotiation message");
      return;
    }
  }

  if (methodName != "authFirstRequest") {
    onError("Bad auth first reply");
    return;
  }
  std::string authRep = saslReply.challenge;

  // Parse encrypted APP_REP from input token. This is mostly
  // copied from init_sec_context.c.
  gss_buffer_desc inputToken;
  inputToken.length = authRep.length();
  inputToken.value = (void*)authRep.data();
  krb5_data ap_rep;
  unsigned char *ptr;
  char *sptr;
  krb5_ap_rep *reply = nullptr;
  krb5_ap_rep_enc_part *enc = nullptr;
  krb5_error_code code;

  ptr = (unsigned char *) inputToken.value;
  code = gssint_g_verify_token_header(gss_mech_krb5, //always use krb5
                                      &(ap_rep.length),
                                      &ptr,
                                      KG_TOK_CTX_AP_REP,
                                      inputToken.length, 1);

  if (code) {
    onError("Failed to verify ap_rep input token header");
    return;
  }

  sptr = (char*)ptr;
  TREAD_STR(sptr, ap_rep.data, ap_rep.length);

  code = decode_krb5_ap_rep(&ap_rep, &reply);
  if (code) {
    onError("Cannot decode ap_rep");
    return;
  }

  SCOPE_EXIT {
    krb5_free_ap_rep(ctx_->get(), reply);
  };

  krb5_data scratch;
  scratch.length = reply->enc_part.ciphertext.length;
  scratch.data = (char*)malloc(scratch.length);
  if (!scratch.data) {
    throw std::runtime_error("Cannot allocate memory");
  }

  SCOPE_EXIT {
    memset(scratch.data, 0, scratch.length);
    free(scratch.data);
  };

  code = krb5_c_decrypt(ctx_->get(),
                        sessionKey_,
                        KRB5_KEYUSAGE_AP_REP_ENCPART,
                        0,
                        &reply->enc_part,
                        &scratch);
  if (code) {
    onError("Cannot decrypt ap_rep");
    return;
  }

  code = decode_krb5_ap_rep_enc_part(&scratch, &enc);
  if (code) {
    onError("Cannot decode ap_rep's encrypted part");
    return;
  }

  SCOPE_EXIT {
    krb5_free_ap_rep_enc_part(ctx_->get(), enc);
  };

  // If subkey is present in ap_rep, it is used for encrypt/decrypt
  // application messages.
  if (enc->subkey) {
    krb5_copy_keyblock(ctx_->get(), enc->subkey, &subkey_);
    krb5_free_keyblock(ctx_->get(), sessionKey_);
    sessionKey_ = nullptr;
  }
}

unique_ptr<IOBuf>
DecryptionManager::PacketHandler::removeThriftHeader(Packet* packet) {
  unique_ptr<IOBufQueue> queue = folly::make_unique<IOBufQueue>();
  queue->wrapBuffer(packet->payload_, packet->payloadSize_);

  unique_ptr<IOBuf> msg;
  size_t remaining;
  try {
    std::map<std::string, std::string> persistentHeaders;
    msg = header_->removeHeader(queue.get(), remaining, persistentHeaders);
    if (remaining != 0) {
      // Assume security message can always live in one packet
      onError("Corrupted security message");
      return nullptr;
    }
    return std::move(msg);
  } catch (...) {
    onError("Failed to remove thrift header. "
            "Maybe not secure thrift connection?");
    return nullptr;
  }
}

void DecryptionManager::PacketHandler::parseFirstMessage(Packet* packet) {
  // Parse input token from thrift request.
  unique_ptr<IOBuf> msg = removeThriftHeader(packet);
  if (!msg) {
    return;
  }

  apache::thrift::sasl::SaslStart saslStart;
  apache::thrift::sasl::SaslAuthService_authFirstRequest_pargs pargs;
  pargs.saslStart = &saslStart;

  try {
    std::string methodName;
    try {
     methodName = PargsPresultProtoDeserialize(
          header_->getProtocolId(),
          pargs,
          msg.get(),
          T_CALL).first;
    } catch (const TProtocolException& e) {
      if (header_->getProtocolId() == protocol::T_BINARY_PROTOCOL &&
          e.getType() == TProtocolException::BAD_VERSION) {
        methodName = PargsPresultProtoDeserialize(
            protocol::T_COMPACT_PROTOCOL,
            pargs,
            msg.get(),
            T_CALL).first;
      } else {
        onError("Protocol mismatch parsing first thrift security message");
        return;
      }
    }
    if (methodName != "authFirstRequest") {
      onError("Bad first auth request. Maybe not secure thrift connection?");
      return;
    }
  } catch(...) {
    onError("Caught exception parsing first thrift security message");
  }

  std::string authReq = saslStart.request.response;

  // Parse encrypted authenticator from input token. This is mostly
  // copied from accept_sec_context.c.
  gss_buffer_desc inputToken;
  inputToken.length = authReq.length();
  inputToken.value = (void*)authReq.data();
  krb5_data ap_req;
  krb5_error_code code;
  unsigned char *ptr;
  char *sptr;
  krb5_ap_req *request = nullptr;

  ptr = (unsigned char*) inputToken.value;
  code = gssint_g_verify_token_header(gss_mech_krb5, // always use krb5
                                      &(ap_req.length),
                                      &ptr,
                                      KG_TOK_CTX_AP_REQ,
                                      inputToken.length, 1);
  if (code) {
    onError("Failed to verify ap_req input token header");
    return;
  }


  sptr = (char*) ptr;
  TREAD_STR(sptr, ap_req.data, ap_req.length);

  code = decode_krb5_ap_req(&ap_req, &request);
  if (code) {
    onError("Cannot decode ap_req");
    return;
  }

  readServiceSessionKey(request->ticket);
  if (!sessionKey_ || sessionKey_->length == 0) {
    onError("Failed to read session key");
    return;
  }

  krb5_data scratch;
  scratch.length = request->authenticator.ciphertext.length;
  scratch.data = (char*)malloc(scratch.length);
  if (!scratch.data) {
    throw std::runtime_error("Cannot allocate memory");
  }

  SCOPE_EXIT {
    memset(scratch.data, 0, scratch.length);
    free(scratch.data);
  };

  code = krb5_c_decrypt(ctx_->get(),
                        sessionKey_,
                        KRB5_KEYUSAGE_AP_REQ_AUTH,
                        0,
                        &request->authenticator,
                        &scratch);
  if (code) {
    onError("Cannot decrypt authenticator");
    return;
  }

  krb5_authenticator *authenticator;
  code = decode_krb5_authenticator(&scratch, &authenticator);
  if (code) {
    onError("Cannot decode authenticator");
    return;
  }

  SCOPE_EXIT {
    free(authenticator);
  };

  // Store this subsession key. In the current thrift security
  // implementation this subkey is always present and server
  // uses it to generate another subkey, which will be used for
  // encrypt/decrypt application messages.
  if (authenticator->subkey) {
    krb5_copy_keyblock(ctx_->get(), authenticator->subkey, &subkey_);
  }
}

void DecryptionManager::PacketHandler::handleApplicationMessage(
    unique_ptr<Packet> packet) {
  // Splits framed encrypted messages and decrypts them.
  //
  // This assumes a thrift message can start in the middle of a tcp
  // packet, which means tcp will accumulate data before sending.
  // This should happen rarely because TAsyncSocket sets TCP_NODELAY
  // by default so this can only happen if TCP_NODELAY is disabled
  // or TCP_CORK is enabled.

  int consumed = 0;
  bool fromClient = (packet->srcAddr_ == clientAddr_);

  if (remaining_ != 0) {
    if (remaining_ <= packet->payloadSize_) {
      cipher_->append(IOBuf::wrapBuffer(packet->payload_, remaining_));
      consumed = remaining_;
      if (UNLIKELY(cipher_->chainLength() == 4)) {
        unique_ptr<IOBuf> length = cipher_->split(4);
        Cursor c(length.get());
        remaining_ = c.readBE<uint32_t>();
        cipher_->append(std::move(length));
        if (remaining_ <= packet->payloadSize_ - consumed) {
          cipher_->append(IOBuf::wrapBuffer(packet->payload_ + consumed,
                                            remaining_));
          decryptApplicationMessage(fromClient);
          consumed += remaining_;
          remaining_ = 0;
        } else {
          cipher_->append(IOBuf::wrapBuffer(packet->payload_ + consumed,
                                            packet->payloadSize_ - consumed));
          remaining_ -= (packet->payloadSize_ - consumed);
          maybeWritePacket(std::move(packet));
          return;
        }
      } else {
        decryptApplicationMessage(fromClient);
        remaining_ = 0;
      }
    } else {
      cipher_->append(IOBuf::wrapBuffer(packet->payload_,
                                        packet->payloadSize_));
      remaining_ -= packet->payloadSize_;
      maybeWritePacket(std::move(packet));
      return;
    }
  }

  while (consumed < packet->payloadSize_) {
    if (UNLIKELY(packet->payloadSize_ - consumed < 4)) {
      // The length field crossed packet boundary, weird.
      cipher_->append(IOBuf::wrapBuffer(packet->payload_ + consumed,
                                        packet->payloadSize_ - consumed));
      remaining_ = 4 - (packet->payloadSize_ - consumed);
      break;
    } else {
      auto buf = IOBuf::wrapBuffer(packet->payload_ + consumed,
                                   packet->payloadSize_ - consumed);
      Cursor c(buf.get());
      uint32_t sz = c.readBE<uint32_t>();
      if (sz <= packet->payloadSize_ - consumed - 4) {
        cipher_->append(IOBuf::wrapBuffer(packet->payload_ + consumed,
                                          sz + 4));
        decryptApplicationMessage(fromClient);
        remaining_ = 0;
        consumed += (sz + 4);
      } else {
        cipher_->append(std::move(buf));
        remaining_ = sz - (packet->payloadSize_ - consumed - 4);
        break;
      }
    }
  }

  packet->endsAMessage_ = (remaining_ == 0);
  maybeWritePacket(std::move(packet));
}

void
DecryptionManager::PacketHandler::decryptApplicationMessage(bool fromClient) {
  // Split and coalesce the cipher data into a continuous buffer
  unique_ptr<IOBuf> msg = cipher_->split(cipher_->chainLength());
  msg->coalesce();

  // Start decryption. This is mostly copied from k5sealv3.c.
  krb5_error_code code;
  unsigned char *ptr;
  unsigned int bodysize;

  ptr = (unsigned char *)msg->data() + 4;
  code = gssint_g_verify_token_header(gss_mech_krb5, // always use krb5
                                      &bodysize,
                                      &ptr,
                                      -1,
                                      msg->length() - 4,
                                      0);

  if (code) {
    onError("Failed to verify application message header");
    return;
  }

  size_t ec = load_16_be(ptr+4);
  size_t rrc = load_16_be(ptr+6);
  if (!gss_krb5int_rotate_left(ptr+16, bodysize-16, rrc)) {
    throw std::runtime_error("Cannot allocate memory");
  }

  krb5_enc_data cipher;
  krb5_data plain;
  int keyUsage;
  cipher.enctype = subkey_->enctype;
  cipher.ciphertext.length = bodysize - 16;
  cipher.ciphertext.data = (char*)ptr + 16;
  plain.length = bodysize - 16;
  plain.data = (char*)malloc(plain.length);
  if (!plain.data) {
    throw std::runtime_error("Cannot allocate memory");
  }

  SCOPE_EXIT {
    free(plain.data);
  };

  keyUsage = (fromClient ? KG_USAGE_INITIATOR_SEAL : KG_USAGE_ACCEPTOR_SEAL);
  code = krb5_c_decrypt(ctx_->get(), subkey_, keyUsage, 0, &cipher, &plain);
  if (code) {
    onError("Cannot decrypt application message");
    return;
  }

  decryptedMessages_.push_back(IOBuf::copyBuffer(plain.data,
                                                 plain.length - ec - 16));
}

void
DecryptionManager::PacketHandler::maybeWritePacket(unique_ptr<Packet> packet) {
  unwrittenPackets_.push_back(std::move(packet));

  // Write all packets and decrypted messages if the latest packet
  // ends a message. This ensures packets and decrypted messages won't
  // be out of sync.
  if (unwrittenPackets_.back()->endsAMessage_) {
    IOBufQueue q(IOBufQueue::cacheChainLength());
    for (auto& pkt : unwrittenPackets_) {
      while (q.chainLength() < pkt->payloadSize_ &&
             !decryptedMessages_.empty()) {
        q.append(std::move(decryptedMessages_.front()));
        decryptedMessages_.pop_front();
      }

      unique_ptr<IOBuf> payload =
        (q.chainLength() >= pkt->payloadSize_ ?
         q.split(pkt->payloadSize_) :
         q.split(q.chainLength()));

      payload->coalesce();
      pkt->copyPayload(payload->length(), payload->data());
      decryptionManager_->writePacket(std::move(pkt));
    }
    unwrittenPackets_.clear();
  }
}

// Get session key from ccache or service ticket.
void DecryptionManager::PacketHandler::readServiceSessionKey(
    krb5_ticket* ticket) {
  Krb5CCache *ccache = decryptionManager_->getCCache();
  if (ccache) {
    Krb5Credentials creds = ccache->retrieveCred(ticket->server);
    krb5_copy_keyblock(ctx_->get(), &creds.get().keyblock, &sessionKey_);
  } else {
    krb5_error_code code = krb5_server_decrypt_ticket_keytab(
        ctx_->get(),
        decryptionManager_->getServerKeytab()->get(),
        ticket);
    if (code) {
      onError("Cannot decrypt service ticket using the given server keytab");
      return;
    }

    krb5_copy_keyblock(ctx_->get(),
                       ticket->enc_part2->session,
                       &sessionKey_);
  }
}

void DecryptionManager::PacketHandler::onError(const string& msg) {
  if (FLAGS_verbose) {
    LOG(ERROR) << msg;
  }
  errorReason_ = msg;
  state_ = ERROR;
}

void DecryptionManager::writePacket(unique_ptr<Packet> packet) {
  capturer_->writePacket(packet.get());
}

void DecryptionManager::pcapCallback(unsigned char *arg,
                                    const struct pcap_pkthdr *hdr,
                                    const unsigned char *bytes) {
  DecryptionManager *manager = reinterpret_cast<DecryptionManager*>(arg);
  manager->handlePacket(hdr, bytes);
}

}} // apache::thrift
