/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/util/DebugString.h>

#include <cassert>
#include <optional>
#include <utility>
#include <vector>

#include <fmt/core.h>
#include <folly/String.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Ops.h>

namespace apache::thrift {
namespace {

using apache::thrift::protocol::TType;

const char* typeLabel(TType type) {
  // NOLINTNEXTLINE(clang-diagnostic-switch-enum)
  switch (type) {
    case TType::T_STOP:
      return "stop";
    case TType::T_BOOL:
      return "bool";
    case TType::T_BYTE:
      return "byte";
    case TType::T_I16:
      return "i16";
    case TType::T_I32:
      return "i32";
    case TType::T_I64:
      return "i64";
    case TType::T_DOUBLE:
      return "double";
    case TType::T_FLOAT:
      return "float";
    case TType::T_STRING:
      return "string";
    case TType::T_UTF8:
      return "utf8";
    case TType::T_STRUCT:
      return "struct";
    case TType::T_LIST:
      return "list";
    case TType::T_SET:
      return "set";
    case TType::T_MAP:
      return "map";
    default:
      return "";
  }
}

std::string lookupTypeStringIfEmpty(std::string typeStr, TType type) {
  return typeStr.empty() ? typeLabel(type) : std::move(typeStr);
}

class DebugStringProtocolWriter {
 public:
  using ProtocolReader = BinaryProtocolReader;

  explicit DebugStringProtocolWriter(
      ExternalBufferSharing /* sharing */ = COPY_EXTERNAL_BUFFER) {}

  static constexpr bool kSupportsArithmeticVectors() { return false; }

  void setOutput(folly::io::QueueAppender&& output) {
    output_ = std::move(output);
  }

  uint32_t writeStructBegin(const char* /* name */) {
    beginContainer(TType::T_STRUCT, "{");
    return 0;
  }

  uint32_t writeStructEnd() {
    endContainer("struct", "}");
    return 0;
  }

  uint32_t writeFieldBegin(
      const char* /* name */, TType type, int16_t fieldId) {
    auto& frame = currentFrame();
    assert(frame.type == TType::T_STRUCT);
    assert(!frame.fieldId.has_value());
    frame.fieldId = fieldId;
    frame.fieldType = type;
    return 0;
  }

  uint32_t writeFieldEnd() {
    auto& frame = currentFrame();
    assert(frame.type == TType::T_STRUCT);
    assert(frame.fieldId.has_value());
    assert(frame.fieldWritten);
    frame.fieldId.reset();
    frame.fieldWritten = false;
    return 0;
  }

  uint32_t writeFieldStop() { return 0; }

  uint32_t writeMapBegin(
      TType keyType,
      TType valueType,
      uint32_t /* size */,
      bool /* alternativeKeyForm */ = false) {
    beginContainer(TType::T_MAP, "{");
    auto& frame = currentFrame();
    frame.keyType = keyType;
    frame.valueType = valueType;
    return 0;
  }

  uint32_t writeMapEnd() {
    auto frame = finishContainer("}");
    assert(!frame.mapKey.has_value());
    completeValue(
        fmt::format(
            "map<{}, {}>",
            lookupTypeStringIfEmpty(
                std::move(frame.observedKeyType), frame.keyType),
            lookupTypeStringIfEmpty(
                std::move(frame.observedValueType), frame.valueType)),
        std::move(frame.value));
    return 0;
  }

  uint32_t writeListBegin(TType elemType, uint32_t /* size */) {
    beginContainer(TType::T_LIST, "[");
    currentFrame().valueType = elemType;
    return 0;
  }

  uint32_t writeListEnd() {
    endSequence("list", "]");
    return 0;
  }

  uint32_t writeSetBegin(TType elemType, uint32_t /* size */) {
    beginContainer(TType::T_SET, "{");
    currentFrame().valueType = elemType;
    return 0;
  }

  uint32_t writeSetEnd() {
    endSequence("set", "}");
    return 0;
  }

  uint32_t writeBool(bool value) {
    completeValue("bool", value ? "true" : "false");
    return 0;
  }

  uint32_t writeByte(int8_t value) {
    completeValue("byte", fmt::format("{}", value));
    return 0;
  }

  uint32_t writeI16(int16_t value) {
    completeValue("i16", fmt::format("{}", value));
    return 0;
  }

  uint32_t writeI32(int32_t value) {
    completeValue("i32", fmt::format("{}", value));
    return 0;
  }

  uint32_t writeI64(int64_t value) {
    completeValue("i64", fmt::format("{}", value));
    return 0;
  }

  uint32_t writeDouble(double value) {
    completeValue("double", fmt::format("{}", value));
    return 0;
  }

  uint32_t writeFloat(float value) {
    completeValue("float", fmt::format("{}", value));
    return 0;
  }

  uint32_t writeString(folly::StringPiece value) { return writeBinary(value); }

  uint32_t writeBinary(folly::StringPiece value) {
    writeByteRange(folly::ByteRange(value));
    return 0;
  }

