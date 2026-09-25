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

package com.facebook.thrift.util.resources;

import static org.junit.jupiter.api.Assertions.assertFalse;
import static org.junit.jupiter.api.Assertions.assertSame;
import static org.junit.jupiter.api.Assertions.assertTrue;

import org.junit.jupiter.api.AfterEach;
import org.junit.jupiter.api.Test;

public class TestRpcResources {
  @AfterEach
  public void tearDown() {
    RpcResources.shutdown();
  }

  @Test
  public void testSnapshotDoesNotInitializeClosedResources() {
    RpcResources.shutdown();

    assertFalse(RpcResources.tryGetResourceSnapshot().isPresent());
  }

  @Test
  public void testSnapshotReturnsCurrentResources() {
    ThriftScheduler scheduler = (ThriftScheduler) RpcResources.getOffLoopScheduler();

    RpcResources.ResourceSnapshot snapshot =
        RpcResources.tryGetResourceSnapshot().orElseThrow(AssertionError::new);

    assertSame(scheduler, snapshot.getOffLoopScheduler());
    assertTrue(snapshot.getNumEventLoopThreads() > 0);
    assertTrue(snapshot.getEventLoopGroupPendingTasks() >= 0);
  }
}
