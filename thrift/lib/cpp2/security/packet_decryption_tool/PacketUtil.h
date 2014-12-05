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

#include <string>
#include <pcap.h>
#include <memory>
#include <folly/io/async/AsyncServerSocket.h>

namespace apache { namespace thrift {

using std::string;
using std::unique_ptr;

struct __attribute__((__packed__)) EthernetHeader {
  uint8_t dst[6];
  uint8_t src[6];
  uint16_t type;
};

struct __attribute__((__packed__)) IPv4Header {
  uint8_t versionAndHeaderLength;
  uint8_t dscpAndEcn;
  uint16_t totalLen;
  uint16_t identification;
  uint16_t flagsAndFramentOffset;
  uint8_t ttl;
  uint8_t protocol;
  uint16_t cksum;
  uint32_t src;
  uint32_t dst;
};

struct __attribute__((__packed__)) IPv6Header {
  uint32_t versionAndTrafficClassAndFlowLabel;
  uint16_t payloadLength;
  uint8_t nextHeader;
  uint8_t hopLimit;
  uint8_t dst[16];
  uint8_t src[16];
};

struct __attribute__((__packed__)) TCPHeader {
  uint16_t srcPort;
  uint16_t dstPort;
  uint32_t seq;
  uint32_t ack;
  uint8_t offset;
  uint8_t flags;
  uint16_t windowSize;
  uint16_t cksum;
  uint16_t urg;
};

enum IPVersion { V4, V6 };

union IPHeader {
  IPv4Header *v4;
  IPv6Header *v6;
};

class Packet {
  public:
    Packet(const struct pcap_pkthdr *hdr, const unsigned char *bytes);
    //Only copy the payload part, headers remain unchanged
    void copyPayload(uint32_t size, const unsigned char *payload);

    unique_ptr<struct pcap_pkthdr> hdr_;
    unique_ptr<uint8_t[]> data_;
    EthernetHeader *ether_;
    IPHeader ip_;
    IPVersion ipVersion_;
    TCPHeader *tcp_;
    folly::SocketAddress srcAddr_;
    folly::SocketAddress dstAddr_;
    uint8_t *payload_;
    uint32_t payloadSize_;
    bool endsAMessage_;

  private:
    void parse();
};

class Capturer {
  public:
    explicit Capturer(const string& savefile, const string& dumpfile);
    ~Capturer();

    void writePacket(Packet* packet);
    void loop(pcap_handler handler, unsigned char *arg, int max=0);

  private:
    pcap_t* pcap_;
    pcap_dumper_t* pcap_dump_;
};

}} // apache::thrift
