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
#include <thrift/compiler/ast/t_typedef.h>

#include <sstream>
#include <string>

#include <openssl/sha.h>

#include <thrift/compiler/ast/endianness.h>
#include <thrift/compiler/ast/t_program.h>

namespace apache {
namespace thrift {
namespace compiler {

constexpr size_t t_types::kTypeBits;
constexpr uint64_t t_types::kTypeMask;

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
  TypeValue tv = get_type_value();

  return (hash & ~t_types::kTypeMask) | int(tv);
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
  const t_type* type = this;
  while (type->is_typedef()) {
    type = static_cast<const t_typedef*>(type)->get_type();
  }
  return type;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
