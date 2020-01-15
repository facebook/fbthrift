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

#pragma once

#include <stdint.h>

#include <fmt/core.h>
#include <glog/logging.h>

namespace apache {
namespace thrift {
namespace rocket {

class Flags {
 public:
  constexpr Flags() = default;

  constexpr explicit Flags(uint16_t flags) : flags_(static_cast<Bits>(flags)) {
    DCHECK((flags & ~mask()) == 0);
    if (flags != 0 && flags < Bits::NEXT) {
      throw std::runtime_error(fmt::format("received invalid flags {}", flags));
    }
  }

  // Flags and frame type are packed into 2 bytes on the wire. Flags occupy the
  // lower 10 bits, frame type occupies the upper 6 bits.
  static constexpr uint8_t frameTypeOffset() {
    return 10;
  }

  static constexpr uint16_t mask() {
    return (static_cast<uint16_t>(1) << frameTypeOffset()) - 1;
  }

  static constexpr Flags none() {
    return Flags{};
  }

  constexpr explicit operator uint16_t() const {
    return static_cast<uint16_t>(flags_);
  }

#define THRIFT_ROCKET_CREATE_GETTER_SETTER( \
    member_function_name, bits_enum_value)  \
  bool member_function_name() const {       \
    return flags_ & bits_enum_value;        \
  }                                         \
                                            \
  Flags& member_function_name(bool on) {    \
    return set(bits_enum_value, on);        \
  }

  THRIFT_ROCKET_CREATE_GETTER_SETTER(next, Bits::NEXT)
  THRIFT_ROCKET_CREATE_GETTER_SETTER(complete, Bits::COMPLETE)
  THRIFT_ROCKET_CREATE_GETTER_SETTER(lease, Bits::LEASE)
  THRIFT_ROCKET_CREATE_GETTER_SETTER(follows, Bits::FOLLOWS)
  THRIFT_ROCKET_CREATE_GETTER_SETTER(resumeToken, Bits::RESUME_TOKEN)
  THRIFT_ROCKET_CREATE_GETTER_SETTER(metadata, Bits::METADATA)
  THRIFT_ROCKET_CREATE_GETTER_SETTER(ignore, Bits::IGNORE_)
  THRIFT_ROCKET_CREATE_GETTER_SETTER(respond, Bits::RESPOND)

#undef THRIFT_ROCKET_CREATE_GETTER_SETTER

  bool operator==(Flags other) const {
    return flags_ == other.flags_;
  }

 private:
  enum Bits : uint16_t {
    NONE = 0,
    NEXT = 1 << 5,
    COMPLETE = 1 << 6,
    LEASE = 1 << 6,
    FOLLOWS = 1 << 7,
    RESUME_TOKEN = 1 << 7,
    RESPOND = 1 << 7,
    METADATA = 1 << 8,
    IGNORE_ = 1 << 9,
  };
  Bits flags_{Bits::NONE};

  Flags& set(Bits bits, bool on) {
    flags_ = on ? static_cast<Bits>(flags_ | bits)
                : static_cast<Bits>(flags_ & ~bits);
    return *this;
  }
};

} // namespace rocket
} // namespace thrift
} // namespace apache
