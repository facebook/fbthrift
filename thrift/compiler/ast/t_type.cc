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

#include <thrift/compiler/ast/t_type.h>

#include <array>
#include <sstream>
#include <stdexcept>
#include <string>

#include <openssl/sha.h>

#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_typedef.h>

namespace apache::thrift::compiler {

const std::string& t_type::type_name(type t) {
  static const auto& kTypeNames =
      *new std::array<std::string, t_type::kTypeCount>(
          {{"void",
            "string",
            "bool",
            "byte",
            "i16",
            "i32",
            "i64",
            "double",
            "enum",
            "list",
            "set",
            "map",
            "struct",
            "service",
            "program",
            "float",
            "", // unused [sink]
            "", // unused [stream]
            "binary"}});
  return kTypeNames.at(static_cast<size_t>(t));
}

uint64_t t_type::get_type_id() const {
  std::string name = get_full_name();
  unsigned char buf[SHA_DIGEST_LENGTH] = {};
  SHA1(reinterpret_cast<const unsigned char*>(name.data()), name.size(), buf);
  uint64_t hash = 0;
  std::memcpy(&hash, &buf, sizeof(hash));
#if !defined(_WIN32) && __BYTE_ORDER__ != __ORDER_LITTLE_ENDIAN__
  hash = __builtin_bswap64(hash);
#endif
  return (hash & ~t_type::kTypeMask) | static_cast<int>(get_type_value());
}

std::string t_type::get_scoped_name() const {
  std::ostringstream os;
  if (program_) {
    os << program_->name() << ".";
  }
  os << name_;
  return os.str();
}

const t_type* t_type::get_true_type() const {
  return t_typedef::find_type_if(
      this, [](const t_type* type) { return !type->is_typedef(); });
}

bool t_type_ref::resolved() const noexcept {
  return !empty() &&
      (unresolved_type_ == nullptr || unresolved_type_->type().resolved());
}

bool t_type_ref::resolve() {
  if (unresolved_type_ != nullptr) { // Try to resolve.
    if (!unresolved_type_->resolve()) {
      return false;
    }
    // Try to excise the placeholder typedef so dynamic_cast works.
    if (unresolved_type_->annotations().empty()) {
      type_ = unresolved_type_->get_type();
    }
    unresolved_type_ = nullptr;
  }
  return true;
}

const t_type& t_type_ref::deref() {
  if (!resolve()) {
    throw std::runtime_error(
        "Could not resolve type: " + unresolved_type_->get_full_name());
  }
  return deref_or_throw();
}

const t_type& t_type_ref::deref_or_throw() const {
  if (type_ == nullptr) {
    throw std::runtime_error("t_type_ref has no type.");
  }
  if (auto ph = dynamic_cast<const t_placeholder_typedef*>(type_)) {
    return ph->type().deref();
  }
  return *type_;
}

t_type_ref t_type_ref::for_placeholder(t_placeholder_typedef& unresolved_type) {
  return t_type_ref{
      unresolved_type, unresolved_type, unresolved_type.src_range()};
}

const t_type_ref& t_type_ref::none() {
  static const t_type_ref empty;
  return empty;
}

std::optional<t_type::value_type> t_type::as_value_type() const {
  switch (get_type_value()) {
    case type::t_bool:
      return value_type::BOOL;
    case type::t_byte:
      return value_type::BYTE;
    case type::t_i16:
      return value_type::I16;
    case type::t_i32:
      return value_type::I32;
    case type::t_i64:
      return value_type::I64;
    case type::t_float:
      return value_type::FLOAT;
    case type::t_double:
      return value_type::DOUBLE;
    case type::t_string:
      return value_type::STRING;
    case type::t_binary:
      return value_type::BINARY;
    case type::t_list:
      return value_type::LIST;
    case type::t_set:
      return value_type::SET;
    case type::t_map:
      return value_type::MAP;
    case type::t_enum:
      return value_type::I32;
    case type::t_structured:
    case type::t_service:
    case type::t_void:
      return std::nullopt;
  }
  abort();
}

} // namespace apache::thrift::compiler
