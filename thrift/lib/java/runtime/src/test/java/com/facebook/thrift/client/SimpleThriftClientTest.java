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

package com.facebook.thrift.client;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.fail;

import com.facebook.swift.service.ThriftServerConfig;
import com.facebook.thrift.example.ping.PingRequest;
import com.facebook.thrift.example.ping.PingResponse;
import com.facebook.thrift.example.ping.PingService;
import com.facebook.thrift.example.ping.PingServiceRpcServerHandler;
import com.facebook.thrift.legacy.server.LegacyServerTransport;
import com.facebook.thrift.legacy.server.LegacyServerTransportFactory;
import com.facebook.thrift.legacy.server.testservices.BlockingPingService;
import com.facebook.thrift.server.RpcServerHandler;
import com.facebook.thrift.util.FutureUtil;
import com.facebook.thrift.util.SPINiftyMetrics;
import com.google.common.util.concurrent.ListenableFuture;
import io.airlift.units.Duration;
import java.net.InetSocketAddress;
import java.util.Collections;
import java.util.concurrent.TimeUnit;
import org.apache.thrift.ProtocolId;
import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.ExpectedException;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;
import reactor.core.publisher.Flux;

public class SimpleThriftClientTest {
  private static final Logger LOG = LoggerFactory.getLogger(SimpleThriftClientTest.class);

  @Rule public ExpectedException expectedException = ExpectedException.none();

  @Test
  public void testPingVoidBlocking() {
    System.out.println("create server handler");
    RpcServerHandler serverHandler =
        new PingServiceRpcServerHandler(new BlockingPingService(), Collections.emptyList());

    System.out.println("starting server");
    LegacyServerTransportFactory transportFactory =
        new LegacyServerTransportFactory(new ThriftServerConfig().setEnableJdkSsl(false));
    LegacyServerTransport transport = transportFactory.createServerTransport(serverHandler).block();
    InetSocketAddress address = (InetSocketAddress) transport.getAddress();

    LOG.info("creating client");

    RpcClientManager manager =
        new RpcClientManager(
            RpcClientFactory.builder()
                .setDisableLoadBalancing(true)
                .setThriftClientConfig(
                    new ThriftClientConfig()
                        .setDisableSSL(true)
                        .setRequestTimeout(Duration.succinctDuration(1, TimeUnit.DAYS)))
                .build(),
            address,
            ProtocolId.BINARY);

    PingService client = manager.createClient(PingService.class);
    client.pingVoid(new PingRequest.Builder().setRequest("ping").build());
  }

  @Test
  @Ignore
  public void testPingVoidAsync() throws Exception {
    System.out.println("create server handler");
    RpcServerHandler serverHandler =
        new PingServiceRpcServerHandler(new BlockingPingService(), Collections.emptyList());

    System.out.println("starting server");
    LegacyServerTransportFactory transportFactory =
        new LegacyServerTransportFactory(new ThriftServerConfig().setEnableJdkSsl(false));
    LegacyServerTransport transport = transportFactory.createServerTransport(serverHandler).block();
    InetSocketAddress address = (InetSocketAddress) transport.getAddress();

    LOG.info("creating client");

    RpcClientManager manager =
        new RpcClientManager(
            RpcClientFactory.builder()
                .setDisableLoadBalancing(true)
                .setThriftClientConfig(
                    new ThriftClientConfig()
                        .setDisableSSL(true)
                        .setRequestTimeout(Duration.succinctDuration(1, TimeUnit.DAYS)))
                .build(),
            address,
            ProtocolId.BINARY);

    PingService.Async client = manager.createClient(PingService.Async.class);
    client.pingVoid(new PingRequest.Builder().setRequest("ping").build()).get();
  }

