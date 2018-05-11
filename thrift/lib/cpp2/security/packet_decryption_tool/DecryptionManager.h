/*
 * Copyright 2014-present Facebook, Inc.
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

#include <map>
#include <memory>
#include <utility>

#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp/transport/THeader.h>
#include <thrift/lib/cpp2/security/packet_decryption_tool/PacketUtil.h>
#include <thrift/lib/cpp2/util/kerberos/Krb5Util.h>

DECLARE_bool(verbose);

namespace apache { namespace thrift {

using std::unique_ptr;
using folly::IOBuf;
using folly::IOBufQueue;
using apache::thrift::krb5::Krb5CCache;
using apache::thrift::krb5::Krb5Context;
using apache::thrift::krb5::Krb5Keytab;

class DecryptionManager {
  public:
    DecryptionManager(const string& ccache,
                      const string& serverKeytab,
                      const string& pcapInput,
                      const string& pcapOutput);
    ~DecryptionManager() {}
    Krb5CCache* getCCache() { return ccache_.get(); }
    Krb5Keytab* getServerKeytab() { return serverKeytab_.get(); }
    void printSummary();
    void handlePacket(const struct pcap_pkthdr *hdr,
                      const unsigned char *bytes);
    void decrypt();

    enum SecurityState {
      NOT_STARTED,
      INIT_SEC_AP_REQ,
      MUTUAL_AUTH_AP_REP,
      FIRST_EMPTY_TOKEN,
      SECURITY_LAYER_MESSAGE_SERVER,
      SECURITY_LAYER_MESSAGE_CLIENT,
      SECOND_EMPTY_TOKEN,
      ENCRYPTED_MESSAGE,
      ERROR
    };

    // Handle packets for a source/destination pair.
    class PacketHandler {
      public:
        PacketHandler(folly::SocketAddress srcAddr,
            folly::SocketAddress dstAddr,
            DecryptionManager *manager);
        ~PacketHandler();

        void handle(unique_ptr<Packet> packet);

        folly::SocketAddress clientAddr_;
        folly::SocketAddress serverAddr_;
        SecurityState state_;
        string errorReason_;

      private:
        void parseFirstMessage(Packet* packet);
        void parseSecondMessage(Packet* packet);
        void readServiceSessionKey(krb5_ticket* ticket);
        unique_ptr<IOBuf> removeThriftHeader(Packet* packet);
        void handleApplicationMessage(unique_ptr<Packet> packet);
        void decryptApplicationMessage(bool fromClient);
        void maybeWritePacket(unique_ptr<Packet> packet);
        void onError(const string& msg);

        DecryptionManager *decryptionManager_;
        unique_ptr<apache::thrift::transport::THeader> header_;
        unique_ptr<Krb5Context> ctx_;
        krb5_keyblock *subkey_;
        krb5_keyblock *sessionKey_;
        uint32_t remaining_;
        unique_ptr<IOBufQueue> cipher_;
        std::vector<unique_ptr<Packet>> unwrittenPackets_;
        std::deque<unique_ptr<IOBuf>> decryptedMessages_;
    };

  private:
    void initCCache(const string& ccacheFile);
    void initServerKeytab(const string& serverKeytab);
    void writePacket(unique_ptr<Packet> packet);
    static void pcapCallback(unsigned char *arg, const struct pcap_pkthdr *hdr,
                      const unsigned char *bytes);

    unique_ptr<Krb5Context> ctx_;
    unique_ptr<Krb5CCache> ccache_;
    unique_ptr<Krb5Keytab> serverKeytab_;
    unique_ptr<Capturer> capturer_;
    typedef std::unordered_map<
      std::pair<folly::SocketAddress, folly::SocketAddress>,
      std::shared_ptr<PacketHandler>> PacketHandlerMap;
    PacketHandlerMap packetHandlerMap_;
    int32_t count_;
};

}} // apache::thrift
