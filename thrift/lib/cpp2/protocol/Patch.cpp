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

#include <thrift/lib/cpp2/protocol/Patch.h>

#include <algorithm>

#include <fmt/core.h>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/thrift/gen-cpp2/patch_types.h>
#include <thrift/lib/thrift/gen-cpp2/protocol_types.h>

namespace apache {
namespace thrift {
namespace protocol {
namespace detail {
namespace {
using op::PatchOp;

bool is_valid_op(const short op) {
  return util::enumName<PatchOp>(static_cast<PatchOp>(op)) != nullptr;
}

const Value* if_op(const Object& patch, PatchOp op) {
  return patch.if_contains(static_cast<FieldId>(op));
}

const Value* if_valid_op(
    const Object& patch, PatchOp op, Value::Type expected) {
  auto* assign = if_op(patch, op);
  if (assign && assign->getType() != expected) {
    folly::throw_exception<std::runtime_error>(fmt::format(
        "Incorrect type in the patch. Expected {} got {}",
        util::enumNameSafe<Value::Type>(expected),
        util::enumNameSafe<Value::Type>(assign->getType())));
  }
  return assign;
}

} // namespace

void ApplyPatch::operator()(const Object& patch, protocol::Value& value) const {
  if (!std::all_of(patch.begin(), patch.end(), [](const auto& fv) {
        return is_valid_op(fv.first);
      })) {
    folly::throw_exception<std::runtime_error>(
        "Unknown operation id found in patch object");
  }

  switch (value.getType()) {
    case Value::Type::boolValue:
      operator()(patch, *value.boolValue_ref());
      return;
    default:
      folly::throw_exception<std::runtime_error>("Not Implemented.");
  }
}

void ApplyPatch::operator()(const Object& patch, bool& value) const {
  if (auto* assign =
          if_valid_op(patch, PatchOp::Assign, Value::Type::boolValue)) {
    value = *assign->boolValue_ref();
    return; // Ignore all other ops.
  }

  if (auto* invert = if_valid_op(patch, PatchOp::Add, Value::Type::boolValue);
      invert && *invert->boolValue_ref()) {
    value = !value;
  }
}

} // namespace detail
} // namespace protocol
} // namespace thrift
} // namespace apache
