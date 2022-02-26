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

package com.facebook.thrift.rsocket.transport.client;

import static com.facebook.thrift.util.RpcClientUtils.getChannelClass;
import static com.facebook.thrift.util.RpcClientUtils.getSslContext;
import static io.rsocket.frame.FrameLengthCodec.FRAME_LENGTH_MASK;
import static io.rsocket.frame.FrameLengthCodec.FRAME_LENGTH_SIZE;

import com.facebook.thrift.client.ThriftClientConfig;
import com.facebook.thrift.metadata.ThriftTransportType;
import com.facebook.thrift.rsocket.transport.NettyDuplexConnection;
import com.facebook.thrift.rsocket.transport.RSocketDuplexHandler;
import com.facebook.thrift.util.NettyUtil;
import com.facebook.thrift.util.resources.RpcResources;
import io.netty.bootstrap.Bootstrap;
import io.netty.buffer.ByteBuf;
import io.netty.channel.ChannelInitializer;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.socket.DuplexChannel;
import io.netty.handler.codec.LengthFieldBasedFrameDecoder;
import io.netty.handler.flush.FlushConsolidationHandler;
import io.netty.handler.ssl.SslContext;
import io.rsocket.DuplexConnection;
import io.rsocket.transport.ClientTransport;
import java.net.SocketAddress;
import reactor.core.publisher.DirectProcessor;
import reactor.core.publisher.FluxProcessor;
import reactor.core.publisher.Mono;
import reactor.core.publisher.MonoProcessor;

public class NettyClientTransport implements ClientTransport {
  private final SocketAddress socketAddress;
  private final EventLoopGroup eventLoopGroup;
  private final SslContext sslContext;

  public NettyClientTransport(SocketAddress socketAddress, ThriftClientConfig config) {
    this.socketAddress = socketAddress;
    this.eventLoopGroup = RpcResources.getEventLoopGroup();
    this.sslContext = getSslContext(config, socketAddress);
  }

  public NettyClientTransport(
      SocketAddress socketAddress, ThriftClientConfig config, ThriftTransportType transportType) {
    this.socketAddress = socketAddress;
    this.eventLoopGroup = RpcResources.getEventLoopGroup();
    this.sslContext = getSslContext(config, socketAddress, transportType);
  }

  @Override
  public Mono<DuplexConnection> connect() {
    try {
      MonoProcessor<DuplexConnection> processor = MonoProcessor.create();
      Bootstrap bootstrap = new Bootstrap();
      bootstrap.group(eventLoopGroup);
      bootstrap.channel(getChannelClass(eventLoopGroup, socketAddress));
      bootstrap.remoteAddress(socketAddress);
      bootstrap.handler(
          new ChannelInitializer<DuplexChannel>() {
            @Override
            protected void initChannel(DuplexChannel ch) {
              final FluxProcessor<ByteBuf, ByteBuf> receiverProcessor = DirectProcessor.create();

              if (sslContext != null) {
                ch.pipeline().addLast(sslContext.newHandler(ch.alloc()));
              }

              ch.pipeline()
                  .addLast(
                      new FlushConsolidationHandler(256, true),
                      new LengthFieldBasedFrameDecoder(
                          FRAME_LENGTH_MASK, 0, FRAME_LENGTH_SIZE, 0, 0),
                      new RSocketDuplexHandler(receiverProcessor));

              NettyDuplexConnection duplexConnection =
                  new NettyDuplexConnection(ch, receiverProcessor, 32, 8);

              processor.onNext(duplexConnection);
            }
          });

      return NettyUtil.toMono(bootstrap.connect()).then(processor);
    } catch (Throwable t) {
      return Mono.error(t);
    }
  }
}
