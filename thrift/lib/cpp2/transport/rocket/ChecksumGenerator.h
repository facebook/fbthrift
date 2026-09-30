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

#pragma once

#include <xxhash.h>
#include <folly/Random.h>
#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp2/IOBufChain.h>

namespace apache::thrift::rocket {

struct XXH3_64;
struct CRC32C;

struct CheckSumResponse {
  int64_t checksum;
  int64_t salt;
};

namespace detail {

struct XXH3Logic {
  int64_t operator()(folly::IOBuf& buffer, int64_t salt) {
    return xxh3(buffer, salt);
  }
  int64_t operator()(IOBufChain& buffer, int64_t salt) {
    return xxh3(buffer, salt);
  }

 private:
  XXH3_state_t* getXXH3State();
  int64_t xxh3(folly::IOBuf& buffer, int64_t salt);
  int64_t xxh3(IOBufChain& buffer, int64_t salt);
};

struct CRC32Logic {
  int64_t operator()(folly::IOBuf& buffer, int64_t salt) {
    return crc32c(buffer, salt);
  }
  int64_t operator()(IOBufChain& buffer, int64_t salt) {
    return crc32c(buffer, salt);
  }

 private:
  int64_t crc32c(folly::IOBuf& buffer, int64_t salt);
  int64_t crc32c(IOBufChain& buffer, int64_t salt);
};

template <typename Algorithm>
using ChecksumLogic = std::conditional_t<
    std::is_same_v<Algorithm, XXH3_64>,
    XXH3Logic,
    std::conditional_t<std::is_same_v<Algorithm, CRC32C>, CRC32Logic, void>>;

template <typename Algorithm>
struct ChecksumCalculator {
  using Logic = ChecksumLogic<Algorithm>;
  template <typename Buffer>
  CheckSumResponse operator()(Buffer& buffer, int64_t salt) {
    auto checksum = Logic()(buffer, salt);
    return CheckSumResponse{checksum, salt};
  }
};

} // namespace detail

template <typename Algorithm>
struct ChecksumGenerator {
  template <typename Buffer>
  CheckSumResponse calculateChecksum(
      Buffer& buffer, int64_t salt = folly::Random::rand64()) {
    detail::ChecksumCalculator<Algorithm> calculator;
    return calculator(buffer, salt);
  }

  template <typename Buffer>
  bool validateChecksum(int64_t checksum, int64_t salt, Buffer& buffer) {
    return calculateChecksum(buffer, salt).checksum == checksum;
  }

  CheckSumResponse calculateChecksumFromIOBuf(
      folly::IOBuf& buffer, int64_t salt = folly::Random::rand64()) {
    return calculateChecksum(buffer, salt);
  }

  bool validateChecksumFromIOBuf(
      int64_t checksum, int64_t salt, folly::IOBuf& buffer) {
    return validateChecksum(checksum, salt, buffer);
  }
};

} // namespace apache::thrift::rocket