  @Test
  public void testPingVoidReactive() {
    System.out.println("create server handler");
    RpcServerHandler serverHandler =
        new PingServiceRpcServerHandler(new BlockingPingService(), Collections.emptyList());

    System.out.println("starting server");
    LegacyServerTransportFactory transportFactory =
        new LegacyServerTransportFactory(new ThriftServerConfig().setEnableJdkSsl(false));
    LegacyServerTransport transport = transportFactory.createServerTransport(serverHandler).block();
    InetSocketAddress address = (InetSocketAddress) transport.getAddress();

    LOG.info("creating client");

    RpcClientManager manager =
        new RpcClientManager(
            RpcClientFactory.builder()
                .setDisableLoadBalancing(true)
                .setThriftClientConfig(
                    new ThriftClientConfig()
                        .setDisableSSL(true)
                        .setRequestTimeout(Duration.succinctDuration(1, TimeUnit.DAYS)))
                .build(),
            address,
            ProtocolId.BINARY);

    PingService.Reactive client = manager.createClient(PingService.Reactive.class);
    client.pingVoid(new PingRequest.Builder().setRequest("ping").build()).block();
  }

  @Test
  @Ignore
  public void testConnectionLimit() throws Throwable {
    System.out.println("create server handler");
    RpcServerHandler serverHandler =
        new PingServiceRpcServerHandler(new BlockingPingService(), Collections.emptyList());

    System.out.println("starting server");
    LegacyServerTransportFactory transportFactory =
        new LegacyServerTransportFactory(new ThriftServerConfig().setEnableJdkSsl(false));
    LegacyServerTransport transport = transportFactory.createServerTransport(serverHandler).block();
    InetSocketAddress address = (InetSocketAddress) transport.getAddress();
    SPINiftyMetrics metrics = transport.getNiftyMetrics();

    LOG.info("creating client");

    RpcClientManager manager =
        new RpcClientManager(
            RpcClientFactory.builder()
                .setDisableLoadBalancing(true)
                .setThriftClientConfig(
                    new ThriftClientConfig()
                        .setDisableSSL(true)
                        .setRequestTimeout(Duration.succinctDuration(1, TimeUnit.DAYS)))
                .build(),
            address,
            ProtocolId.BINARY);

    PingRequest pingRequest = new PingRequest.Builder().setRequest("ping").build();

    try {
      PingService open =
          manager.createClient(PingService.class, Collections.emptyMap(), Collections.emptyMap());
      open.ping(pingRequest);
      assertEquals(1, metrics.getChannelCount());
    } catch (Throwable t) {
      t.printStackTrace();
      fail("shouldn't get an exception here");
    }

    try {
      PingService closed =
          manager.createClient(PingService.class, Collections.emptyMap(), Collections.emptyMap());
      closed.ping(pingRequest);
    } catch (Throwable t) {
      assertEquals(metrics.getRejectedConnections(), 1);
      assertEquals(metrics.getChannelCount(), 1);

      throw t.getCause();
    }
  }

  @Test
  @Ignore
  public void testSchedulerRejection() throws Throwable {
    System.out.println("create server handler");
    RpcServerHandler serverHandler =
        new PingServiceRpcServerHandler(new BlockingPingService(), Collections.emptyList());

    System.out.println("starting server");
    LegacyServerTransportFactory transportFactory =
        new LegacyServerTransportFactory(new ThriftServerConfig().setEnableJdkSsl(false));
    LegacyServerTransport transport = transportFactory.createServerTransport(serverHandler).block();
    InetSocketAddress address = (InetSocketAddress) transport.getAddress();

    LOG.info("creating client");

    RpcClientManager manager =
        new RpcClientManager(
            RpcClientFactory.builder()
                .setDisableLoadBalancing(true)
                .setThriftClientConfig(
                    new ThriftClientConfig()
                        .setDisableSSL(true)
                        .setRequestTimeout(Duration.succinctDuration(1, TimeUnit.DAYS)))
                .build(),
            address,
            ProtocolId.BINARY);

    PingRequest pingRequest = new PingRequest.Builder().setRequest("ping").build();
    PingService pingService =
        manager.createClient(PingService.class, Collections.emptyMap(), Collections.emptyMap());
    try {
      pingService.ping(pingRequest);
    } catch (Throwable t) {
      throw t.getCause();
    }
  }

