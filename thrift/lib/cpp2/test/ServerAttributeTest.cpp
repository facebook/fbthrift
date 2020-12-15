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

#include <thrift/lib/cpp2/server/ServerAttribute.h>
#include <folly/portability/GTest.h>

using namespace apache::thrift;

TEST(ServerAttributeTest, baselineFirst) {
  ServerAttribute<int> a{0};
  EXPECT_EQ(a.get(), 0);

  a.set(1, AttributeSource::BASELINE);
  EXPECT_EQ(a.get(), 1);
  a.set(2, AttributeSource::OVERRIDE);
  EXPECT_EQ(a.get(), 2);

  a.unset(AttributeSource::OVERRIDE);
  EXPECT_EQ(a.get(), 1);
  a.unset(AttributeSource::BASELINE);
  EXPECT_EQ(a.get(), 0);
}

TEST(ServerAttributeTest, overrideFirst) {
  ServerAttribute<int> a{0};
  EXPECT_EQ(a.get(), 0);

  a.set(2, AttributeSource::OVERRIDE);
  EXPECT_EQ(a.get(), 2);
  a.set(1, AttributeSource::BASELINE);
  // still return overrided value
  EXPECT_EQ(a.get(), 2);

  a.unset(AttributeSource::BASELINE);
  // still return overrided value
  EXPECT_EQ(a.get(), 2);
  a.unset(AttributeSource::OVERRIDE);
  EXPECT_EQ(a.get(), 0);
}

TEST(ServerAttributeTest, stringBaselineFirst) {
  ServerAttribute<std::string> a{"a"};
  EXPECT_EQ(a.get(), "a");

  a.set("b", AttributeSource::BASELINE);
  EXPECT_EQ(a.get(), "b");
  a.set("c", AttributeSource::OVERRIDE);
  EXPECT_EQ(a.get(), "c");

  a.unset(AttributeSource::OVERRIDE);
  EXPECT_EQ(a.get(), "b");
  a.unset(AttributeSource::BASELINE);
  EXPECT_EQ(a.get(), "a");
}

TEST(ServerAttributeTest, stringOverrideFirst) {
  ServerAttribute<std::string> a{"a"};
  EXPECT_EQ(a.get(), "a");

  a.set("c", AttributeSource::OVERRIDE);
  EXPECT_EQ(a.get(), "c");
  a.set("b", AttributeSource::BASELINE);
  // still return overrided value
  EXPECT_EQ(a.get(), "c");

  a.unset(AttributeSource::BASELINE);
  // still return overrided value
  EXPECT_EQ(a.get(), "c");
  a.unset(AttributeSource::OVERRIDE);
  EXPECT_EQ(a.get(), "a");
}

TEST(ServerAttribute, Observable) {
  folly::observer::SimpleObservable<std::string> defaultObservable{"default"};
  folly::observer::SimpleObservable<std::string> baselineObservable{"baseline"};
  folly::observer::SimpleObservable<std::string> overrideObservable{"override"};
  detail::ServerAttributeObservable<std::string> attr{
      defaultObservable.getObserver()};
  auto observer = attr.getObserver();

  attr.set(baselineObservable.getObserver(), AttributeSource::BASELINE);
  EXPECT_EQ(**observer, "baseline");

  attr.set(overrideObservable.getObserver(), AttributeSource::OVERRIDE);
  EXPECT_EQ(**observer, "override");

  overrideObservable.setValue("override 2");
  folly::observer::waitForAllUpdates();
  EXPECT_EQ(**observer, "override 2");

  baselineObservable.setValue("baseline 2");
  folly::observer::waitForAllUpdates();
  attr.unset(AttributeSource::OVERRIDE);
  EXPECT_EQ(**observer, "baseline 2");

  attr.set("baseline 3", AttributeSource::BASELINE);
  EXPECT_EQ(**observer, "baseline 3");

  defaultObservable.setValue("default 2");
  folly::observer::waitForAllUpdates();
  attr.unset(AttributeSource::BASELINE);
  EXPECT_EQ(**observer, "default 2");
}

TEST(ServerAttribute, Atomic) {
  ServerAttributeAtomic<int> attr{42};
  auto observer = attr.getAtomicObserver();
  EXPECT_EQ(*observer, 42);

  attr.set(24, AttributeSource::BASELINE);
  EXPECT_EQ(*observer, 24);

  attr.set(12, AttributeSource::OVERRIDE);
  EXPECT_EQ(*observer, 12);

  attr.unset(AttributeSource::OVERRIDE);
  EXPECT_EQ(*observer, 24);
}

TEST(ServerAttribute, ThreadLocal) {
  ServerAttributeThreadLocal<int> attr{42};
  auto observer = attr.getTLObserver();
  EXPECT_EQ(**observer, 42);

  attr.set(24, AttributeSource::BASELINE);
  EXPECT_EQ(**observer, 24);

  attr.set(12, AttributeSource::OVERRIDE);
  EXPECT_EQ(**observer, 12);

  attr.unset(AttributeSource::OVERRIDE);
  EXPECT_EQ(**observer, 24);
}
