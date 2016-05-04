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

#include <thrift/lib/cpp2/async/PcapLoggingHandler.h>
#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>
#include <folly/FileUtil.h>
#include <folly/MPMCQueue.h>
#include <folly/FBVector.h>

#include <net/ethernet.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/tcp.h>

#include <time.h>
#include <stdlib.h>
#include <fcntl.h>

// For reallllly old systems, sigh
#ifndef O_CLOEXEC
#define O_CLOEXEC 02000000
#endif

#ifndef ETHERTYPE_IPV6
#define ETHERTYPE_IPV6 0x86dd
#endif

DEFINE_bool(thrift_pcap_logging_prohibit, false,
    "Don't allow pcap logging to be enabled");

namespace apache { namespace thrift {

using clock = std::chrono::system_clock;
using TAsyncSocket = async::TAsyncSocket;
using TAsyncSSLSocket = async::TAsyncSSLSocket;

namespace {

struct PcapFileHeader {
  uint32_t magic_number;   /* magic number */
  uint16_t version_major;  /* major version number */
  uint16_t version_minor;  /* minor version number */
  int32_t  thiszone;       /* GMT to local correction */
  uint32_t sigfigs;        /* accuracy of timestamps */
  uint32_t snaplen;        /* max length of captured packets, in octets */
  uint32_t network;        /* data link type */
};

struct PcapRecordHeader {
  uint32_t ts_sec;         /* timestamp seconds */
  uint32_t ts_usec;        /* timestamp microseconds */
  uint32_t incl_len;       /* number of octets of packet saved in file */
  uint32_t orig_len;       /* actual length of packet */
};

enum class Direction {READ, WRITE};

PcapRecordHeader generateRecordHeader(clock::time_point time,
    uint32_t wireLen, uint32_t capturedLen) {
  uint64_t us = std::chrono::duration_cast<std::chrono::microseconds>(
      time.time_since_epoch()).count();
  PcapRecordHeader recHeader = {
    static_cast<uint32_t>(us / 1000000),
    static_cast<uint32_t>(us % 1000000),
    static_cast<uint32_t>(capturedLen),
    static_cast<uint32_t>(wireLen)};

  return recHeader;
}

class Headers {
 public:
  enum TcpFlags {SYN, SYN_ACK, ACK, FIN, RST};
  Headers(const folly::SocketAddress& local,
      const folly::SocketAddress& remote,
      PcapLoggingHandler::Peer peer);
  void setTcpFlags(TcpFlags flags);
  void appendToIov(
      folly::fbvector<iovec>* iov,
      Direction dir,
      clock::time_point time,
      uint16_t dataLen = 0,
      uint16_t capturedLen = 0,
      PcapLoggingHandler::EncryptionType encryptionType =
          PcapLoggingHandler::EncryptionType::NONE);
 private:
  bool is6_;
  struct HeaderStruct {
    ether_header ether;
    union {
      iphdr ip4;
      ip6_hdr ip6;
    };
    tcphdr tcp;
  } read_, write_;
  PcapRecordHeader pcapHeader_;
  // Start seq nums with 0x100 because Wireshark 1.8.10 (and maybe others)
  // prints "TCP port numbers reused" when they start with 0.
  // Wireshark 2.0.0 doesn't have this bug.
  uint32_t tcpSeq_ = 0x100;
  uint32_t tcpAck_ = 0x100;
  void setTcpFlagsImpl(tcphdr* tcp, TcpFlags flags);
};

Headers::Headers(const folly::SocketAddress& local,
    const folly::SocketAddress& remote,
    PcapLoggingHandler::Peer peer) {
  using std::swap;
  DCHECK(local.getFamily() == remote.getFamily() &&
      (local.getFamily() == AF_INET || local.getFamily() == AF_INET6));

  bool client = peer == PcapLoggingHandler::CLIENT;
  write_.ether.ether_shost[0] = 0x06;
  memset(&write_.ether.ether_shost[1], client ? 0x11 : 0x22, 4);
  write_.ether.ether_dhost[0] = 0x06;
  memset(&write_.ether.ether_dhost[1], client ? 0x22 : 0x11, 4);

  memset(&write_.tcp, 0, sizeof(tcphdr));
  write_.tcp.source = htons(local.getPort());
  write_.tcp.dest = htons(remote.getPort());
  write_.tcp.doff = 5;
  write_.tcp.ack = 1;
  write_.tcp.window = htons(100);

  if (local.getFamily() == AF_INET) {
    is6_ = false;
    write_.ether.ether_type = htons(ETHERTYPE_IP);
    write_.ip4.version = 4;
    write_.ip4.ihl = 5;
    write_.ip4.tos = 0;
    write_.ip4.tot_len = 0;
    write_.ip4.id = 0;
    write_.ip4.frag_off = 0;
    write_.ip4.ttl = 16;
    write_.ip4.protocol = IPPROTO_TCP;
    write_.ip4.saddr = local.getIPAddress().asV4().toLong();
    write_.ip4.daddr = remote.getIPAddress().asV4().toLong();
    // HBO = host byte order
    uint32_t srcHBO = htonl(write_.ip4.saddr);
    uint32_t destHBO = htonl(write_.ip4.daddr);
    uint32_t checksum = 0x5506
      + (srcHBO >> 16) + (srcHBO & 0xffff)
      + (destHBO >> 16) + (destHBO & 0xffff);
    while ((checksum >> 16) != 0) {
      checksum = (checksum >> 16) + (checksum & 0xffff);
    }
    // checksum for a 0 length packet
    write_.ip4.check = ~htons(checksum);

    // The read_ header is identical to the write_ one, except the
    // src and dest addresses and ports. The IP checksum is the same too.
    read_ = write_;
    swap(read_.ip4.saddr, read_.ip4.daddr);
  } else {
    is6_ = true;
    write_.ether.ether_type = htons(ETHERTYPE_IPV6);
    write_.ip6.ip6_flow = htonl(0x60000000);
    write_.ip6.ip6_plen = 0;
    write_.ip6.ip6_nxt = IPPROTO_TCP;
    write_.ip6.ip6_hlim = 16;
    write_.ip6.ip6_src = local.getIPAddress().asV6().toAddr();
    write_.ip6.ip6_dst = remote.getIPAddress().asV6().toAddr();
    //
    // The read_ header is identical to the write_ one, except the
    // src and dest addresses and ports.
    read_ = write_;
    swap(read_.ip6.ip6_src, read_.ip6.ip6_dst);
  }

  swap(read_.tcp.source, read_.tcp.dest);
  swap(read_.ether.ether_shost, read_.ether.ether_dhost);
}

void Headers::setTcpFlagsImpl(tcphdr* tcp, Headers::TcpFlags flags) {
  tcp->syn = tcp->ack = tcp->fin = tcp->rst = 0;
  switch (flags) {
  case SYN: tcp->syn = 1; break;
  case ACK: tcp->ack = 1; break;
  case SYN_ACK: tcp->syn = tcp->ack = 1; break;
  case FIN: tcp->fin = tcp->ack = 1; break;
  case RST: tcp->rst = tcp->ack = 1; break;
  }
}

void Headers::setTcpFlags(TcpFlags flags) {
  setTcpFlagsImpl(&read_.tcp, flags);
  setTcpFlagsImpl(&write_.tcp, flags);
}

void Headers::appendToIov(
    folly::fbvector<iovec>* iov,
    Direction dir,
    clock::time_point time,
    uint16_t capturedLen,
    uint16_t dataLen,
    PcapLoggingHandler::EncryptionType encryptionType) {

  uint32_t ipsize = (is6_ ? sizeof(ip6_hdr) : sizeof(iphdr));
  uint32_t len = ipsize + sizeof(tcphdr) + dataLen;

  pcapHeader_ = generateRecordHeader(time,
      len + sizeof(ether_header),
      ipsize + sizeof(tcphdr) + capturedLen + sizeof(ether_header));
  iov->push_back({&pcapHeader_, sizeof(PcapRecordHeader)});

  auto h = dir == Direction::READ ? &read_ : &write_;

  h->ether.ether_shost[5] = h->ether.ether_dhost[5] = (uint8_t)encryptionType;
  iov->push_back({&h->ether, sizeof(ether_header)});

  if (is6_) {
    h->ip6.ip6_plen = htons(len);
  } else {
    uint32_t lendiff = len - htons(h->ip4.tot_len);
    h->ip4.tot_len = htons(len);
    uint32_t checksum = htons(~h->ip4.check) + lendiff;
    while ((checksum >> 16) != 0) {
      checksum = (checksum >> 16) + (checksum & 0xffff);
    }
    h->ip4.check = ~htons(checksum);
  }
  void* ip = is6_ ? static_cast<void*>(&h->ip6) : static_cast<void*>(&h->ip4);
  iov->push_back({ip, ipsize});

  // syn, fin take 1 byte in the seqid space
  uint32_t seqNumInc = dataLen == 0 && (h->tcp.syn || h->tcp.fin) ? 1 : dataLen;
  if (dir == Direction::WRITE) {
    h->tcp.seq = htonl(tcpSeq_);
    h->tcp.ack_seq = (h->tcp.syn && !h->tcp.ack) ? 0 : htonl(tcpAck_);
    tcpSeq_ += seqNumInc;
  } else {
    h->tcp.seq = htonl(tcpAck_);
    h->tcp.ack_seq = (h->tcp.syn && !h->tcp.ack) ? 0 : htonl(tcpSeq_);
    tcpAck_ += seqNumInc;
  }
  iov->push_back({&h->tcp, sizeof(tcphdr)});
}

class RotatingFile {
 public:
  explicit RotatingFile(const char* prefix, int rotateAfterMB)
    : prefix_(prefix)
    , rotateAfterMB_(rotateAfterMB)
  {}

