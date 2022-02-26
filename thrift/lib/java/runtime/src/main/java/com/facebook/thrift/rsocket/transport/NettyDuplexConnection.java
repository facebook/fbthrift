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
import io.netty.buffer.ByteBufAllocator;
import io.netty.channel.Channel;
import io.netty.channel.EventLoop;
import io.rsocket.DuplexConnection;
import org.reactivestreams.Publisher;
import reactor.core.publisher.Flux;
import reactor.core.publisher.FluxProcessor;
import reactor.core.publisher.Mono;
import reactor.core.publisher.MonoProcessor;
import reactor.core.scheduler.Scheduler;
import reactor.core.scheduler.Schedulers;

public class NettyDuplexConnection implements DuplexConnection {
  private final Channel channel;
  private final EventLoop eventLoop;
  private final Scheduler scheduler;
  private final FluxProcessor<ByteBuf, ByteBuf> receiverProcessor;
  private final MonoProcessor<Void> onClose;
  private final int highWaterMark;
  private final int lowWaterMark;

  public NettyDuplexConnection(
      final Channel channel,
      final FluxProcessor<ByteBuf, ByteBuf> receiverProcessor,
      final int highWaterMark,
      final int lowWaterMark) {
    this.channel = channel;
    this.eventLoop = channel.eventLoop();
    this.scheduler = Schedulers.fromExecutor(eventLoop);
    this.onClose = MonoProcessor.create();
    this.receiverProcessor = receiverProcessor;
    this.highWaterMark = highWaterMark;
    this.lowWaterMark = lowWaterMark;

    Util.toMono(channel.closeFuture()).subscribe(onClose);

    onClose
        .doFinally(
            __ -> {
              if (channel.isOpen()) {
                channel.close();
              }
            })
        .subscribe();
  }

  @Override
  public Mono<Void> send(Publisher<ByteBuf> frames) {
    Mono<Void> send =
        Mono.create(
            sink ->
                frames.subscribe(
                    new DuplexConnectionSubscriber(channel, highWaterMark, lowWaterMark, sink)));
    return eventLoop.inEventLoop() ? send : send.subscribeOn(scheduler);
  }

  @Override
  public Mono<Void> sendOne(ByteBuf frame) {
    final Mono<Void> send = Util.toMono(channel.writeAndFlush(frame));
    return eventLoop.inEventLoop() ? send : send.subscribeOn(scheduler);
  }

  @Override
  public Flux<ByteBuf> receive() {
    return receiverProcessor.map(byteBuf -> byteBuf.skipBytes(3));
  }

  @Override
  public ByteBufAllocator alloc() {
    return channel.alloc();
  }

  @Override
  public Mono<Void> onClose() {
    return onClose;
  }

  @Override
  public void dispose() {
    onClose.onComplete();
  }

  @Override
  public double availability() {
    return channel.isActive() ? 1 : 0;
  }

  @Override
  public boolean isDisposed() {
    return onClose.isDisposed();
  }
}
