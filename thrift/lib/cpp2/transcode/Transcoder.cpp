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

#include <thrift/lib/cpp2/transcode/Transcoder.h>

#include <thrift/lib/cpp2/transcode/Codec.h>
#include <thrift/lib/cpp2/transcode/TranscodeInterpreter.h>

#include <atomic>
#include <optional>
#include <string>
#include <utility>
#include <variant>

namespace apache::thrift::transcode {

namespace {

class InterpretedTranscoder final : public ITranscoder {
 public:
  explicit InterpretedTranscoder(TranscodePlan plan)
      : interp_(std::move(plan)) {}

  folly::Expected<std::unique_ptr<folly::IOBuf>, TranscodeError> transcode(
      const folly::IOBuf& input,
      ScalarFieldOverrides topLevelScalarOverrides) const override {
    return interp_.transcode(input, topLevelScalarOverrides);
  }

  folly::Expected<size_t, TranscodeError> transcodeInto(
      const folly::IOBuf& input,
      uint8_t* out,
      size_t cap,
      ScalarFieldOverrides topLevelScalarOverrides) const override {
    return interp_.transcodeInto(input, out, cap, topLevelScalarOverrides);
  }

  Engine engine() const override { return Engine::Interpreter; }

 private:
  TranscodeInterpreter interp_;
};

std::atomic<JitTranscoderFactory>& jitFactory() {
  static std::atomic<JitTranscoderFactory> factory{nullptr};
  return factory;
}

bool hasJsonMapSourceAndTarget(const Command& cmd) {
  if (const auto* mp = std::get_if<MapOp>(&cmd)) {
    if (mp->readFraming == ContainerFraming::Json &&
        mp->writeFraming == ContainerFraming::Json) {
      return true;
    }
    return (mp->key != nullptr && hasJsonMapSourceAndTarget(*mp->key)) ||
        (mp->value != nullptr && hasJsonMapSourceAndTarget(*mp->value));
  }
  if (const auto* st = std::get_if<StructOp>(&cmd)) {
    for (const auto& field : st->fields) {
      if (field.command != nullptr &&
          hasJsonMapSourceAndTarget(*field.command)) {
        return true;
      }
    }
    return false;
  }
  if (const auto* sq = std::get_if<SeqOp>(&cmd)) {
    return sq->element != nullptr && hasJsonMapSourceAndTarget(*sq->element);
  }
  return false;
}

std::optional<std::string> unsupportedTaggedUnionReason(
    const StructOp& op, const TaggedUnion& taggedUnion, bool read) {
  const bool json = read ? op.fieldIdent == FieldIdent::ByName
                         : op.writeFieldIdent == FieldIdent::ByName;
  if (!json) {
    return read
        ? "interpreter only supports read-tagged unions for JSON input"
        : "interpreter only supports write-tagged unions for JSON output";
  }
  const auto protocol = read ? op.writeFieldProto : op.readFieldProto;
  if (protocol == FieldProto::Unsupported) {
    return "interpreter tagged unions require a supported non-JSON protocol";
  }
  for (const auto& field : op.fields) {
    if (field.command == nullptr) {
      return "interpreter tagged unions require arm commands";
    }
    if (!taggedUnion.content.has_value()) {
      if (!std::holds_alternative<StructOp>(*field.command)) {
        return "interpreter internally tagged unions require struct arms";
      }
      const auto& member = std::get<StructOp>(*field.command);
      const auto memberProtocol =
          read ? member.writeFieldProto : member.readFieldProto;
      if (memberProtocol == FieldProto::Unsupported) {
        return "interpreter internally tagged unions require supported "
               "struct arm protocols";
      }
    }
  }
  return std::nullopt;
}

std::optional<std::string> unsupportedTaggedUnionReason(const Command& cmd) {
  if (const auto* st = std::get_if<StructOp>(&cmd)) {
    if (st->readTaggedUnion.has_value()) {
      if (auto reason =
              unsupportedTaggedUnionReason(*st, *st->readTaggedUnion, true)) {
        return reason;
      }
    }
    if (st->writeTaggedUnion.has_value()) {
      if (auto reason =
              unsupportedTaggedUnionReason(*st, *st->writeTaggedUnion, false)) {
        return reason;
      }
    }
    for (const auto& field : st->fields) {
      if (field.command != nullptr) {
        if (auto reason = unsupportedTaggedUnionReason(*field.command)) {
          return reason;
        }
      }
    }
    return std::nullopt;
  }
  if (const auto* sq = std::get_if<SeqOp>(&cmd)) {
    if (sq->element != nullptr) {
      return unsupportedTaggedUnionReason(*sq->element);
    }
    return std::nullopt;
  }
  if (const auto* mp = std::get_if<MapOp>(&cmd)) {
    if (mp->key != nullptr) {
      if (auto reason = unsupportedTaggedUnionReason(*mp->key)) {
        return reason;
      }
    }
    if (mp->value != nullptr) {
      return unsupportedTaggedUnionReason(*mp->value);
    }
  }
  return std::nullopt;
}

std::optional<std::string> missingProtocolReason(const TranscodePlan& plan) {
  if (plan.sourceProtocol == WireProtocol::Unknown ||
      plan.targetProtocol == WireProtocol::Unknown) {
    return "transcode plan protocol metadata is missing";
  }
  return std::nullopt;
}

std::optional<std::string> unsupportedProtocolReason(
    const TranscodePlan& plan) {
  const bool protobuf = plan.sourceProtocol == WireProtocol::ProtobufBinary ||
      plan.targetProtocol == WireProtocol::ProtobufBinary;
  if (protobuf) {
    return "Protobuf protocol support is still in development; "
           "pass UnsupportedPlanPolicy::AllowExperimentalProtocols to opt in";
  }
  return std::nullopt;
}

// Returns a reason string when the interpreter cannot run this plan, otherwise
// nullopt.
std::optional<std::string> interpreterSupports(const TranscodePlan& plan) {
  if (plan.structTarget) {
    return "interpreter does not support struct-memory targets; use Engine::Jit";
  }
  if (plan.sourceProtocol == WireProtocol::Json &&
      plan.targetProtocol == WireProtocol::Json) {
    return "interpreter does not yet support JSON-to-JSON plans; use Engine::Jit";
  }
  if (hasJsonMapSourceAndTarget(plan.root)) {
    return "interpreter does not support JSON-to-JSON map transcodes";
  }
  if (auto reason = unsupportedTaggedUnionReason(plan.root)) {
    return reason;
  }
  return std::nullopt;
}

} // namespace

folly::Expected<std::unique_ptr<ITranscoder>, CompileError> makeTranscoder(
    TranscodePlan plan, Engine engine, TranscoderOptions options) {
  if (engine == Engine::Jit) {
    return folly::makeUnexpected(CompileError{"JIT engine not linked"});
  }
  if (engine != Engine::Interpreter) {
    return folly::makeUnexpected(CompileError{"unknown engine"});
  }
  if (auto reason = missingProtocolReason(plan)) {
    return folly::makeUnexpected(CompileError{std::move(*reason)});
  }
  if (options.unsupportedPlanPolicy == UnsupportedPlanPolicy::Reject) {
    if (auto reason = unsupportedProtocolReason(plan)) {
      return folly::makeUnexpected(CompileError{std::move(*reason)});
    }
  }
  if (auto reason = interpreterSupports(plan)) {
    return folly::makeUnexpected(CompileError{std::move(*reason)});
  }
  return std::make_unique<InterpretedTranscoder>(std::move(plan));
}

void registerJitTranscoderFactory(JitTranscoderFactory factory) {
  jitFactory().store(factory, std::memory_order_release);
}

} // namespace apache::thrift::transcode