  ~RotatingFile() {
    if (fd_ != -1) {
      close(fd_);
    }
  }

  void write(void* data, size_t count) {
    maybeRotate();
    ssize_t r = folly::writeFull(fd_, data, count);
    if (r != -1) {
      total_ += r;
    }
  }

  void writev(iovec* iov, int count) {
    maybeRotate();
    ssize_t r = folly::writevFull(fd_, iov, count);
    if (r != -1) {
      total_ += r;
    }
  }
 private:
  std::string prefix_;
  int rotateAfterMB_;
  int fd_ = -1;
  size_t total_ = 0;

  void maybeRotate() {
    if (rotateAfterMB_ > 0 && total_ / (1<<20) >= (size_t)rotateAfterMB_) {
      close(fd_);
      fd_ = -1;
    }
    if (fd_ == -1) {
      // should be big enough for strftime buffer and mkstemp
      constexpr size_t extra = 30;
      std::string filename;
      filename.reserve(prefix_.size() + extra);
      filename = prefix_;
      filename.resize(prefix_.size() + extra, '\0');
      {
        time_t now = time(nullptr);
        tm tmbuf;
        localtime_r(&now, &tmbuf);
        int len = strftime(&filename[prefix_.size()], extra,
            "_%Y%m%d-%H%M%S.XXXXXX", &tmbuf);
        filename.resize(prefix_.size() + len);
      }
      fd_ = Mkostemp(&filename[0], O_CLOEXEC);
      if (fd_ == -1) {
        // fd_ is -1 so all subsequent write operations will silently fail
        PLOG_EVERY_N(ERROR, 10000) <<
            "Can't create pcap file '" << filename << "'";
      }

      total_ = 0;
      static PcapFileHeader fileHeader = {0xa1b2c3d4, 2, 4, 0, 0, 65535, 1};
      write(&fileHeader, sizeof(fileHeader));
    }
  }

