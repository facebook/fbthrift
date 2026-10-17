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

#include <gtest/gtest.h>

#include <string>
#include <type_traits>
#include <utility>

#include <folly/io/IOBufQueue.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/IOBufChain.h>
#include <thrift/lib/cpp2/async/processor/GeneratedAsyncProcessorBase.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/ChainService.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/ChainService.tcc>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/ChainServiceAsyncClient.h>

namespace {

using apache::thrift::BinaryProtocolChainReader;
using apache::thrift::BinaryProtocolReader;
using apache::thrift::BinaryProtocolWriter;
using apache::thrift::CompactProtocolReader;
using apache::thrift::CompactProtocolWriter;
using apache::thrift::FieldData;
using apache::thrift::IOBufChain;
using apache::thrift::test::iobuf_chain_rpc::ChainService;
using apache::thrift::test::iobuf_chain_rpc::ChainService_echo_pargs;
using apache::thrift::test::iobuf_chain_rpc::ChainService_echo_presult;
using apache::thrift::test::iobuf_chain_rpc::Payload;
using apache::thrift::type_class::structure;

using GeneratedArgsField = std::remove_reference_t<
    decltype(std::declval<ChainService_echo_pargs&>().get<0>())>;
static_assert(
    std::is_same_v<GeneratedArgsField, FieldData<1, structure, Payload*>>);
static_assert(apache::thrift::detail::
                  use_op_decode_for_fields_v<BinaryProtocolChainReader, void>);
static_assert(!apache::thrift::detail::
                  use_op_decode_for_fields_v<BinaryProtocolReader, void>);
static_assert(!apache::thrift::detail::
                  use_op_decode_for_fields_v<CompactProtocolReader, void>);
static_assert(std::is_same_v<
              decltype(std::declval<BinaryProtocolReader>().getCursor()),
              const folly::io::Cursor&>);
static_assert(std::is_same_v<
              decltype(std::declval<CompactProtocolReader>().getCursor()),
              const folly::io::Cursor&>);

std::string chainString(const IOBufChain& chain) {
  std::string result;
  result.reserve(chain.chainLength());
  for (const auto& buffer : chain) {
    result.append(
        reinterpret_cast<const char*>(buffer.data()), buffer.length());
  }
  return result;
}

TEST(IOBufChainRpcCodegenTest, DecodesGeneratedPargsWithBothBufferTypes) {
  Payload input;
  *input.sequence() = 123456789;
  *input.text() = "chain reader";
  *input.ordinaryBinary() = std::string{"ordinary binary"};
  *input.chainBinary() = IOBufChain{folly::IOBuf::copyBuffer("chain binary")};
  input.choice()->number() = 42;
  ChainService_echo_pargs inputArgs;
  inputArgs.get<0>().value = &input;

  folly::IOBufQueue queue;
  BinaryProtocolWriter writer;
  writer.setOutput(&queue);
  inputArgs.write(&writer);

  auto serialized = queue.move();
  auto iobufSerialized = serialized->clone();
  IOBufChain serializedChain{std::move(serialized)};
  BinaryProtocolChainReader reader;
  reader.setInput(&serializedChain);
  Payload output;
  ChainService_echo_pargs outputArgs;
  outputArgs.get<0>().value = &output;
  outputArgs.read(&reader);

  EXPECT_EQ(*input.sequence(), *output.sequence());
  EXPECT_EQ(*input.text(), *output.text());
  EXPECT_EQ(*input.ordinaryBinary(), *output.ordinaryBinary());
  EXPECT_EQ(*input.choice()->number(), *output.choice()->number());
  EXPECT_EQ(
      chainString(*input.chainBinary()), chainString(*output.chainBinary()));

  BinaryProtocolReader iobufReader;
  iobufReader.setInput(iobufSerialized.get());
  Payload iobufOutput;
  ChainService_echo_pargs iobufOutputArgs;
  iobufOutputArgs.get<0>().value = &iobufOutput;
  iobufOutputArgs.read(&iobufReader);

  EXPECT_EQ(*input.sequence(), *iobufOutput.sequence());
  EXPECT_EQ(*input.text(), *iobufOutput.text());
  EXPECT_EQ(*input.ordinaryBinary(), *iobufOutput.ordinaryBinary());
  EXPECT_EQ(*input.choice()->number(), *iobufOutput.choice()->number());
  EXPECT_EQ(
      chainString(*input.chainBinary()),
      chainString(*iobufOutput.chainBinary()));
}

TEST(IOBufChainRpcCodegenTest, SelectsReaderFromResponseBufferRepresentation) {
  Payload input;
  *input.sequence() = 123456789;
  *input.text() = "chain reader";
  *input.ordinaryBinary() = std::string{"ordinary binary"};
  *input.chainBinary() = IOBufChain{folly::IOBuf::copyBuffer("chain binary")};
  input.choice()->number() = 42;
  ChainService_echo_presult inputResult;
  inputResult.get<0>().value = &input;
  inputResult.setIsSet(0, true);

  folly::IOBufQueue queue;
  BinaryProtocolWriter writer;
  writer.setOutput(&queue);
  inputResult.write(&writer);
  auto serialized = queue.move();

  auto decode = [&](apache::thrift::SerializedResponse response,
                    bool hasResponseBuffer) {
    apache::thrift::ClientReceiveState state(
        apache::thrift::protocol::T_BINARY_PROTOCOL,
        apache::thrift::MessageType::T_REPLY,
        std::move(response),
        nullptr,
        nullptr,
        apache::thrift::RpcTransportStats{});
    EXPECT_EQ(state.hasResponseBuffer(), hasResponseBuffer);

    Payload output;
    auto error =
        apache::thrift::Client<ChainService>::recv_wrapped_echo(output, state);
    EXPECT_FALSE(error);
    EXPECT_EQ(*input.sequence(), *output.sequence());
    EXPECT_EQ(*input.text(), *output.text());
    EXPECT_EQ(*input.ordinaryBinary(), *output.ordinaryBinary());
    EXPECT_EQ(*input.choice()->number(), *output.choice()->number());
    EXPECT_EQ(
        chainString(*input.chainBinary()), chainString(*output.chainBinary()));
  };

  decode(apache::thrift::SerializedResponse{serialized->clone()}, true);
  decode(
      apache::thrift::SerializedResponse{IOBufChain{std::move(serialized)}},
      false);
}

TEST(IOBufChainRpcCodegenTest, RejectsChainResponseForCompactProtocol) {
  Payload input;
  *input.sequence() = 123456789;
  *input.text() = "compact response";
  *input.ordinaryBinary() = std::string{"ordinary binary"};
  *input.chainBinary() = IOBufChain{folly::IOBuf::copyBuffer("chain binary")};
  input.choice()->text() = "compact union";
  ChainService_echo_presult inputResult;
  inputResult.get<0>().value = &input;
  inputResult.setIsSet(0, true);

  folly::IOBufQueue queue;
  CompactProtocolWriter writer;
  writer.setOutput(&queue);
  inputResult.write(&writer);
  auto serialized = queue.move();

  apache::thrift::ClientReceiveState iobufState(
      apache::thrift::protocol::T_COMPACT_PROTOCOL,
      apache::thrift::MessageType::T_REPLY,
      apache::thrift::SerializedResponse{serialized->clone()},
      nullptr,
      nullptr,
      apache::thrift::RpcTransportStats{});
  Payload output;
  auto error = apache::thrift::Client<ChainService>::recv_wrapped_echo(
      output, iobufState);
  EXPECT_FALSE(error);
  EXPECT_EQ(*input.sequence(), *output.sequence());
  EXPECT_EQ(*input.text(), *output.text());
  EXPECT_EQ(*input.ordinaryBinary(), *output.ordinaryBinary());
  EXPECT_EQ(*input.choice()->text(), *output.choice()->text());
  EXPECT_EQ(
      chainString(*input.chainBinary()), chainString(*output.chainBinary()));

  apache::thrift::ClientReceiveState chainState(
      apache::thrift::protocol::T_COMPACT_PROTOCOL,
      apache::thrift::MessageType::T_REPLY,
      apache::thrift::SerializedResponse{IOBufChain{std::move(serialized)}},
      nullptr,
      nullptr,
      apache::thrift::RpcTransportStats{});
  EXPECT_FALSE(chainState.hasResponseBuffer());
  Payload chainOutput;
  error = apache::thrift::Client<ChainService>::recv_wrapped_echo(
      chainOutput, chainState);
  const auto* exception =
      error.get_exception<apache::thrift::TApplicationException>();
  ASSERT_NE(nullptr, exception);
  EXPECT_EQ(
      apache::thrift::TApplicationException::UNSUPPORTED_CLIENT_TYPE,
      exception->getType());
}

class RequestDeserializer : public apache::thrift::GeneratedAsyncProcessorBase {
 public:
  using GeneratedAsyncProcessorBase::deserializeRequest;
};

template <typename Reader, typename Writer>
void testRequestReaderSelection() {
  Payload input;
  *input.sequence() = 123456789;
  *input.text() = "request reader";
  *input.ordinaryBinary() = "ordinary binary";
  *input.chainBinary() = IOBufChain{folly::IOBuf::copyBuffer("chain binary")};
  input.choice()->text() = "request union";
  ChainService_echo_pargs inputArgs;
  inputArgs.get<0>().value = &input;

  folly::IOBufQueue queue;
  Writer writer;
  writer.setOutput(&queue);
  inputArgs.write(&writer);
  auto serialized = queue.move();

  auto decode = [&](apache::thrift::SerializedRequest request) {
    Payload output;
    ChainService_echo_pargs outputArgs;
    outputArgs.get<0>().value = &output;
    RequestDeserializer::deserializeRequest<
        Reader,
        ChainService_echo_pargs,
        apache::thrift::detail::IOBufChainRequestReader>(
        outputArgs, "echo", request, nullptr);
    EXPECT_EQ(*input.sequence(), *output.sequence());
    EXPECT_EQ(*input.text(), *output.text());
    EXPECT_EQ(*input.ordinaryBinary(), *output.ordinaryBinary());
    EXPECT_EQ(*input.choice()->text(), *output.choice()->text());
    EXPECT_EQ(
        chainString(*input.chainBinary()), chainString(*output.chainBinary()));
  };
  decode(apache::thrift::SerializedRequest{serialized->clone()});

  IOBufChain chain;
  const auto bytes = serialized->toString();
  for (char byte : bytes) {
    chain.append(folly::IOBuf::copyBuffer(&byte, 1));
  }
  apache::thrift::SerializedRequest chainRequest{std::move(chain)};
  Payload output;
  ChainService_echo_pargs outputArgs;
  outputArgs.get<0>().value = &output;
  EXPECT_THROW(
      (RequestDeserializer::deserializeRequest<Reader, ChainService_echo_pargs>(
          outputArgs, "echo", chainRequest, nullptr)),
      apache::thrift::TrustedServerException);
  if constexpr (std::is_same_v<Reader, BinaryProtocolReader>) {
    decode(std::move(chainRequest));
  } else {
    EXPECT_THROW(
        (RequestDeserializer::deserializeRequest<
            Reader,
            ChainService_echo_pargs,
            apache::thrift::detail::IOBufChainRequestReader>(
            outputArgs, "echo", chainRequest, nullptr)),
        apache::thrift::TrustedServerException);
  }
}

TEST(
    IOBufChainRpcCodegenTest,
    SelectsBinaryReaderFromRequestBufferRepresentation) {
  testRequestReaderSelection<BinaryProtocolReader, BinaryProtocolWriter>();
}

TEST(IOBufChainRpcCodegenTest, RejectsChainRequestForCompactProtocol) {
  testRequestReaderSelection<CompactProtocolReader, CompactProtocolWriter>();
}

} // namespace
