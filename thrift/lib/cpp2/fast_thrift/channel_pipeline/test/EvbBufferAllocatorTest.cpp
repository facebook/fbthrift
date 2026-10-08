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

#include <thrift/lib/cpp2/fast_thrift/channel_pipeline/EvbBufferAllocator.h>

#include <array>
#include <cstddef>
#include <cstring>

#include <folly/io/async/EventBase.h>
#include <folly/portability/GTest.h>

namespace apache::thrift::fast_thrift::channel_pipeline {
namespace {

TEST(EvbBufferAllocator, AllocatesPayloadAndControlBlockFromEventBasePages) {
  folly::EventBase eventBase;
  EvbBufferAllocator allocator{eventBase};

  BytesPtr bytes = allocator.allocate(1400);

  ASSERT_NE(bytes, nullptr);
  EXPECT_EQ(bytes->capacity(), 1400);
  EXPECT_EQ(bytes->length(), 0);
  auto* const page = mem::Page::fromObject(bytes.get());
  EXPECT_EQ(page->evb, &eventBase);
  EXPECT_EQ(mem::Page::fromObject(bytes->writableBuffer()), page);
  EXPECT_EQ(page->outstanding, 2);
  bytes.reset();
  EXPECT_EQ(page->outstanding, 0);
}

TEST(EvbBufferAllocator, CopiesPayloadWithoutHeapBackedIOBufStorage) {
  folly::EventBase eventBase;
  EvbBufferAllocator allocator{eventBase};
  constexpr std::array<std::byte, 4> expected{
      std::byte{1}, std::byte{2}, std::byte{3}, std::byte{4}};

  BytesPtr bytes = allocator.copyBuffer(expected.data(), expected.size());

  ASSERT_NE(bytes, nullptr);
  EXPECT_EQ(bytes->length(), expected.size());
  EXPECT_EQ(std::memcmp(bytes->data(), expected.data(), expected.size()), 0);
  EXPECT_EQ(mem::Page::fromObject(bytes.get())->evb, &eventBase);
  EXPECT_EQ(mem::Page::fromObject(bytes->writableBuffer())->evb, &eventBase);
}

TEST(EvbBufferAllocator, PoolsControlBlockForExternalStorage) {
  folly::EventBase eventBase;
  EvbBufferAllocator allocator{eventBase};
  size_t releases = 0;
  auto* const data = new std::byte[64];

  BytesPtr bytes = allocator.takeOwnership(
      OwnedBuffer{
          .data = data,
          .capacity = 64,
          .length = 64,
          .freeFn =
              [](void* buffer, void* userData) noexcept {
                delete[] static_cast<std::byte*>(buffer);
                ++*static_cast<size_t*>(userData);
              },
          .userData = &releases,
      });

  ASSERT_NE(bytes, nullptr);
  EXPECT_EQ(bytes->data(), reinterpret_cast<const uint8_t*>(data));
  auto* const page = mem::Page::fromObject(bytes.get());
  EXPECT_EQ(page->evb, &eventBase);
  EXPECT_EQ(page->outstanding, 1);
  bytes.reset();
  EXPECT_EQ(releases, 1);
  EXPECT_EQ(page->outstanding, 0);
}

} // namespace
} // namespace apache::thrift::fast_thrift::channel_pipeline
