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

package com.facebook.thrift.rsocket.client;

import com.facebook.thrift.client.RpcClient;
import com.facebook.thrift.client.RpcClientFactory;
import com.facebook.thrift.client.ThriftClientConfig;
import com.facebook.thrift.model.StreamResponse;
import com.facebook.thrift.payload.ClientResponsePayload;
import com.facebook.thrift.protocol.ByteBufTProtocol;
import com.facebook.thrift.protocol.TProtocolType;
import com.facebook.thrift.util.ReactorHooks;
import com.facebook.thrift.util.RpcClientUtils;
import io.rsocket.Payload;
import io.rsocket.RSocket;
import io.rsocket.SocketAcceptor;
import java.net.SocketAddress;
import java.util.Map;
import org.apache.thrift.ResponseRpcMetadata;
import org.apache.thrift.ServerPushMetadata;
import org.apache.thrift.StreamPayloadMetadata;
import reactor.core.publisher.Mono;

public final class HeaderAwareRpcClientFactory implements RpcClientFactory {
  static {
    ReactorHooks.init();
  }

  private static final StreamResponse<Void, String> HEADER_RESPONSE =
      StreamResponse.fromData("header_response");

  private final HeaderEventBus headerEventBus;
  private final RSocketRpcClientFactory delegate;

  public HeaderAwareRpcClientFactory(ThriftClientConfig config) {
    this.headerEventBus = new DefaultHeaderEventBus();
    this.delegate =
        new RSocketRpcClientFactory(
            config,
            RpcClientUtils.createRSocketConnector()
                .acceptor(
                    SocketAcceptor.with(
                        new RSocket() {
                          @Override
                          public Mono<Void> metadataPush(Payload payload) {
                            try {
                              final ByteBufTProtocol protocol =
                                  TProtocolType.TCompact.apply(payload.sliceMetadata());
                              final ServerPushMetadata serverPushMetadata =
                                  ServerPushMetadata.read0(protocol);

                              if (serverPushMetadata.isSetStreamHeadersPush()
                                  && serverPushMetadata
                                          .getStreamHeadersPush()
                                          .getHeadersPayloadContent()
                                      != null) {
                                Map<String, byte[]> headers =
                                    serverPushMetadata
                                        .getStreamHeadersPush()
                                        .getHeadersPayloadContent()
                                        .getOtherMetadata();

                                StreamPayloadMetadata streamPayloadMetadata =
                                    new StreamPayloadMetadata.Builder()
                                        .setOtherMetadata(headers)
                                        .build();

                                ClientResponsePayload<Object> responsePayload =
                                    ClientResponsePayload.createStreamResult(
                                        HEADER_RESPONSE,
                                        ResponseRpcMetadata.defaultInstance(),
                                        streamPayloadMetadata,
                                        headers,
                                        serverPushMetadata.getStreamHeadersPush().getStreamId());

                                headerEventBus.sendEmit(
                                    serverPushMetadata.getStreamHeadersPush().getStreamId(),
                                    responsePayload);
                              }
                              return Mono.empty();
                            } finally {
                              payload.release();
                            }
                          }
                        })));
  }

  @Override
  public Mono<RpcClient> createRpcClient(SocketAddress socketAddress) {

    try {
      return delegate
          .createRpcClient(socketAddress)
          .map(rpcClient -> new HeaderAwareRSocketRpcClient(rpcClient, headerEventBus));
    } catch (Throwable t) {
      return Mono.error(t);
    }
  }

  public void close() {
    delegate.close();
  }
}