  @Test
  public void test1BlockingPingPongs() {
    sendNBlockingPingPongs(1);
  }

  @Test
  public void test10BlockingPingPongs() {
    sendNBlockingPingPongs(10);
  }

  @Test
  public void test100BlockingPingPongs() {
    sendNBlockingPingPongs(100);
  }

  @Test
  public void test1000BlockingPingPongs() {
    sendNBlockingPingPongs(1000);
  }

  @Test
  @Ignore
  public void test10_000BlockingPingPongs() {
    sendNBlockingPingPongs(10_000);
  }

  private void sendNBlockingPingPongs(int n) {
    System.out.println("create server handler");
    RpcServerHandler serverHandler =
        new PingServiceRpcServerHandler(new BlockingPingService(), Collections.emptyList());

    System.out.println("starting server");
    LegacyServerTransportFactory transportFactory =
        new LegacyServerTransportFactory(new ThriftServerConfig().setEnableJdkSsl(false));
    LegacyServerTransport transport = transportFactory.createServerTransport(serverHandler).block();
    InetSocketAddress address = (InetSocketAddress) transport.getAddress();

    LOG.info("creating client");

    RpcClientManager manager =
        new RpcClientManager(
            RpcClientFactory.builder()
                .setDisableLoadBalancing(true)
                .setThriftClientConfig(
                    new ThriftClientConfig()
                        .setDisableSSL(true)
                        .setRequestTimeout(Duration.succinctDuration(1, TimeUnit.DAYS)))
                .build(),
            address,
            ProtocolId.BINARY);

    PingService pingService =
        manager.createClient(PingService.class, Collections.emptyMap(), Collections.emptyMap());

    for (int i = 0; i < n; i++) {
      PingRequest pingRequest = new PingRequest.Builder().setRequest(i + "ping").build();
      PingResponse pingResponse = pingService.ping(pingRequest);
      assertEquals(pingResponse.getResponse(), i + "ping_pong_" + i);
    }
  }

  @Test
  public void test1AsyncPingPongs() {
    sendNAsyncPingPongs(1);
  }

  @Test
  public void test10AsyncPingPongs() {
    sendNAsyncPingPongs(10);
  }

  @Test
  public void test100AsyncPingPongs() {
    sendNAsyncPingPongs(100);
  }

  @Test
  public void test1000AsyncPingPongs() {
    sendNAsyncPingPongs(1000);
  }

  @Test
  public void test10_000AsyncPingPongs() {
    sendNAsyncPingPongs(10_000);
  }

  private void sendNAsyncPingPongs(int n) {
    System.out.println("create server handler");
    RpcServerHandler serverHandler =
        new PingServiceRpcServerHandler(new BlockingPingService(), Collections.emptyList());

    System.out.println("starting server");
    LegacyServerTransportFactory transportFactory =
        new LegacyServerTransportFactory(new ThriftServerConfig().setEnableJdkSsl(false));
    LegacyServerTransport transport = transportFactory.createServerTransport(serverHandler).block();
    InetSocketAddress address = (InetSocketAddress) transport.getAddress();

    LOG.info("creating client");

    RpcClientManager manager =
        new RpcClientManager(
            RpcClientFactory.builder()
                .setDisableLoadBalancing(true)
                .setThriftClientConfig(
                    new ThriftClientConfig()
                        .setDisableSSL(true)
                        .setRequestTimeout(Duration.succinctDuration(1, TimeUnit.DAYS)))
                .build(),
            address,
            ProtocolId.BINARY);

    PingService.Async pingService =
        manager.createClient(
            PingService.Async.class, Collections.emptyMap(), Collections.emptyMap());

    LOG.info("new client created, sending ping");

    Flux.range(0, n)
        .flatMap(
            i -> {
              PingRequest pingRequest = new PingRequest.Builder().setRequest("ping").build();
              ListenableFuture<PingResponse> ping = pingService.ping(pingRequest);
              return FutureUtil.toMono(ping);
            },
            32)
        .blockLast();
  }
}
