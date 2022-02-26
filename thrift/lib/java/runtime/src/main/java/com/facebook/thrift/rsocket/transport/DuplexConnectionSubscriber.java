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

package com.facebook.thrift.rsocket.transport;

import io.netty.buffer.ByteBuf;
import io.netty.channel.Channel;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelPromise;
import io.netty.channel.EventLoop;
import io.netty.util.ReferenceCountUtil;
import io.netty.util.concurrent.PromiseCombiner;
import io.netty.util.internal.PlatformDependent;
import java.util.Queue;
import java.util.concurrent.atomic.AtomicIntegerFieldUpdater;
import org.reactivestreams.Subscriber;
import org.reactivestreams.Subscription;
import reactor.core.Exceptions;
import reactor.core.publisher.MonoSink;
import reactor.core.publisher.Operators;
import reactor.util.context.Context;

class DuplexConnectionSubscriber implements Subscriber<ByteBuf> {
  @SuppressWarnings("rawtypes")
  static final AtomicIntegerFieldUpdater<DuplexConnectionSubscriber> WIP =
      AtomicIntegerFieldUpdater.newUpdater(DuplexConnectionSubscriber.class, "wip");

  private final Queue<ByteBuf> queue;
  private final Channel channel;
  private final EventLoop eventLoop;
  private final int highWaterMark;
  private final int lowWaterMark;
  private final MonoSink<Void> sink;

  private boolean terminated;
  private boolean complete;
  private long pending;
  private long requested;
  private Subscription subscription;

  private volatile int wip;

  public DuplexConnectionSubscriber(
      final Channel channel,
      final int highWaterMark,
      final int lowWaterMark,
      final MonoSink<Void> sink) {
    this.queue = PlatformDependent.newFixedMpscQueue(highWaterMark);
    this.channel = channel;
    this.eventLoop = channel.eventLoop();
    this.highWaterMark = highWaterMark;
    this.lowWaterMark = lowWaterMark;
    this.sink = sink;
  }

  @Override
  public void onSubscribe(Subscription s) {
    this.subscription = s;
    this.requested = highWaterMark;

    subscription.request(highWaterMark);

    tryDrain();
  }

  @Override
  public void onNext(ByteBuf t) {
    if (!queue.offer(t)) {
      Throwable ex =
          Operators.onOperatorError(null, Exceptions.failWithOverflow(), t, Context.empty());
      onError(Operators.onOperatorError(null, ex, t, Context.empty()));
      ReferenceCountUtil.safeRelease(t);
      return;
    }
    tryDrain();
  }

  @Override
  public void onError(Throwable t) {
    terminated = true;
    sink.error(t);
  }

  @Override
  public void onComplete() {
    if (!eventLoop.inEventLoop()) {
      eventLoop.execute(this::onComplete);
    }

    complete = true;
    tryDrain();
  }

  private void doComplete() {
    terminated = true;
    sink.success();
  }

  private void tryDrain() {
    if (WIP.getAndIncrement(this) == 0) {
      eventLoop.execute(this::drain);
    }
  }

  private void drain() {
    try {
      int missed = 1;
      do {
        long e = 0L;
        int p = 0;
        PromiseCombiner combiner = null;
        while (requested != e) {
          if (terminated) {
            return;
          }

          if (complete) {
            break;
          }

          ByteBuf buffer = queue.poll();
          if (buffer == null) {
            break;
          }

          p++;
          ChannelFuture write = channel.write(buffer);

          if (combiner == null) {
            combiner = new PromiseCombiner(eventLoop);
          }

          combiner.add(write);

          e++;
        }

        if (combiner != null) {
          ChannelPromise promise = channel.newPromise();
          combiner.finish(promise);

          final long emitted = e;
          final int pending = p;
          promise.addListener(
              result -> {
                if (result.cause() != null) {
                  onError(result.cause());
                  return;
                }

                this.pending -= pending;
                requested -= emitted;
                if (requested <= lowWaterMark && !complete) {
                  long n = highWaterMark - requested;
                  requested = highWaterMark;
                  eventLoop.execute(() -> subscription.request(n));
                }

                if (complete && pending == 0) {
                  tryDrain();
                }
              });
        }

        if (complete && pending == 0) {
          doComplete();
        }

        missed = WIP.addAndGet(this, -missed);
      } while (missed != 0);
    } catch (Throwable t) {
      onError(t);
    }
  }
}
