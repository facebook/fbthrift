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

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/rust/RustTailEndpoint.h>

#include <memory>
#include <vector>

#include <folly/ExceptionWrapper.h>
#include <folly/io/IOBuf.h>
#include <folly/io/async/EventBase.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/BufferAllocator.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/PipelineBuilder.h>
#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/test/MockAdapters.h>

namespace channel_pipeline_rust::test {
namespace {

namespace cp = apache::thrift::fast_thrift::channel_pipeline;

TEST(RustTailEndpointTest, DelegatesDataAndLifecycle) {
  rust_tail_endpoint_reset_test_counts();

  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_echo_test()};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();

  pipeline->activate();
  auto bytes = folly::IOBuf::copyBuffer("tail");
  EXPECT_EQ(
      pipeline->fireRead(cp::erase_and_box(std::move(bytes))),
      cp::Result::Success);
  EXPECT_EQ(head.writeCount(), 1);
  ASSERT_EQ(head.writtenBytes().size(), 1);
  EXPECT_EQ(head.writtenBytes().front()->moveToFbString(), "tail");

  pipeline->onWriteReady();
  pipeline->fireException(
      folly::make_exception_wrapper<std::runtime_error>("test"));
  pipeline->deactivate();
  pipeline->close();

  const auto counts = rust_tail_endpoint_test_counts();
  const std::vector<uint32_t> expected{1, 1, 1, 1, 1, 1, 1};
  EXPECT_EQ(std::vector<uint32_t>(counts.begin(), counts.end()), expected);
}

TEST(RustTailEndpointTest, QueuedReadRunsLaterInTheCurrentLoopIteration) {
  rust_tail_endpoint_reset_test_counts();

  folly::EventBase eventBase;
  cp::test::MockHeadHandler head;
  cp::SimpleBufferAllocator allocator;
  RustTailEndpoint tail{rust_tail_endpoint_new_queued_test()};
  auto pipeline = cp::PipelineBuilder<
                      cp::test::MockHeadHandler,
                      RustTailEndpoint,
                      cp::SimpleBufferAllocator>()
                      .setEventBase(&eventBase)
                      .setHead(&head)
                      .setTail(&tail)
                      .setAllocator(&allocator)
                      .build();

  pipeline->activate();
  eventBase.runInLoop([&] {
    EXPECT_EQ(
        pipeline->fireRead(
            cp::erase_and_box(folly::IOBuf::copyBuffer("queued"))),
        cp::Result::Success);
    EXPECT_EQ(rust_tail_endpoint_queued_test_completions(), 0);
  });

  eventBase.loopOnce(EVLOOP_NONBLOCK);

  EXPECT_EQ(rust_tail_endpoint_queued_test_completions(), 1);
  pipeline->deactivate();
  pipeline->close();
}

} // namespace
} // namespace channel_pipeline_rust::test
