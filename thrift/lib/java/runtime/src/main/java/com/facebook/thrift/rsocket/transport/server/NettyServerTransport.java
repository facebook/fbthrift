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

import com.facebook.swift.service.ThriftServerConfig;
import com.facebook.thrift.rsocket.transport.Util;
import com.facebook.thrift.util.RpcServerUtils;
import com.facebook.thrift.util.resources.RpcResources;
import io.netty.bootstrap.ServerBootstrap;
import io.netty.channel.ChannelFuture;
import io.netty.channel.EventLoopGroup;
import io.netty.channel.unix.DomainSocketAddress;
import io.rsocket.transport.ServerTransport;
import java.net.InetSocketAddress;
import java.net.SocketAddress;
import java.util.Objects;
import reactor.core.publisher.Mono;
import reactor.core.publisher.MonoProcessor;

public class NettyServerTransport implements ServerTransport<NettyServerClosable> {
  final MonoProcessor<Void> onClose;
  final SocketAddress socketAddress;
  final EventLoopGroup eventLoopGroup;
  final ThriftServerConfig config;

  public NettyServerTransport(ThriftServerConfig config) {
    if (config.isEnableUDS()) {
      Objects.requireNonNull(config.getUdsPath(), "Null UDS path for UDS server");
      this.socketAddress = new DomainSocketAddress(config.getUdsPath());
    } else {
      this.socketAddress = new InetSocketAddress("localhost", config.getPort());
    }
    this.config = config;
    this.eventLoopGroup = RpcResources.getEventLoopGroup();
    this.onClose = MonoProcessor.create();
  }

  @Override
  public Mono<NettyServerClosable> start(final ConnectionAcceptor acceptor) {
    ServerBootstrap bootstrap = new ServerBootstrap();
    bootstrap.group(eventLoopGroup);
    bootstrap.channel(RpcServerUtils.getChannelClass(eventLoopGroup, socketAddress));
    RSocketServerInitializer initializer = new RSocketServerInitializer(config, acceptor, onClose);
    bootstrap.childHandler(initializer);
    ChannelFuture bind = bootstrap.bind(socketAddress);

    return Util.toMono(bind)
        .thenReturn(
            new NettyServerClosable() {
              @Override
              public SocketAddress getAddress() {
                return bind.channel().localAddress();
              }

              @Override
              public Mono<Void> onClose() {
                return onClose;
              }

              @Override
              public void dispose() {
                onClose.onComplete();
              }
            });
  }
}
