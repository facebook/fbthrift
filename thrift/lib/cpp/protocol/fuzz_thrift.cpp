#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <utility>

#include <thrift/lib/cpp/protocol/TBinaryProtocol.h>
#include <thrift/lib/cpp/protocol/TCompactProtocol.h>
#include <thrift/lib/cpp/protocol/TProtocol.h>
#include <thrift/lib/cpp/transport/TTransport.h>

namespace apache::thrift::protocol {

[[noreturn]] void TProtocolException::throwInvalidSkipType(TType /*type*/) { throw TProtocolException(TProtocolException::INVALID_DATA, "invalid skip type while fuzzing protocol input"); }
[[noreturn]] void TProtocolException::throwExceededSizeLimit(size_t /*size*/, size_t /*limit*/) { throw TProtocolException(TProtocolException::SIZE_LIMIT, "size limit exceeded while fuzzing protocol input"); }

} // namespace apache::thrift::protocol

namespace apache::thrift::transport {
std::string TTransportException::getDefaultMessage(
    TTransportExceptionType /*type*/, std::string&& message) {
  return std::move(message);
}
} // namespace apache::thrift::transport

namespace {
constexpr uint32_t kMaxFields = 256;
constexpr int32_t kStringSizeLimit = 1 << 20;
constexpr int32_t kContainerSizeLimit = 1 << 16;

class FuzzMemoryTransport : public apache::thrift::transport::TTransport {
 public:
  FuzzMemoryTransport(const uint8_t* data, size_t size) : data_(data), size_(size), offset_(0) {}

  uint32_t read_virt(uint8_t* out, uint32_t len) override {
    const size_t remaining = size_ - offset_;
    if (remaining == 0) return 0;
    const uint32_t toCopy = static_cast<uint32_t>(std::min<size_t>(len, remaining));
    if (toCopy != 0) {
      std::memcpy(out, data_ + offset_, toCopy);
      offset_ += toCopy;
    }
    return toCopy;
  }

  uint32_t readAll_virt(uint8_t* out, uint32_t len) override {
    uint32_t copied = 0;
    while (copied < len) {
      const uint32_t n = read_virt(out + copied, len - copied);
      if (n == 0) throw std::runtime_error("truncated input");
      copied += n;
    }
    return copied;
  }

  void write_virt(const uint8_t* /*buf*/, uint32_t /*len*/) override {}

  const uint8_t* borrow_virt(uint8_t* /*buf*/, uint32_t* len) override {
    if (len == nullptr) return nullptr;
    const size_t remaining = size_ - offset_;
    if (remaining < *len) return nullptr;
    *len = static_cast<uint32_t>(remaining);
    return data_ + offset_;
  }

  void consume_virt(uint32_t len) override {
    const size_t remaining = size_ - offset_;
    if (len > remaining) throw std::runtime_error("consume past end");
    offset_ += len;
  }

 private:
  const uint8_t* data_;
  size_t size_;
  size_t offset_;
};

template <typename Protocol>
void parseStruct(Protocol& p) {
  std::string name;
  p.readStructBegin(name);
  for (uint32_t i = 0; i < kMaxFields; ++i) {
    apache::thrift::protocol::TType fieldType = apache::thrift::protocol::T_STOP;
    int16_t fieldId = 0;
    p.readFieldBegin(name, fieldType, fieldId);
    if (fieldType == apache::thrift::protocol::T_STOP) break;
    apache::thrift::protocol::skip(p, fieldType, 0);
    p.readFieldEnd();
  }
  p.readStructEnd();
}

template <typename Protocol>
void fuzzProtocol(const uint8_t* data, size_t size, bool parseAsMessage) {
  FuzzMemoryTransport transport(data, size);
  Protocol p(&transport);
  p.setStringSizeLimit(kStringSizeLimit);
  p.setContainerSizeLimit(kContainerSizeLimit);
  if (parseAsMessage) {
    std::string name;
    apache::thrift::protocol::TMessageType messageType = apache::thrift::protocol::T_CALL;
    int32_t seqId = 0;
    p.readMessageBegin(name, messageType, seqId);
    parseStruct(p);
    p.readMessageEnd();
    return;
  }
  parseStruct(p);
}

template <typename Protocol>
void runOne(const uint8_t* data, size_t size, bool parseAsMessage) {
  try {
    fuzzProtocol<Protocol>(data, size, parseAsMessage);
  } catch (...) {}
}

} // namespace

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size) {
  if (size <= 1) return 0;
  const uint8_t mode = data[0];
  const uint8_t* payload = data + 1;
  const size_t payloadSize = size - 1;
  runOne<apache::thrift::protocol::TBinaryProtocolT<FuzzMemoryTransport>>(payload, payloadSize, (mode & 0x1u) != 0);
  runOne<apache::thrift::protocol::TCompactProtocolT<FuzzMemoryTransport>>(payload, payloadSize, (mode & 0x2u) != 0);
  return 0;
}