  static int Mkostemp(char* Template, int flags) {
    // mkostemp is only available with glibc >= 2.7
    // return mkostemp(Template, flags);
    int fd = mkstemp(Template);
    if (fd == -1) {
      return -1;
    }
    if (flags & O_CLOEXEC) {
      fcntl(fd, F_SETFD, FD_CLOEXEC);
    }
    return fd;
  }
};

class Message {
 public:
  enum class Type {CONN_OPEN, CONN_CLOSE, CONN_ERROR, DATA, SHUTDOWN, CONFIG};

  Message() noexcept
    : type(Type::SHUTDOWN)
  {}

  explicit Message(std::shared_ptr<const PcapLoggingConfig> config)
    : type(Type::CONFIG)
    , config(std::move(config))
  {}

  Message(
    clock::time_point time,
    Direction dir,
    const folly::SocketAddress& local,
    const folly::SocketAddress& remote,
    PcapLoggingHandler::Peer peer,
    folly::IOBufQueue&& buf,
    size_t origLength,
    PcapLoggingHandler::EncryptionType encryptionType)
    : type(Type::DATA)
    , time(time)
    , dir(dir)
    , local(local)
    , remote(remote)
    , peer(peer)
    , buf(std::move(buf))
    , origLength(origLength > 65000 ? 65000 : origLength)
    , encryptionType(encryptionType)
  {}

