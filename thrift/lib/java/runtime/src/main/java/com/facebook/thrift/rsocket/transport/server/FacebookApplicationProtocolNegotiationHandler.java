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

package com.facebook.thrift.rsocket.transport.server;

import static com.facebook.thrift.util.RpcServerUtils.configureRSocket;

import com.facebook.thrift.metadata.ThriftTransportType;
import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelHandlerContext;
import io.netty.handler.ssl.ApplicationProtocolNegotiationHandler;
import reactor.core.publisher.FluxProcessor;

public class FacebookApplicationProtocolNegotiationHandler
    extends ApplicationProtocolNegotiationHandler {
  private final FluxProcessor<ByteBuf, ByteBuf> receiverProcessor;

  public FacebookApplicationProtocolNegotiationHandler(
      String fallbackProtocol, FluxProcessor<ByteBuf, ByteBuf> receiverProcessor) {
    super(fallbackProtocol);
    this.receiverProcessor = receiverProcessor;
  }

  @Override
  protected void configurePipeline(ChannelHandlerContext ctx, String protocol) throws Exception {
    ThriftTransportType thriftTransport = ThriftTransportType.fromProtocol(protocol);
    if (thriftTransport == ThriftTransportType.RSOCKET) {
      configureRSocket(ctx.pipeline(), receiverProcessor);
    } else {
      throw new IllegalAccessException("unsupported type -> " + thriftTransport.protocol());
    }
  }
}
