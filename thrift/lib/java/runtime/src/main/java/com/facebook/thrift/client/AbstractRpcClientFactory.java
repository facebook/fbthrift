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
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import org.apache.thrift.ProtocolId;
import reactor.core.publisher.Mono;

public abstract class AbstractRpcClientFactory {

  protected final SocketAddress socketAddress;
  protected final RpcClientFactory rpcClientFactory;
  protected final ProtocolId protocolId;

  public AbstractRpcClientFactory(
      SocketAddress socketAddress, RpcClientFactory rpcClientFactory, ProtocolId protocolId) {
    this.socketAddress = socketAddress;
    this.rpcClientFactory = rpcClientFactory;
    this.protocolId = protocolId;
  }

  public <T> T createClient(
      Class<T> serviceClass, Map<String, String> headers, Map<String, String> persistentHeaders) {
    Mono<? extends RpcClient> rpcClientMono = rpcClientFactory.createRpcClient(socketAddress);
    try {
      TypeConstructor constructor =
          serviceCache.computeIfAbsent(serviceClass, this::getConstructor);
      return serviceClass.cast(
          doCreateClient(constructor, headers, persistentHeaders, rpcClientMono));
    } catch (Exception e) {
      throw new IllegalStateException("Exception while creating client", e);
    }
  }

  abstract <T> TypeConstructor getConstructor(Class<T> serviceClass);

  abstract Object doCreateClient(
      TypeConstructor constructor,
      Map<String, String> headers,
      Map<String, String> persistentHeaders,
      Mono<? extends RpcClient> rpcClientMono)
      throws Exception;

  protected final ConcurrentHashMap<Class<?>, TypeConstructor> serviceCache =
      new ConcurrentHashMap<>();

  protected static class TypeConstructor {
    Constructor<?> constructor;

    public TypeConstructor(Constructor<?> constructor) {
      this.constructor = constructor;
    }

    public Object construct(
        ProtocolId protocolId,
        Mono<? extends RpcClient> rpcClientMono,
        Map<String, String> headers,
        Map<String, String> persistentHeaders)
        throws Exception {
      return constructor.newInstance(protocolId, rpcClientMono, headers, persistentHeaders);
    }
  }
}
