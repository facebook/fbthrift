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
#include <thrift/lib/cpp2/IOBufChain.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/ChainService.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/ChainService.tcc>

namespace {

using apache::thrift::BinaryProtocolChainReader;
using apache::thrift::BinaryProtocolReader;
using apache::thrift::BinaryProtocolWriter;
using apache::thrift::FieldData;
using apache::thrift::IOBufChain;
using apache::thrift::test::iobuf_chain_rpc::ChainService_echo_pargs;
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

std::string chainString(const IOBufChain& chain) {
  std::string result;
  result.reserve(chain.chainLength());
  for (const auto& buffer : chain) {
    result.append(
        reinterpret_cast<const char*>(buffer.data()), buffer.length());
  }
  return result;
}

TEST(IOBufChainRpcCodegenTest, DecodesGeneratedPargsWithBothBinaryReaders) {
  Payload input;
  *input.sequence() = 123456789;
  *input.text() = "chain reader";
  *input.ordinaryBinary() = std::string{"ordinary binary"};
  *input.chainBinary() = IOBufChain{folly::IOBuf::copyBuffer("chain binary")};
  ChainService_echo_pargs inputArgs;
  inputArgs.get<0>().value = &input;

  folly::IOBufQueue queue;
  BinaryProtocolWriter writer;
  writer.setOutput(&queue);
  inputArgs.write(&writer);

  auto serialized = queue.move();
  auto legacySerialized = serialized->clone();
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
  EXPECT_EQ(
      chainString(*input.chainBinary()), chainString(*output.chainBinary()));

  BinaryProtocolReader legacyReader;
  legacyReader.setInput(legacySerialized.get());
  Payload legacyOutput;
  ChainService_echo_pargs legacyOutputArgs;
  legacyOutputArgs.get<0>().value = &legacyOutput;
  legacyOutputArgs.read(&legacyReader);

  EXPECT_EQ(*input.sequence(), *legacyOutput.sequence());
  EXPECT_EQ(*input.text(), *legacyOutput.text());
  EXPECT_EQ(*input.ordinaryBinary(), *legacyOutput.ordinaryBinary());
  EXPECT_EQ(
      chainString(*input.chainBinary()),
      chainString(*legacyOutput.chainBinary()));
}

} // namespace
