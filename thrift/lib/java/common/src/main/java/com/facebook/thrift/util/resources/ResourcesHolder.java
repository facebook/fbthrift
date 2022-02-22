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

import com.google.common.util.concurrent.ThreadFactoryBuilder;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.SingleThreadEventLoop;
import io.netty.channel.epoll.Epoll;
import io.netty.channel.epoll.EpollEventLoopGroup;
import io.netty.channel.kqueue.KQueue;
import io.netty.channel.kqueue.KQueueEventLoopGroup;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.util.HashedWheelTimer;
import io.netty.util.ResourceLeakDetector;
import io.netty.util.concurrent.EventExecutor;
import io.netty.util.concurrent.FastThreadLocalThread;
import java.util.Map;
import java.util.concurrent.ThreadFactory;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import reactor.core.publisher.Mono;
import reactor.core.publisher.MonoProcessor;
import reactor.core.scheduler.NonBlocking;

class ResourcesHolder {
  private static final Logger LOGGER = LoggerFactory.getLogger(ResourcesHolder.class);

  private final boolean forceExecutionOffEventLoop =
      System.getProperty("thrift.force-execution-off-eventloop", "true").equalsIgnoreCase("true");

  private final int maxPendingTasksForOffLoop =
      Math.max(16, Integer.getInteger("thrift.pending-tasks.count", Integer.MAX_VALUE));

  private final int numThreadsForOffLoop =
      Math.max(
          Runtime.getRuntime().availableProcessors(),
          Integer.getInteger(
              "thrift.executor-threads.count", Runtime.getRuntime().availableProcessors() * 5));

  private final int numThreadsForEventLoop =
      Math.max(
          1,
          Integer.getInteger(
              "thrift.eventloop-threads.count", Runtime.getRuntime().availableProcessors() * 2));

  private final MonoProcessor<Void> onClose = MonoProcessor.create();

  private final HashedWheelTimer timer;
  private final EventLoopGroup eventLoopGroup;
  private final ThreadPoolScheduler offLoopScheduler;
  private final ThreadPoolScheduler clientOffLoopScheduler;

  public ResourcesHolder() {
    this.timer = createHashedWheelTimer();
    this.eventLoopGroup = createEventLoopGroup();
    this.offLoopScheduler = createOffLoopScheduler();
    boolean separateOffLoopScheduler =
        System.getProperty("thrift.separate-offloop-scheduler", "false").equalsIgnoreCase("true");
    this.clientOffLoopScheduler =
        separateOffLoopScheduler ? createClientOffLoopScheduler() : offLoopScheduler;

    // If system properties does not contain leak detection, disable it
    if (!System.getProperties().contains("io.netty.leakDetectionLevel")) {
      ResourceLeakDetector.setLevel(ResourceLeakDetector.Level.DISABLED);
    }
  }

  public HashedWheelTimer getTimer() {
    return timer;
  }

  public EventLoopGroup getEventLoopGroup() {
    return eventLoopGroup;
  }

  public ThreadPoolScheduler getOffLoopScheduler() {
    return offLoopScheduler;
  }

  public ThreadPoolScheduler getClientOffLoopScheduler() {
    return clientOffLoopScheduler;
  }

  private HashedWheelTimer createHashedWheelTimer() {
    ThreadFactory threadFactory =
        new ThreadFactoryBuilder()
            .setDaemon(true)
            .setUncaughtExceptionHandler(
                (t, e) -> LOGGER.error("uncaught exception on thread {}", t.getName(), e))
            .setNameFormat("thrift-timer-%d")
            .build();

    return new HashedWheelTimer(threadFactory);
  }

  private void shutdownHashedWheelTimer() {
    timer.stop();
  }