  Message(
    Type type,
    clock::time_point time,
    Direction dir,
    const folly::SocketAddress& local,
    const folly::SocketAddress& remote,
    PcapLoggingHandler::Peer peer)
    : type(type)
    , time(time)
    , dir(dir)
    , local(local)
    , remote(remote)
  , peer(peer)
  {}

  Type type;
  clock::time_point time;
  Direction dir;
  folly::SocketAddress local;
  folly::SocketAddress remote;
  PcapLoggingHandler::Peer peer;
  folly::IOBufQueue buf;
  size_t origLength;
  PcapLoggingHandler::EncryptionType encryptionType;
  std::shared_ptr<const PcapLoggingConfig> config;
};

class LoggingThread {
 public:
  static LoggingThread& get() {
   static LoggingThread t;
   return t;
  }

  // Does not block. Returns true if enqueued successfully.
  bool addMessage(Message&& msg) {
    return globalQueue_.write(std::move(msg));
  }

  void setConfig(std::shared_ptr<const PcapLoggingConfig> config) {
    globalQueue_.blockingWrite(Message(std::move(config)));
  }
 private:
  folly::MPMCQueue<Message> globalQueue_{4096};

  using ConnKey = std::pair<folly::SocketAddress, folly::SocketAddress>;
  struct PacketData {
    enum Type {START, DATA, END, ERROR};
    Type type;
    clock::time_point time;
    Direction dir;
    folly::IOBufQueue buf{};
    size_t origLength;
    PcapLoggingHandler::EncryptionType encryptionType;

    PacketData(Type type, clock::time_point time, Direction dir)
      : type(type)
      , time(time)
      , dir(dir)
    {}

    PacketData(Type type, clock::time_point time, Direction dir,
        folly::IOBufQueue&& buf, size_t origLength,
        PcapLoggingHandler::EncryptionType encryptionType)
    : type(type)
    , time(time)
    , dir(dir)
    , buf(std::move(buf))
    , origLength(origLength)
    , encryptionType(encryptionType)
  {}
  };
  struct ConnData {
    int initialPackets = 0;
    bool remoteClosed = false;
    PcapLoggingHandler::Peer peer;
    folly::Optional<Headers> headers;
    std::deque<PacketData> packets;
  };
  std::unordered_map<ConnKey, ConnData> localQueues_;

  folly::fbvector<iovec> iov_;

