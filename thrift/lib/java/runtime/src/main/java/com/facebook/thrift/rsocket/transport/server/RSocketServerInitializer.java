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

import com.facebook.swift.service.ThriftServerConfig;
import com.facebook.thrift.metadata.ThriftTransportType;
import com.facebook.thrift.rsocket.transport.NettyDuplexConnection;
import com.facebook.thrift.util.RpcServerUtils;
import io.netty.buffer.ByteBuf;
import io.netty.channel.Channel;
import io.netty.channel.ChannelInitializer;
import io.netty.handler.logging.LogLevel;
import io.netty.handler.logging.LoggingHandler;
import io.netty.handler.ssl.OptionalSslHandler;
import io.netty.handler.ssl.SslContext;
import io.rsocket.transport.ServerTransport;
import reactor.core.Disposable;
import reactor.core.publisher.DirectProcessor;
import reactor.core.publisher.FluxProcessor;
import reactor.core.publisher.MonoProcessor;

public class RSocketServerInitializer extends ChannelInitializer<Channel> {
  final MonoProcessor<Void> onClose;
  final ThriftServerConfig config;
  final ServerTransport.ConnectionAcceptor acceptor;

  public RSocketServerInitializer(
      ThriftServerConfig config,
      ServerTransport.ConnectionAcceptor acceptor,
      MonoProcessor<Void> onClose) {
    this.config = config;
    this.acceptor = acceptor;
    this.onClose = onClose;
  }

  @Override
  protected void initChannel(Channel ch) {
    final FluxProcessor<ByteBuf, ByteBuf> receiverProcessor = DirectProcessor.create();

    onClose
        .doFinally(
            __ -> {
              if (ch.isOpen()) {
                ch.close();
              }
            })
        .subscribe();

    ch.pipeline().addLast(new LoggingHandler(LogLevel.TRACE));
    if (config.isSslEnabled() && !config.isEnableUDS()) {
      SslContext sslContext = RpcServerUtils.getSslContext(config);
      ch.pipeline()
          .addLast(
              "ssl",
              config.isAllowPlaintext()
                  ? new OptionalSslHandler(sslContext)
                  : sslContext.newHandler(ch.alloc()));
      if (config.isEnableAlpn()) {
        ch.pipeline()
            .addLast(
                "alpn",
                new FacebookApplicationProtocolNegotiationHandler(
                    ThriftTransportType.RSOCKET.protocol(), receiverProcessor));
      } else {
        configureRSocket(ch.pipeline(), receiverProcessor);
      }
    } else {
      configureRSocket(ch.pipeline(), receiverProcessor);
    }

    NettyDuplexConnection duplexConnection =
        new NettyDuplexConnection(ch, receiverProcessor, 32, 8);

    Disposable subscribe = acceptor.apply(duplexConnection).subscribe();

    ch.closeFuture()
        .addListener(
            future -> {
              if (!subscribe.isDisposed()) {
                subscribe.dispose();
              }
            });
  }
}