  // NOLINTNEXTLINE(clang-diagnostic-unneeded-member-function)
  uint32_t writeBinary(const folly::IOBuf& value) {
    const auto clone = value.clone();
    assert(clone);
    writeByteRange(clone->coalesce());
    return 0;
  }

 private:
  struct Frame {
    explicit Frame(TType frameType, std::string initialValue)
        : type(frameType), value(std::move(initialValue)) {}

    TType type;
    std::string value;
    TType keyType{TType::T_STOP};
    TType valueType{TType::T_STOP};
    std::string observedKeyType;
    std::string observedValueType;
    std::optional<std::string> mapKey;
    std::optional<int16_t> fieldId;
    TType fieldType{TType::T_STOP};
    bool fieldWritten{false};
    size_t lineLength{80};
  };

  void beginContainer(TType type, std::string initialValue) {
    frames_.emplace_back(type, std::move(initialValue));
    incIndent();
  }

  Frame finishContainer(folly::StringPiece closing) {
    decIndent();
    assert(!frames_.empty());
    auto frame = std::move(frames_.back());
    frames_.pop_back();
    wrapLine(frame.value);
    frame.value.append(closing.data(), closing.size());
    return frame;
  }

  void endContainer(folly::StringPiece type, folly::StringPiece closing) {
    auto frame = finishContainer(closing);
    completeValue(type.str(), std::move(frame.value));
  }

  void endSequence(folly::StringPiece type, folly::StringPiece closing) {
    auto frame = finishContainer(closing);
    completeValue(
        fmt::format(
            "{}<{}>",
            type,
            lookupTypeStringIfEmpty(
                std::move(frame.observedValueType), frame.valueType)),
        std::move(frame.value));
  }

  void completeValue(std::string type, std::string value) {
    if (frames_.empty()) {
      writeOutput(fmt::format("{} {}", type, value));
      return;
    }

    auto& frame = currentFrame();
    // NOLINTNEXTLINE(clang-diagnostic-switch-enum)
    switch (frame.type) {
      case TType::T_STRUCT:
        assert(frame.fieldId.has_value());
        assert(frame.fieldType != TType::T_STOP);
        wrapLine(frame.value);
        frame.value += fmt::format("{}: {} = {}", *frame.fieldId, type, value);
        frame.fieldWritten = true;
        return;
      case TType::T_LIST:
      case TType::T_SET:
        frame.observedValueType = std::move(type);
        appendSequenceValue(frame, std::move(value));
        return;
      case TType::T_MAP:
        if (!frame.mapKey) {
          frame.observedKeyType = std::move(type);
          frame.mapKey = std::move(value);
          return;
        }
        frame.observedValueType = std::move(type);
        wrapLine(frame.value);
        frame.value += fmt::format("{} : {},", *frame.mapKey, value);
        frame.mapKey.reset();
        return;
      default:
        assert(false);
        return;
    }
  }

  void appendSequenceValue(Frame& frame, std::string value) {
    constexpr size_t kMaxLineTarget = 80;
    if (frame.lineLength + value.size() >= kMaxLineTarget) {
      wrapLine(frame.value);
      frame.value += fmt::format("{},", value);
      frame.lineLength = indent_.size() + value.size();
      return;
    }
    frame.value += fmt::format(" {},", value);
    frame.lineLength += value.size() + 2;
  }

  void writeByteRange(folly::ByteRange value) {
    const std::string raw = value.empty()
        ? std::string{}
        : std::string(
              reinterpret_cast<const char*>(value.data()), value.size());
    completeValue("string", fmt::format("\"{}\"", folly::humanify(raw)));
  }

  void incIndent() { indent_.append(kIndentAmount, ' '); }

  void decIndent() {
    assert(indent_.size() >= kIndentAmount);
    indent_.resize(indent_.size() - kIndentAmount);
  }

  void wrapLine(std::string& value) const {
    value += fmt::format("\n{}", indent_);
  }

  void writeOutput(folly::StringPiece value) {
    output_.push(reinterpret_cast<const uint8_t*>(value.data()), value.size());
  }

  Frame& currentFrame() {
    assert(!frames_.empty());
    return frames_.back();
  }

  folly::io::QueueAppender output_{nullptr, 0};
  static constexpr size_t kIndentAmount = 2;
  std::string indent_;
  std::vector<Frame> frames_;
};

static_assert(ThriftProtocolWriter<DebugStringProtocolWriter>);

struct DebugStringSerializer {
  using ProtocolReader = DebugStringProtocolWriter::ProtocolReader;
  using ProtocolWriter = DebugStringProtocolWriter;
};

} // namespace

template <ThriftProtocolReader ProtocolReader>
std::string toDebugString(ProtocolReader& inProtoReader) {
  auto output = protocol::transcode<DebugStringSerializer>(inProtoReader);
  assert(output);
  return output->toString();
}

template std::string toDebugString<CompactProtocolReader>(
    CompactProtocolReader& inProtoReader);
template std::string toDebugString<BinaryProtocolReader>(
    BinaryProtocolReader& inProtoReader);

} // namespace apache::thrift
