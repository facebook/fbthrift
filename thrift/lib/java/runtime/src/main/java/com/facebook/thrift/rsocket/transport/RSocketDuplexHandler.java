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
import io.netty.channel.ChannelDuplexHandler;
import io.netty.channel.ChannelHandlerContext;
import io.netty.channel.ChannelPromise;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import reactor.core.publisher.FluxProcessor;

public class RSocketDuplexHandler extends ChannelDuplexHandler {
  private static final Logger logger = LoggerFactory.getLogger(RSocketDuplexHandler.class);
  private final FluxProcessor<ByteBuf, ByteBuf> receiverProcessor;

  public RSocketDuplexHandler(FluxProcessor<ByteBuf, ByteBuf> receiverProcessor) {
    this.receiverProcessor = receiverProcessor;
  }

  @Override
  public void channelRead(ChannelHandlerContext ctx, Object msg) throws Exception {
    if (msg instanceof ByteBuf) {
      ByteBuf frame = (ByteBuf) msg;
      receiverProcessor.onNext(frame);
    } else {
      if (logger.isDebugEnabled()) {
        logger.debug("unexpected message type: " + msg.getClass());
      }
    }
  }

  @Override
  public void write(ChannelHandlerContext ctx, Object msg, ChannelPromise promise)
      throws Exception {
    if (msg instanceof ByteBuf) {
      ByteBuf frame = (ByteBuf) msg;
      ByteBuf size = ctx.alloc().buffer();
      size.writeMedium(frame.readableBytes());
      ctx.write(size);
      ctx.writeAndFlush(frame, promise);
    }
  }

  @Override
  public void exceptionCaught(ChannelHandlerContext ctx, Throwable cause) throws Exception {
    logger.error("RSocketDuplexHandler received uncaught exception", cause);
    receiverProcessor.onError(cause);
  }
}
