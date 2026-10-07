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

#include <thrift/lib/cpp/protocol/TProtocolTypes.h>
#include <thrift/lib/cpp2/async/RpcTypes.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>

#include <gtest/gtest.h>

#include <thrift/lib/cpp2/transport/rocket/compression/CompressionManager.h>

using namespace apache::thrift;

class RpcTypesTest : public ::testing::TestWithParam<
                         std::tuple<bool, protocol::PROTOCOL_TYPES>> {};

folly::StringPiece methodName("methodName");

IOBufChain chainPayload() {
  IOBufChain result;
  result.append(folly::IOBuf::copyBuffer("pay"));
  result.append(folly::IOBuf::copyBuffer("load"));
  return result;
}

void expectSameBuffer(
    const folly::IOBuf& expected, const folly::IOBuf& actual) {
  EXPECT_EQ(folly::ordering::eq, folly::IOBufCompare()(expected, actual));
}

TEST_P(RpcTypesTest, Responses) {
  auto lsr1 = []() {
    return LegacySerializedResponse(
        std::get<1>(GetParam()),
        methodName,
        SerializedResponse{folly::IOBuf::copyBuffer(std::string{"payload"})});
  };
  auto lsr2 = [](uint32_t seqId) {
    return LegacySerializedResponse(
        std::get<1>(GetParam()),
        seqId,
        methodName,
        SerializedResponse{folly::IOBuf::copyBuffer(std::string{"payload"})});
  };
  auto lsr3 = [](uint32_t seqId) {
    return LegacySerializedResponse(
        std::get<1>(GetParam()),
        seqId,
        MessageType::T_REPLY,
        methodName,
        SerializedResponse{folly::IOBuf::copyBuffer(std::string{"payload"})});
  };
  auto lsrep = [](LegacySerializedResponse&& lsr) {
    return std::move(lsr)
        .extractPayload(std::get<0>(GetParam()), std::get<1>(GetParam()))
        .second;
  };
  auto lsrep1 = [](LegacySerializedResponse&& lsr, uint32_t seqId) {
    return std::move(lsr)
        .extractPayload(std::get<0>(GetParam()), std::get<1>(GetParam()), seqId)
        .second;
  };

  auto sr1 = []() {
    return SerializedResponse(folly::IOBuf::copyBuffer(std::string{"payload"}));
  };

  auto srep = [](SerializedResponse&& sr, uint32_t seqId) {
    return std::move(sr).extractPayload(
        std::get<0>(GetParam()),
        std::get<1>(GetParam()),
        seqId,
        MessageType::T_REPLY,
        methodName);
  };

  auto check = [](auto&& lsrPayload, auto&& srPayload) {
    EXPECT_EQ(
        folly::ordering::eq,
        folly::IOBufCompare()(*lsrPayload.buffer(), *srPayload.buffer()));
  };

  check(lsrep(lsr1()), srep(sr1(), 0));
  check(lsrep(lsr2(10)), srep(sr1(), 10));
  check(lsrep(lsr3(10)), srep(sr1(), 10));
  // Providing a seqId of 0 to extractPayload means ignore it and use the
  // existing seqId
  check(lsrep1(lsr2(10), 0), srep(sr1(), 10));
  check(lsrep1(lsr3(10), 0), srep(sr1(), 10));
  check(lsrep1(lsr2(10), 5), srep(sr1(), 5));
  check(lsrep1(lsr3(10), 5), srep(sr1(), 5));
}

TEST(RpcTypesTestStandalone, UncompressPreservesChain) {
  auto request =
      std::move(SerializedCompressedRequest{chainPayload()}).uncompress();

  ASSERT_TRUE(request.chainBuffer);
  EXPECT_EQ(nullptr, request.buffer);
  EXPECT_EQ(chainPayload(), *request.chainBuffer);
}

TEST(RpcTypesTestStandalone, UncompressPreservesRequestChain) {
  auto request =
      std::move(SerializedCompressedRequest{SerializedRequest{chainPayload()}})
          .uncompress();

  ASSERT_TRUE(request.chainBuffer);
  EXPECT_EQ(nullptr, request.buffer);
  EXPECT_EQ(chainPayload(), *request.chainBuffer);
}

TEST(RpcTypesTestStandalone, ClonePreservesChecksum) {
  const SerializedCompressedRequest request{
      folly::IOBuf::copyBuffer("payload"),
      CompressionAlgorithm::NONE,
      ChecksumAlgorithm::CRC32};

  EXPECT_EQ(ChecksumAlgorithm::CRC32, request.clone().getChecksumAlgorithm());
}

TEST(RpcTypesTestStandalone, UncompressesChain) {
  auto compressed = rocket::CompressionManager().compressBuffer(
      folly::IOBuf::copyBuffer("payload"), CompressionAlgorithm::ZSTD);
  auto request =
      std::move(
          SerializedCompressedRequest{
              IOBufChain(std::move(compressed)), CompressionAlgorithm::ZSTD})
          .uncompress();

  ASSERT_TRUE(request.chainBuffer);
  EXPECT_EQ(nullptr, request.buffer);
  EXPECT_EQ(
      IOBufChain(folly::IOBuf::copyBuffer("payload")), *request.chainBuffer);
}

TEST(RpcTypesTestStandalone, UncompressPassesThroughChecksummedChain) {
  auto request = std::move(
                     SerializedCompressedRequest{
                         chainPayload(),
                         CompressionAlgorithm::NONE,
                         ChecksumAlgorithm::CRC32})
                     .uncompress();

  ASSERT_TRUE(request.chainBuffer);
  EXPECT_EQ(nullptr, request.buffer);
  EXPECT_EQ(chainPayload(), *request.chainBuffer);
}

TEST_P(RpcTypesTest, Exceptions) {
  auto lsr1 = []() {
    return LegacySerializedResponse(
        std::get<1>(GetParam()),
        methodName,
        TApplicationException(TApplicationException::MISSING_RESULT));
  };
  auto lsr2 = []() {
    return LegacySerializedResponse(
        std::get<1>(GetParam()),
        0,
        methodName,
        TApplicationException(TApplicationException::MISSING_RESULT));
  };
  auto lsrep = [](LegacySerializedResponse&& lsr) {
    return std::move(lsr)
        .extractPayload(std::get<0>(GetParam()), std::get<1>(GetParam()), 0)
        .second;
  };

  auto sr1 = []() {
    return SerializedResponse(serializeErrorStruct(
        std::get<1>(GetParam()),
        TApplicationException(TApplicationException::MISSING_RESULT)));
  };
  auto srep = [](SerializedResponse&& sr) {
    return std::move(sr).extractPayload(
        std::get<0>(GetParam()),
        std::get<1>(GetParam()),
        0,
        MessageType::T_EXCEPTION,
        methodName);
  };

  auto check = [](auto&& lsrPayload, auto&& srPayload) {
    EXPECT_EQ(
        folly::ordering::eq,
        folly::IOBufCompare()(*lsrPayload.buffer(), *srPayload.buffer()));
  };

  check(lsrep(lsr1()), srep(sr1()));
  check(lsrep(lsr2()), srep(sr1()));
}

INSTANTIATE_TEST_CASE_P(
    RpcTypesTestAll,
    RpcTypesTest,
    ::testing::Combine(
        ::testing::Values(true, false),
        ::testing::Values(
            protocol::T_COMPACT_PROTOCOL, protocol::T_BINARY_PROTOCOL)));
