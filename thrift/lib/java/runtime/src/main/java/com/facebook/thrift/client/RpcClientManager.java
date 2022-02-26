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

import java.lang.reflect.Constructor;
import java.net.SocketAddress;
import java.util.Collections;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import org.apache.thrift.ProtocolId;
import reactor.core.Exceptions;
import reactor.core.publisher.Mono;

public final class RpcClientManager {
  private final RpcClientFactory rpcClientFactory;
  private final SocketAddress socketAddress;
  private final ProtocolId protocolId;

  @SuppressWarnings("rawtypes")
  private final Map<Class, Constructor> constructorMap;

  public RpcClientManager(
      final RpcClientFactory rpcClientFactory, final SocketAddress socketAddress) {
    this(rpcClientFactory, socketAddress, ProtocolId.BINARY);
  }

  public RpcClientManager(
      final RpcClientFactory rpcClientFactory,
      final SocketAddress socketAddress,
      final ProtocolId protocolId) {
    this.rpcClientFactory = rpcClientFactory;
    this.socketAddress = socketAddress;
    this.protocolId = protocolId;
    this.constructorMap = new ConcurrentHashMap<>();
  }

  @SuppressWarnings("unchecked")
  public <T> T createClient(
      final Class<T> clientInterface,
      final Map<String, String> headers,
      final Map<String, String> persistentHeaders) {

    try {
      Constructor<T> constructor =
          constructorMap.computeIfAbsent(clientInterface, this::getConstructor);
      return constructor.newInstance(
          protocolId, rpcClientFactory.createRpcClient(socketAddress), headers, persistentHeaders);
    } catch (Exception e) {
      throw Exceptions.propagate(e);
    }
  }

  private <T> Constructor<T> getConstructor(Class<T> clientInterface) {
    try {
      return clientInterface.getConstructor(ProtocolId.class, Mono.class, Map.class, Map.class);
    } catch (Exception e) {
      throw Exceptions.propagate(e);
    }
  }

  public <T> T createClient(final Class<T> clientInterface) {
    return createClient(clientInterface, Collections.emptyMap(), Collections.emptyMap());
  }
}
