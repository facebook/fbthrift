/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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
#include <string>

#include <openssl/sha.h>

#include <thrift/compiler/ast/endianness.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_typedef.h>

namespace apache {
namespace thrift {
namespace compiler {

constexpr size_t t_type::kTypeBits;
constexpr uint64_t t_type::kTypeMask;

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
            "sink",
            "stream",
            "binary"}});
  return kTypeNames.at(static_cast<size_t>(t));
}

uint64_t t_type::get_type_id() const {
  // This union allows the conversion of the SHA char buffer to a 64bit uint
  union {
    uint64_t val;
    unsigned char buf[SHA_DIGEST_LENGTH];
  } u;

  std::string name = get_full_name();
  SHA1(reinterpret_cast<const unsigned char*>(name.data()), name.size(), u.buf);
  const auto hash =
      apache::thrift::compiler::bswap_host_to_little_endian(u.val);
  type tv = get_type_value();

  return (hash & ~t_type::kTypeMask) | int(tv);
}

std::string t_type::make_full_name(const char* prefix) const {
  std::ostringstream os;
  os << prefix << " ";
  if (program_) {
    os << program_->get_name() << ".";
  }
  os << name_;
  return os.str();
}

const t_type* t_type::get_true_type() const {
  return t_typedef::find_type_if(
      this, [](const t_type* type) { return !type->is_typedef(); });
}

} // namespace compiler
} // namespace thrift
} // namespace apache