  bool enabled_ = false;
  int numMessagesConnStart_;
  int numMessagesConnEnd_;

  folly::Optional<RotatingFile> file_;

  // has to be declared last, so that all other members are initialized
  // before use by the thread
  std::thread thread_;

  LoggingThread()
    : thread_(std::thread([this](){ this->threadFunc(); }))
  {}

  ~LoggingThread() {
    globalQueue_.blockingWrite();
    thread_.join();
  }

  void threadFunc() {
    while (true) {
      Message msg;
      globalQueue_.blockingRead(msg);

      switch (msg.type) {
      case Message::Type::CONN_OPEN:
        if (enabled_) {
          ConnKey key = std::make_pair(msg.local, msg.remote);
          ConnData connData;
          connData.peer = msg.peer;
          if (numMessagesConnStart_ != 0) {
            if (numMessagesConnStart_ != -1) {
              connData.initialPackets++;
            }
            PacketData packet(PacketData::START, msg.time, Direction::WRITE);
            dumpPacket(key, connData, packet);
          }
          localQueues_[key] = std::move(connData);
        }
        break;
      case Message::Type::CONN_CLOSE:
        if (enabled_) {
          ConnKey key = std::make_pair(msg.local, msg.remote);
          auto iter = localQueues_.find(key);
          // close might be called multiple times, guard against that
          if (iter != localQueues_.end()) {
            ConnData& connData = iter->second;
            PacketData packet(PacketData::END, msg.time, msg.dir);
            dumpOrQueuePacket(key, connData, std::move(packet));
            if (msg.dir == Direction::WRITE) {
              dumpPackets(key, connData);
              localQueues_.erase(key);
            } else {
              connData.remoteClosed = true;
            }
          }
        }
        break;
      case Message::Type::CONN_ERROR:
        if (enabled_) {
          ConnKey key = std::make_pair(msg.local, msg.remote);
          auto iter = localQueues_.find(key);
          if (iter != localQueues_.end()) {
            ConnData& connData = iter->second;
            PacketData packet(PacketData::ERROR, msg.time, msg.dir);
            dumpOrQueuePacket(key, connData, std::move(packet));
          }
        }
        break;
      case Message::Type::DATA:
        if (enabled_) {
          ConnKey key = std::make_pair(msg.local, msg.remote);
          auto iter = localQueues_.find(key);
          if (iter != localQueues_.end()) {
            ConnData& connData = iter->second;
            PacketData packet(PacketData::DATA, msg.time, msg.dir,
              std::move(msg.buf), msg.origLength, msg.encryptionType);
            dumpOrQueuePacket(key, connData, std::move(packet));
          }
        }
        break;
      case Message::Type::SHUTDOWN:
        return;
      case Message::Type::CONFIG:
        setConfigPrivate(*msg.config);
        break;
      default:
        CHECK(false);
      }
    }
  }

  void setConfigPrivate(const PcapLoggingConfig& config) {
    if (config.enabled()) {
      file_.emplace(config.prefix().c_str(), config.rotateAfterMB());
      numMessagesConnStart_ = config.numMessagesConnStart();
      numMessagesConnEnd_ = config.numMessagesConnEnd();
      enabled_ = true;
    } else {
      file_ = folly::none;
      localQueues_.clear();
      enabled_ = false;
    }
  }

