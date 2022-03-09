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

import java.net.SocketAddress;
import java.util.Collections;
import java.util.Map;
import org.apache.thrift.ProtocolId;

public final class RpcClientManager {
  private static final String ASYNC_TYPE_NAME = "Async";
  private static final String REACTIVE_TYPE_NAME = "Reactive";

  private final AbstractRpcClientFactory syncClientFactory;
  private final AbstractRpcClientFactory asyncClientFactory;
  private final AbstractRpcClientFactory reactiveClientFactory;

  public RpcClientManager(
      final RpcClientFactory rpcClientFactory, final SocketAddress socketAddress) {
    this(rpcClientFactory, socketAddress, ProtocolId.BINARY);
  }

  public RpcClientManager(
      final RpcClientFactory rpcClientFactory,
      final SocketAddress socketAddress,
      final ProtocolId protocolId) {
    this.syncClientFactory = new SyncClientFactory(socketAddress, rpcClientFactory, protocolId);
    this.asyncClientFactory = new AsyncClientFactory(socketAddress, rpcClientFactory, protocolId);
    this.reactiveClientFactory =
        new ReactiveClientFactory(socketAddress, rpcClientFactory, protocolId);
  }

  public <T> T createClient(
      final Class<T> clientInterface,
      final Map<String, String> headers,
      final Map<String, String> persistentHeaders) {
    String classType = clientInterface.getSimpleName();
    if (ASYNC_TYPE_NAME.equals(classType)) {
      return asyncClientFactory.createClient(clientInterface, headers, persistentHeaders);
    } else if (REACTIVE_TYPE_NAME.equals(classType)) {
      return reactiveClientFactory.createClient(clientInterface, headers, persistentHeaders);
    } else {
      return syncClientFactory.createClient(clientInterface, headers, persistentHeaders);
    }
  }

  public <T> T createClient(final Class<T> clientInterface) {
    return createClient(clientInterface, Collections.emptyMap(), Collections.emptyMap());
  }
}
