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

package com.facebook.thrift.util;

import io.netty.channel.ChannelFuture;
import io.netty.util.concurrent.Future;
import io.netty.util.concurrent.GenericFutureListener;
import java.util.Objects;
import java.util.concurrent.CancellationException;
import reactor.core.publisher.Mono;
import reactor.core.publisher.MonoSink;

public final class NettyUtil {
  /**
   * Converts a Netty {@link ChannelFuture} to a reactor-core Mono&lt;Void&gt;
   *
   * @param future the future you want to convert
   * @return mono representing the ChannelFuture.
   */
  public static Mono<Void> toMono(ChannelFuture future) {
    try {
      Objects.requireNonNull(future, "future");
      if (future.isDone()) {
        if (future.isSuccess()) {
          return Mono.empty();
        } else if (future.isCancelled()) {
          return Mono.error(new CancellationException());
        } else {
          return Mono.error(future.cause());
        }
      }

      return Mono.create(
          sink -> {
            if (future.isDone()) {
              handleFutureForSink(sink, future);
            } else {
              GenericFutureListener<Future<? super Void>> listener =
                  result -> handleFutureForSink(sink, result);

              future.addListener(listener);
              sink.onDispose(
                  () -> {
                    future.removeListener(listener);
                    future.cancel(true);
                  });
            }
          });
    } catch (Throwable t) {
      return Mono.error(t);
    }
  }

  /**
   * This methods maps the events from a Netty {@link Future} and maps them to a reactor-core {@link
   * MonoSink}.
   *
   * @param sink The sink to receive events
   * @param future The future that produced the events
   */
  private static void handleFutureForSink(MonoSink<Void> sink, Future<? super Void> future) {
    if (future.isSuccess()) {
      sink.success();
    } else if (future.isCancelled()) {
      sink.error(new CancellationException());
    } else {
      sink.error(future.cause());
    }
  }
}
