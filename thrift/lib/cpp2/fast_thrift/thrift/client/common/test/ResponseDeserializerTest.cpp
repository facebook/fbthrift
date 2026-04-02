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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <folly/io/IOBuf.h>
#include <thrift/lib/cpp/TApplicationException.h>
#include <thrift/lib/cpp2/fast_thrift/thrift/client/common/ResponseDeserializer.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>

namespace apache::thrift::fast_thrift::thrift {

namespace {

using ResultType = folly::Expected<int32_t, folly::exception_wrapper>;

std::unique_ptr<folly::IOBuf> serializePayloadData(int32_t value) {
  apache::thrift::CompactProtocolWriter writer;
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  writer.setOutput(&queue);
  writer.writeStructBegin("Result");
  writer.writeFieldBegin("value", apache::thrift::protocol::T_I32, 1);
  writer.writeI32(value);
  writer.writeFieldEnd();
  writer.writeFieldStop();
  writer.writeStructEnd();
  return queue.move();
}

ResultType readInt32Struct(apache::thrift::CompactProtocolReader& reader) {
  std::string fname;
  apache::thrift::protocol::TType ftype;
  int16_t fid;
  int32_t value = 0;
  reader.readStructBegin(fname);
  reader.readFieldBegin(fname, ftype, fid);
  reader.readI32(value);
  reader.readFieldEnd();
  reader.readFieldBegin(fname, ftype, fid);
  reader.readStructEnd();
  return value;
}

} // namespace

TEST(ResponseDeserializerTest, DeserializesValidBuffer) {
  auto buf = serializePayloadData(42);

  auto result = deserializeResponse<apache::thrift::CompactProtocolReader>(
      buf.get(), readInt32Struct);

  ASSERT_TRUE(result.hasValue());
  EXPECT_EQ(*result, 42);
}

TEST(ResponseDeserializerTest, MalformedBufferReturnsError) {
  auto buf = folly::IOBuf::copyBuffer("not valid thrift");

  auto result = deserializeResponse<apache::thrift::CompactProtocolReader>(
      buf.get(), readInt32Struct);

  ASSERT_TRUE(result.hasError());
  EXPECT_TRUE(result.error()
                  .is_compatible_with<apache::thrift::TApplicationException>());
  result.error().handle([](const apache::thrift::TApplicationException& ex) {
    EXPECT_THAT(
        std::string(ex.what()),
        testing::HasSubstr("Failed to deserialize response"));
  });
}

TEST(ResponseDeserializerTest, DeserializeFnErrorPreservesType) {
  auto buf = serializePayloadData(42);

  auto result = deserializeResponse<apache::thrift::CompactProtocolReader>(
      buf.get(), [](apache::thrift::CompactProtocolReader&) -> ResultType {
        return folly::makeUnexpected(
            folly::make_exception_wrapper<std::runtime_error>(
                "declared error"));
      });

  ASSERT_TRUE(result.hasError());
  EXPECT_TRUE(result.error().is_compatible_with<std::runtime_error>());
  result.error().handle([](const std::runtime_error& ex) {
    EXPECT_EQ(std::string(ex.what()), "declared error");
  });
}

TEST(ResponseDeserializerTest, DeserializeFnThrowIsCaught) {
  auto buf = serializePayloadData(42);

  auto result = deserializeResponse<apache::thrift::CompactProtocolReader>(
      buf.get(), [](apache::thrift::CompactProtocolReader&) -> ResultType {
        throw std::runtime_error("unexpected throw");
      });

  ASSERT_TRUE(result.hasError());
  EXPECT_TRUE(result.error()
                  .is_compatible_with<apache::thrift::TApplicationException>());
  result.error().handle([](const apache::thrift::TApplicationException& ex) {
    EXPECT_THAT(std::string(ex.what()), testing::HasSubstr("unexpected throw"));
  });
}

} // namespace apache::thrift::fast_thrift::thrift
