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

#include <thrift/compiler/parse/t_type.h>

#include <string>
#include <sstream>

#include <openssl/sha.h>
#include <folly/Bits.h>

#include <thrift/compiler/parse/t_program.h>

constexpr size_t t_types::kTypeBits;
constexpr uint64_t t_types::kTypeMask;

uint64_t t_type::get_type_id() const {
  union {
    uint64_t val;
    unsigned char buf[SHA_DIGEST_LENGTH];
  } u;
  std::string name = get_full_name();
  SHA1(reinterpret_cast<const unsigned char*>(name.data()), name.size(), u.buf);
  uint64_t hash = folly::Endian::little(u.val);
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
