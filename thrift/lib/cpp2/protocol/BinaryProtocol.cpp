/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>

#include <folly/Conv.h>
#include <folly/portability/GFlags.h>
#include <functional>
#include <type_traits>

#if defined(__ARM_NEON) && __ARM_NEON
  #include <arm_neon.h>
#endif

FOLLY_GFLAGS_DEFINE_int32(
    thrift_cpp2_protocol_reader_string_limit,
    0,
    "Limit on string size when deserializing thrift, 0 is no limit");
FOLLY_GFLAGS_DEFINE_int32(
    thrift_cpp2_protocol_reader_container_limit,
    0,
    "Limit on container size when deserializing thrift, 0 is no limit");

namespace apache::thrift {

[[noreturn]] void BinaryProtocolReader::throwBadVersionIdentifier(int32_t sz) {
  throw TProtocolException(
      TProtocolException::BAD_VERSION,
      folly::to<std::string>("Bad version identifier, sz=", sz));
}

[[noreturn]] void BinaryProtocolReader::throwMissingVersionIdentifier(
    int32_t sz) {
  throw TProtocolException(
      TProtocolException::BAD_VERSION,
      folly::to<std::string>(
          "No version identifier... old protocol client in strict mode? sz=",
          sz));
}

#define USE_OPTIMIZED_ROUTINES 1
#if USE_OPTIMIZED_ROUTINES && defined(__ARM_NEON) && __ARM_NEON
#define COPY_REVERSE_USE_LDP 1
/**
 * Reverses the byte ordering for elements of any fundamental type in a 128b input vector.
 * E.g., a vector of LE-encoded int32_t will be transformed to BE-encoded int32_t
 * @tparam T type of the elements to reverse byte order for
 * @param a input vector
 * @return transformed output vector
 */
template<typename T>
inline uint8x16_t vrevq_generic_u8(uint8x16_t a) {
    if constexpr (sizeof(T) == 1) {
      return a; // identity -> byte representation identical on LE and BE
    } else if constexpr (sizeof(T) == 2) {
      return vrev16q_u8(a);
    } else if constexpr (sizeof(T) == 4) {
      return vrev32q_u8(a);
    } else if constexpr (sizeof(T) == 8) {
      return vrev64q_u8(a);
    } else {
      static_assert(!std::is_same_v<T, T>, "Unknown element size"); // C++23: static_assert(false); (P2593)
    }
}

/**
 * Copies a contiguous array of fundamental values to another, non overlapping, array while reversing the
 * byte ordering of the values
 * @tparam T Type of the elements to copy
 * @param dsts Destination pointer
 * @param srcs Source pointer
 * @param lens Number of elements to copy
 */
template <typename T>
inline std::enable_if_t<std::is_fundamental_v<T>, void> copyReverse(T* dsts, T* srcs, size_t lens) {
  auto* dst = (uint8_t*) dsts;
  auto* src = (uint8_t*) srcs;

  size_t len = lens * sizeof(T);
  // iterate over chunks of 64 bytes
  while(len >= 64) {
#if COPY_REVERSE_USE_LDP
    // Both GCC 13 and LLVM 17 transform this sequence to two ldp instruction and two stp at -O2
    // first block that loads 32B, reverses byte order, and stores results
    uint8x16_t  v0 = vld1q_u8(src);
    uint8x16_t  v1 = vld1q_u8(src + 16);
    v0 = vrevq_generic_u8<T>(v0);
    v1 = vrevq_generic_u8<T>(v1);
    vst1q_u8(dst, v0);
    vst1q_u8(dst + 16, v1);
    // second block that loads 32B, reverses byte order, and stores results
    uint8x16_t  v2 = vld1q_u8(src + 32);
    uint8x16_t  v3 = vld1q_u8(src + 48);
    v2 = vrevq_generic_u8<T>(v2);
    v3 = vrevq_generic_u8<T>(v3);
    vst1q_u8(dst + 32, v2);
    vst1q_u8(dst + 48, v3);
#else
    // Perform a single 4-reg load & store
    uint8x16x4_t v0 = vld1q_u8_x4(src);
    v0.val[0] = vrevq_generic_u8<T>(v0.val[0]);
    v0.val[1] = vrevq_generic_u8<T>(v0.val[1]);
    v0.val[2] = vrevq_generic_u8<T>(v0.val[2]);
    v0.val[3] = vrevq_generic_u8<T>(v0.val[3]);
    vst1q_u8_x4(dst, v0);
#endif

    // adjust pointers
    src += 64;
    dst += 64;
    len -= 64;
  }
  // copy single chunk of 32 bytes
  if(len >= 32) {
    uint8x16_t  v0 = vld1q_u8(src);
    uint8x16_t  v1 = vld1q_u8(src + 16);
    v0 = vrevq_generic_u8<T>(v0);
    v1 = vrevq_generic_u8<T>(v1);
    vst1q_u8(dst, v0);
    vst1q_u8(dst + 16, v1);
    // adjust pointers
    src += 32;
    dst += 32;
    len -= 32;
  }
  // copy single chunk of 16 bytes
  if(len >= 16) {
    uint8x16_t  v0 = vld1q_u8(src);
    v0 = vrevq_generic_u8<T>(v0);
    vst1q_u8(dst, v0);
    // adjust pointers
    src += 16;
    dst += 16;
    len -= 16;
  }

  // copy remainder sequentially
  dsts = (T*) dst;
  srcs = (T*) src;
  for(int i = 0; i < len/sizeof(T); ++i) {
    dsts[i] = folly::Endian::big(srcs[i]);
  }
}
#else
template <typename T>
inline void copyReverse(T* dst, T* src, size_t len) {
    // fallback implementation, this compiles to vectorized copy with scalar registers on aarch64
#pragma unroll
    for(int i = 0; i < len; ++i) {
      dst[i] = folly::Endian::big(src[i]);
    }
}
#endif



template <typename T>
void BinaryProtocolReader::readFundamentalVector(std::vector<T>& output) {
    size_t len_elements = output.size();
    size_t len = len_elements * sizeof(T);
    if (FOLLY_LIKELY(in_.length() >= len)) {
        copyReverse(&output[0], (T*) in_.data(), len_elements);
        in_.skip(len);
        return;
    }
    // vector does not fit into a contiguous buffer, read element one-by-one
    for(auto& el : output) {
      el = in_.readBE<T>();
    }
}

template
void BinaryProtocolReader::readFundamentalVector(std::vector<int64_t>& output);
template
void BinaryProtocolReader::readFundamentalVector(std::vector<int32_t>& output);
template
void BinaryProtocolReader::readFundamentalVector(std::vector<int16_t>& output);
template
void BinaryProtocolReader::readFundamentalVector(std::vector<int8_t>& output);
template
void BinaryProtocolReader::readFundamentalVector(std::vector<float>& output);
template
void BinaryProtocolReader::readFundamentalVector(std::vector<double>& output);

template <typename T>
inline size_t BinaryProtocolWriter::writeFundamentalVector(const std::vector<T>& input) {
  size_t len_elements = input.size();
  size_t len = len_elements * sizeof(T);
  out_.ensure(len);
  if (FOLLY_LIKELY(out_.length() >= len)) {
    copyReverse( (T*) out_.writableData(), (T*)  &input[0], len_elements);
    out_.append(len);
    return len;
  }
  // vector does not fit into output buffer, write out one-by-one
  for(const auto& el : input) {
    out_.writeBE(el);
  }
  return len;
}

template
size_t BinaryProtocolWriter::writeFundamentalVector(const std::vector<int64_t>& output);
template
size_t BinaryProtocolWriter::writeFundamentalVector(const std::vector<int32_t>& output);
template
size_t BinaryProtocolWriter::writeFundamentalVector(const std::vector<int16_t>& output);
template
size_t BinaryProtocolWriter::writeFundamentalVector(const std::vector<int8_t>& output);
template
size_t BinaryProtocolWriter::writeFundamentalVector(const std::vector<float>& output);
template
size_t BinaryProtocolWriter::writeFundamentalVector(const std::vector<double>& output);

void BinaryProtocolReader::skip(TType type, int depth) {
  if (depth >= FLAGS_thrift_protocol_max_depth) {
    protocol::TProtocolException::throwExceededDepthLimit();
  }
  size_t bytesToSkip = 0;
  switch (type) {
    case TType::T_BYTE:
    case TType::T_BOOL:
      bytesToSkip = sizeof(uint8_t);
      break;
    case TType::T_I16:
      bytesToSkip = sizeof(int16_t);
      break;
    case TType::T_FLOAT:
    case TType::T_I32:
      bytesToSkip = sizeof(int32_t);
      break;
    case TType::T_DOUBLE:
    case TType::T_U64:
    case TType::T_I64:
      bytesToSkip = sizeof(int64_t);
      break;
    case TType::T_UTF8:
    case TType::T_UTF16:
    case TType::T_STRING: {
      int32_t size = 0;
      auto in = getCursor();
      readI32(size);
      if (FOLLY_UNLIKELY(!in.canAdvance(static_cast<int32_t>(size)))) {
        protocol::TProtocolException::throwTruncatedData();
      }
      bytesToSkip = size;
      break;
    }
    case TType::T_STRUCT: {
      std::string name;
      TType ftype;
      readStructBegin(name);
      while (true) {
        int8_t rawType;
        readByte(rawType);
        ftype = static_cast<TType>(rawType);
        if (ftype == TType::T_STOP) {
          readStructEnd();
          return;
        }
        skipBytes(sizeof(int16_t));
        skip(ftype, depth + 1);
        readFieldEnd();
      }
    }
    case TType::T_MAP: {
      TType keyType;
      TType valType;
      uint32_t size;
      readMapBegin(keyType, valType, size);
      skip_n(*this, size, {keyType, valType}, depth + 1);
      readMapEnd();
      return;
    }
    case TType::T_SET: {
      TType elemType;
      uint32_t size;
      readSetBegin(elemType, size);
      skip_n(*this, size, {elemType}, depth + 1);
      readSetEnd();
      return;
    }
    case TType::T_LIST: {
      TType elemType;
      uint32_t size;
      readListBegin(elemType, size);
      skip_n(*this, size, {elemType}, depth + 1);
      readListEnd();
      return;
    }
    case TType::T_STOP:
    case TType::T_VOID:
    case TType::T_STREAM:
      // Unimplemented, fallback to default
    default: {
      TProtocolException::throwInvalidSkipType(type);
    }
  }
  skipBytes(bytesToSkip);
}

} // namespace apache::thrift