  private EventLoopGroup createEventLoopGroup() {
    EventLoopGroup eventLoop;
    if (Epoll.isAvailable()) {
      eventLoop = new EpollEventLoopGroup(numThreadsForEventLoop, daemonThreadFactory());
    } else if (KQueue.isAvailable()) {
      eventLoop = new KQueueEventLoopGroup(numThreadsForEventLoop, daemonThreadFactory());
    } else {
      eventLoop = new NioEventLoopGroup(numThreadsForEventLoop, daemonThreadFactory());
    }
    LOGGER.info("Using '{}' with '{}' threads.", eventLoop.getClass(), numThreadsForEventLoop);
    return eventLoop;
  }

  private void shutdownEventLoopGroup() {
    try {
      eventLoopGroup.shutdownGracefully().get();
    } catch (Exception e) {
      LOGGER.error("Error closing event loop", e);
      throw new IllegalStateException("Exception shutting down event loop", e);
    }
  }

  private ThreadPoolScheduler createOffLoopScheduler() {
    LOGGER.info("force execution off event loop enabled:  {}", forceExecutionOffEventLoop);
    LOGGER.info("off event loop max threads: {}", numThreadsForOffLoop);
    LOGGER.info("off event loop max pending tasks: {}", maxPendingTasksForOffLoop);
    return new ThreadPoolScheduler(numThreadsForOffLoop, maxPendingTasksForOffLoop);
  }

  private ThreadPoolScheduler createClientOffLoopScheduler() {
    LOGGER.info("creating separate off event loop scheduler for client");
    LOGGER.info("force client execution off event loop enabled:  {}", forceExecutionOffEventLoop);
    LOGGER.info("client off event loop max threads: {}", numThreadsForOffLoop);
    LOGGER.info("client off event loop max pending tasks: {}", maxPendingTasksForOffLoop);
    return new ThreadPoolScheduler(numThreadsForOffLoop, maxPendingTasksForOffLoop);
  }

  protected void shutdownOffLoopScheduler() {
    try {
      offLoopScheduler.dispose();
    } catch (Exception e) {
      LOGGER.error("Error closing event loop", e);
      throw new IllegalStateException("Exception shutting down event loop", e);
    }
  }

  public int getNumThreadsForEventLoop() {
    return numThreadsForEventLoop;
  }

  public int pendingTasksForEventLoop() {
    int pending = 0;
    for (EventExecutor eventExecutor : eventLoopGroup) {
      SingleThreadEventLoop loop = (SingleThreadEventLoop) eventExecutor;
      pending += loop.pendingTasks();
    }

    return pending;
  }

  public boolean isForceExecutionOffEventLoop() {
    return forceExecutionOffEventLoop;
  }

  public Map<String, Long> stats() {
    return offLoopScheduler.getStats();
  }

  private static ThreadFactory daemonThreadFactory() {
    return new ThreadFactoryBuilder()
        .setNameFormat("thrift-eventloop-%d")
        .setDaemon(true)
        .setUncaughtExceptionHandler(
            (t, e) -> LOGGER.error("uncaught exception on thread {}", t.getName(), e))
        .setThreadFactory(ThriftEventLoopThread::new)
        .build();
  }

  public Mono<Void> onClose() {
    return onClose;
  }

  public void dispose() {
    try {
      shutdownHashedWheelTimer();
      shutdownEventLoopGroup();
      shutdownOffLoopScheduler();
    } finally {
      onClose.onComplete();
    }
  }

  /**
   * Thread used by Thrift for a Netty Eventloop. It immplements {@link FastThreadLocalThread} which
   * is special thread local used by Netty 4 esp for ByteBufs. It also is marked with {@
   * NonBlocking} interface from reactor-core. This means that if someone tries to call
   * block()/blockFirst()/blockLast() methods, which could block the eventloop, it cause an
   * exception to thrown preventing this.
   */
  private static class ThriftEventLoopThread extends FastThreadLocalThread implements NonBlocking {
    ThriftEventLoopThread(Runnable r) {
      super(r);
    }
  }
}