  void dumpPacket(const ConnKey& key,
      ConnData& connData,
      const PacketData& packet) {
    iov_.clear(); // preserves capacity so no reallocations on push_back
    if (!connData.headers.hasValue()) {
      connData.headers.emplace(key.first, key.second, connData.peer);
    }
    switch (packet.type) {
    case PacketData::DATA:
      connData.headers->appendToIov(&iov_,
          packet.dir,
          packet.time,
          packet.buf.chainLength(),
          packet.origLength,
          packet.encryptionType);
      packet.buf.front()->appendToIov(&iov_);
      file_->writev(iov_.data(), iov_.size());
      break;
    case PacketData::START: {
      bool client = connData.peer == PcapLoggingHandler::CLIENT;
      connData.headers->setTcpFlags(Headers::SYN);
      connData.headers->appendToIov(&iov_,
          client ? Direction::WRITE : Direction::READ,
          packet.time);
      file_->writev(iov_.data(), iov_.size());
      iov_.clear();
      connData.headers->setTcpFlags(Headers::SYN_ACK);
      connData.headers->appendToIov(&iov_,
          client? Direction::READ : Direction::WRITE,
          packet.time);
      file_->writev(iov_.data(), iov_.size());
      iov_.clear();
      connData.headers->setTcpFlags(Headers::ACK);
      connData.headers->appendToIov(&iov_,
          client ? Direction::WRITE : Direction::READ,
          packet.time);
      file_->writev(iov_.data(), iov_.size());
      break;
    }
    case PacketData::END:
      connData.headers->setTcpFlags(Headers::FIN);
      connData.headers->appendToIov(&iov_,
          packet.dir,
          packet.time);
      file_->writev(iov_.data(), iov_.size());
      if (packet.dir == Direction::WRITE && !connData.remoteClosed) {
        iov_.clear();
        connData.headers->appendToIov(&iov_,
            Direction::READ,
            packet.time);
        file_->writev(iov_.data(), iov_.size());
      } else {
        // Set flags back to ACK in case the remote end closed but we're going
        // to send more data.
        connData.headers->setTcpFlags(Headers::ACK);
      }
      break;
    case PacketData::ERROR:
      connData.headers->setTcpFlags(Headers::RST);
      connData.headers->appendToIov(&iov_,
          packet.dir,
          packet.time);
      file_->writev(iov_.data(), iov_.size());
      connData.headers->setTcpFlags(Headers::ACK);
      break;
    }
  }

  void dumpOrQueuePacket(const ConnKey& key,
      ConnData& connData,
      PacketData&& packet) {
    if (numMessagesConnStart_ == -1 ||
        connData.initialPackets < numMessagesConnStart_) {
      if (numMessagesConnStart_ != -1) {
        connData.initialPackets++;
      }
      dumpPacket(key, connData, packet);
    } else {
      if (numMessagesConnEnd_ > 0) {
        if (connData.packets.size() >= numMessagesConnEnd_) {
          connData.packets.pop_front();
        }
        connData.packets.push_back(std::move(packet));
      }
    }
  }

