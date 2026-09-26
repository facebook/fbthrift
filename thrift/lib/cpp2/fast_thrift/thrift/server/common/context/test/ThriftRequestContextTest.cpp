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

#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/context/ThriftRequestContext.h>

#include <gtest/gtest.h>

#include <memory>
#include <utility>

#include <folly/CancellationToken.h>

#include <thrift/lib/cpp2/fast_thrift/thrift/server/common/context/ThriftConnContext.h>

namespace apache::thrift::fast_thrift::thrift {

TEST(ThriftRequestContextTest, DefaultConstructedHasNoConnContext) {
  ThriftRequestContext rc;
  EXPECT_EQ(rc.getConnectionContext(), nullptr);
  EXPECT_FALSE(rc.isCancellationEnabled());
  EXPECT_FALSE(rc.getCancellationToken().canBeCancelled());
  EXPECT_FALSE(rc.requestCancellation());
}

TEST(ThriftRequestContextTest, SetConnectionContextStoresIt) {
  boost::intrusive_ptr<ThriftConnContext> conn{new ThriftConnContext()};
  conn->setSecurityProtocol("TLS1.3");

  ThriftRequestContext rc;
  rc.setConnectionContext(conn);
  ASSERT_EQ(rc.getConnectionContext(), conn.get());
  EXPECT_EQ(rc.getConnectionContext()->getSecurityProtocol(), "TLS1.3");
}

TEST(ThriftRequestContextTest, MethodNameIsEmptyUntilSet) {
  ThriftRequestContext rc;
  EXPECT_TRUE(rc.getMethodName().empty());

  rc.setMethodName("Service.method");
  EXPECT_EQ(rc.getMethodName(), "Service.method");
}

TEST(ThriftRequestContextTest, KeepsConnContextAliveAfterLocalReset) {
  boost::intrusive_ptr<ThriftConnContext> conn{new ThriftConnContext()};
  auto* raw = conn.get();

  ThriftRequestContext rc;
  rc.setConnectionContext(std::move(conn));
  EXPECT_EQ(raw->use_count(), 1);
  EXPECT_EQ(rc.getConnectionContext(), raw);
}

TEST(ThriftRequestContextTest, CancellationCallbackMayDestroyContext) {
  auto context = std::make_unique<ThriftRequestContext>();
  context->enableCancellation();
  auto* contextPtr = context.get();
  folly::CancellationCallback callback(
      context->getCancellationToken(), [&] { context.reset(); });

  EXPECT_TRUE(contextPtr->requestCancellation());
  EXPECT_EQ(context, nullptr);
}

} // namespace apache::thrift::fast_thrift::thrift
