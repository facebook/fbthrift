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
#include <thrift/lib/cpp2/security/packet_decryption_tool/PacketUtil.h>
#include <stdexcept>
#include <iostream>
#include <netinet/in.h>
#include <netinet/if_ether.h>

namespace apache { namespace thrift {

Packet::Packet(const struct pcap_pkthdr *hdr, const unsigned char *bytes)
  : endsAMessage_(false) {
  if (hdr->len != hdr->caplen) {
    throw std::runtime_error("Packet truncated. Run tcpdump again with "
                             "-s0 to capture full packet");
  }

  hdr_.reset(new pcap_pkthdr);
  memcpy(hdr_.get(), hdr, sizeof(struct pcap_pkthdr));
  data_.reset(new uint8_t[hdr_->caplen]);
  memcpy(data_.get(), bytes, hdr_->caplen);
  parse();
}

void Packet::parse() {
  // Ethernet header
  ether_ = reinterpret_cast<EthernetHeader*>(data_.get());
  uint16_t etherType = ntohs(ether_->type);
  if (etherType == ETHERTYPE_IP) {
    ipVersion_ = V4;
  } else if (etherType == ETHERTYPE_IPV6) {
    ipVersion_ = V6;
  } else {
    throw std::runtime_error("Not an IP frame");
  }

  // IP and TCP header
  if (ipVersion_ == V4) {
    IPv4Header *ipHeader = reinterpret_cast<IPv4Header*>(data_.get() +
        sizeof(EthernetHeader));
    if (ipHeader->protocol != IPPROTO_TCP) {
      throw std::runtime_error("Not a TCP packet");
    }
    ip_.v4 = ipHeader;
    size_t ipHeaderLen = 4 * (ip_.v4->versionAndHeaderLength & 0x0f);
    uint8_t *tcpStart = data_.get() + sizeof(EthernetHeader) + ipHeaderLen;
    tcp_ = reinterpret_cast<TCPHeader*>(tcpStart);

    struct sockaddr_in rawAddr;
    rawAddr.sin_family = AF_INET;
    rawAddr.sin_addr.s_addr = ipHeader->src;
    rawAddr.sin_port = tcp_->srcPort;
    srcAddr_.setFromSockaddr(&rawAddr);

    rawAddr.sin_addr.s_addr = ipHeader->dst;
    rawAddr.sin_port = tcp_->dstPort;
    dstAddr_.setFromSockaddr(&rawAddr);

    size_t tcpHeaderLen = 4 * (tcp_->offset >> 4);
    payload_ = tcpStart + tcpHeaderLen;
    payloadSize_ = hdr_->caplen - 14 - ipHeaderLen - tcpHeaderLen;
  } else {
    IPv6Header *ipHeader = reinterpret_cast<IPv6Header*>(data_.get() +
        sizeof(EthernetHeader));
    // TODO (haijunz): support extension headers
    if (ipHeader->nextHeader != 6) {
      throw std::runtime_error("Extension headers in an IPv6 frame");
    }
    ip_.v6 = ipHeader;
    uint8_t *tcpStart = data_.get() + sizeof(EthernetHeader) + 40;
    tcp_ = reinterpret_cast<TCPHeader*>(tcpStart);

    struct sockaddr_in6 rawAddr;
    rawAddr.sin6_family = AF_INET6;
    memcpy((char*)&rawAddr.sin6_addr, ipHeader->src, 16);
    rawAddr.sin6_port = tcp_->srcPort;
    srcAddr_.setFromSockaddr(&rawAddr);

    memcpy((char*)&rawAddr.sin6_addr, ipHeader->dst, 16);
    rawAddr.sin6_port = tcp_->dstPort;
    dstAddr_.setFromSockaddr(&rawAddr);

    size_t tcpHeaderLen = 4 * (tcp_->offset >> 4);
    payload_ = tcpStart + tcpHeaderLen;
    payloadSize_ = hdr_->caplen - 14 - 40 - tcpHeaderLen;
  }
}

void Packet::copyPayload(uint32_t size, const unsigned char *payload) {
  size_t headerSize = hdr_->caplen - payloadSize_;
  hdr_->caplen += (size - payloadSize_);
  uint8_t *newData = new uint8_t[hdr_->caplen];
  memcpy(newData, data_.get(), headerSize);
  memcpy(newData + headerSize, payload, size);
  data_.reset(newData);
}

Capturer::Capturer(const string& savefile, const string& dumpfile) {
  char errbuf[PCAP_ERRBUF_SIZE];
  pcap_ = pcap_open_offline(savefile.c_str(), errbuf);
  if (pcap_ == nullptr) {
    throw std::runtime_error("Cannot open pcap file");
  }

  pcap_dump_ = pcap_dump_open(pcap_, dumpfile.c_str());
  if (pcap_dump_ == nullptr) {
    throw std::runtime_error("Cannot open dump file for writing pcap");
  }
}

Capturer::~Capturer() {
  pcap_close(pcap_);
  pcap_dump_close(pcap_dump_);
}

void Capturer::writePacket(Packet* packet) {
  pcap_dump((unsigned char*)pcap_dump_,
            packet->hdr_.get(),
            packet->data_.get());
}

void Capturer::loop(pcap_handler handler, unsigned char *arg, int max) {
  int cnt = pcap_loop(pcap_, max, handler, arg);
  if (cnt == -1) {
    throw std::runtime_error("Error processing packet");
  } else if (cnt == -2) {
    throw std::runtime_error("Loop terminated when processing packet");
  }
}

}} //apache::thrift