  void dumpPackets(const ConnKey& key, ConnData& connData) {
    if (numMessagesConnStart_ == -1 ||
        connData.initialPackets < numMessagesConnStart_) {
      return;
    }
    for (const auto& packet : connData.packets) {
      dumpPacket(key, connData, packet);
    }
  }
};

} // namespace

PcapLoggingHandler::PcapLoggingHandler(std::function<bool()> isKrbEncrypted)
  : isKrbEncrypted_(std::move(isKrbEncrypted))
{}

PcapLoggingHandler::EncryptionType PcapLoggingHandler::getEncryptionType() {
  if (ssl_.hasValue() && ssl_.value()) {
    return EncryptionType::SSL;
  }
  return isKrbEncrypted_() ? EncryptionType::KRB : EncryptionType::NONE;
}

void PcapLoggingHandler::transportActive(Context* ctx) {
  auto config = PcapLoggingConfig::get();

  if (!config->enabled()) {
    return;
  }

  if (config->sampleConnectionPct() != 100) {
    int rnd = folly::Random::rand32(100);
    if (rnd >= config->sampleConnectionPct()) {
      return;
    }
  }

  enabled_ = true;
  snaplen_ = config->snaplen();

  auto transport = ctx->getTransport();
  transport->getLocalAddress(&local_);
  transport->getPeerAddress(&remote_);

  peer_ = SERVER;
  if (auto sock = std::dynamic_pointer_cast<TAsyncSocket>(transport)) {
    if (!sock->isAccepted()) {
      peer_ = CLIENT;
    }
  }

  Message msg(Message::Type::CONN_OPEN, clock::now(), Direction::READ,
      local_, remote_, peer_);
  LoggingThread::get().addMessage(std::move(msg));
}

void PcapLoggingHandler::maybeCheckSsl(Context* ctx) {
  if (ssl_.hasValue()) {
    return;
  }
  if (auto sock = dynamic_cast<TAsyncSSLSocket*>(ctx->getTransport().get())) {
    ssl_ = sock->getSSLState() == TAsyncSSLSocket::STATE_ESTABLISHED;
  }
}

folly::Future<folly::Unit> PcapLoggingHandler::write(
    Context* ctx,
    std::unique_ptr<folly::IOBuf> buf) {

  if (enabled_) {
    maybeCheckSsl(ctx);
    folly::IOBufQueue q(folly::IOBufQueue::cacheChainLength());
    q.append(buf->clone());
    size_t origLength = q.chainLength();
    if (origLength > snaplen_) {
      q.trimEnd(origLength - snaplen_);
    }
    Message msg(clock::now(), Direction::WRITE, local_, remote_, peer_,
        std::move(q), origLength, getEncryptionType());
    LoggingThread::get().addMessage(std::move(msg));
  }

  return ctx->fireWrite(std::move(buf));
}

void PcapLoggingHandler::read(Context* ctx, folly::IOBufQueue& q) {
  if (enabled_) {
    maybeCheckSsl(ctx);
    folly::IOBufQueue copy(folly::IOBufQueue::cacheChainLength());
    copy.append(q.front()->clone());
    size_t origLength = copy.chainLength();
    if (origLength > snaplen_) {
      copy.trimEnd(origLength - snaplen_);
    }
    Message msg(clock::now(), Direction::READ, local_, remote_, peer_,
       std::move(copy), origLength, getEncryptionType());
    LoggingThread::get().addMessage(std::move(msg));
  }

  ctx->fireRead(q);
}

folly::Future<folly::Unit> PcapLoggingHandler::close(Context* ctx) {
  if (enabled_) {
    Message msg(Message::Type::CONN_CLOSE, clock::now(), Direction::WRITE,
        local_, remote_, peer_);
    LoggingThread::get().addMessage(std::move(msg));
  }

  return ctx->fireClose();
}

void PcapLoggingHandler::readEOF(Context* ctx) {
  if (enabled_) {
    Message msg(Message::Type::CONN_CLOSE, clock::now(), Direction::READ,
        local_, remote_, peer_);
    LoggingThread::get().addMessage(std::move(msg));
  }

  return ctx->fireReadEOF();
}

void PcapLoggingHandler::readException(Context* ctx,
    folly::exception_wrapper e) {
  if (enabled_) {
    Message msg(Message::Type::CONN_ERROR, clock::now(), Direction::READ,
        local_, remote_, peer_);
    LoggingThread::get().addMessage(std::move(msg));
  }

  return ctx->fireReadException(std::move(e));
}

folly::Future<folly::Unit> PcapLoggingHandler::writeException(Context* ctx,
    folly::exception_wrapper e) {
  if (enabled_) {
    Message msg(Message::Type::CONN_ERROR, clock::now(), Direction::WRITE,
        local_, remote_, peer_);
    LoggingThread::get().addMessage(std::move(msg));
  }

  return ctx->fireWriteException(std::move(e));
}

void PcapLoggingConfig::set(std::shared_ptr<const PcapLoggingConfig> config) {
  if (FLAGS_thrift_pcap_logging_prohibit) {
    return;
  }

  LoggingThread::get().setConfig(config);
  auto p = config_.try_get();
  if (p) {
    *p = *config;
  }
}

folly::Singleton<PcapLoggingConfig> PcapLoggingConfig::config_;

}} // namespace
