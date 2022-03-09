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
import org.apache.thrift.ProtocolId;
import reactor.core.publisher.Mono;

public class ReactiveClientFactory extends AbstractRpcClientFactory {

  private static final String REACTIVE_CLIENT_SUFFIX = "ReactiveClient";

  public ReactiveClientFactory(
      SocketAddress socketAddress, RpcClientFactory rpcClientFactory, ProtocolId protocolId) {
    super(socketAddress, rpcClientFactory, protocolId);
  }

  @Override
  <T> TypeConstructor getConstructor(Class<T> serviceClass) {
    try {
      Class<?> clientClass =
          Class.forName(serviceClass.getEnclosingClass().getName() + REACTIVE_CLIENT_SUFFIX);
      Constructor<?> constructor =
          clientClass.getConstructor(ProtocolId.class, Mono.class, Map.class, Map.class);

      return new TypeConstructor(constructor);
    } catch (ClassNotFoundException | NoSuchMethodException e) {
      throw new RuntimeException(e);
    }
  }

  @Override
  Object doCreateClient(
      TypeConstructor constructor,
      Map<String, String> headers,
      Map<String, String> persistentHeaders,
      Mono<? extends RpcClient> rpcClientMono)
      throws Exception {
    return constructor.construct(protocolId, rpcClientMono, headers, persistentHeaders);
  }
}
