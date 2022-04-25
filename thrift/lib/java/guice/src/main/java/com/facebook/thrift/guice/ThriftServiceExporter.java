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

package com.facebook.thrift.guice;

import static com.google.inject.multibindings.Multibinder.newSetBinder;

import com.facebook.swift.service.ThriftEventHandler;
import com.facebook.thrift.server.CompositeRpcServerHandler;
import com.facebook.thrift.server.RpcServerHandler;
import com.facebook.thrift.util.RpcServerUtils;
import com.facebook.thrift.util.ThriftService;
import com.google.inject.Binder;
import com.google.inject.Inject;
import com.google.inject.Provider;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import java.util.Set;
import java.util.stream.Collectors;

public class ThriftServiceExporter {
  private final Binder binder;

  public ThriftServiceExporter(Binder binder) {
    this.binder = binder;
  }

  public static ThriftServiceExporter binder(Binder binder) {
    return new ThriftServiceExporter(binder);
  }

  public ThriftServiceExporter addEventHandler(ThriftEventHandler eventHandler) {
    Objects.requireNonNull(eventHandler);
    newSetBinder(binder, ThriftEventHandler.class).addBinding().toInstance(eventHandler);
    return this;
  }

  public ThriftServiceExporter addEventHandler(Class<? extends ThriftEventHandler> clazz) {
    newSetBinder(binder, ThriftEventHandler.class).addBinding().to(clazz);
    return this;
  }

  public ThriftServiceExporter addThriftService(ThriftService... thriftService) {
    Objects.requireNonNull(thriftService);
    for (ThriftService ts : thriftService) {
      newSetBinder(binder, ThriftService.class).addBinding().toInstance(ts);
    }
    return this;
  }

  public static class RpcServerHandlerProvider implements Provider<RpcServerHandler> {
    private final Set<ThriftService> thriftServices;
    private final Set<ThriftEventHandler> eventHandlers;

    @Inject
    public RpcServerHandlerProvider(
        Set<ThriftService> thriftServices, Set<ThriftEventHandler> eventHandlers) {
      this.thriftServices = thriftServices;
      this.eventHandlers = eventHandlers;
    }

    @Override
    public RpcServerHandler get() {
      List<ThriftEventHandler> eventHandlers = new ArrayList<>(this.eventHandlers);
      List<RpcServerHandler> rpcServerHandlers =
          thriftServices.stream()
              .map(
                  thriftService ->
                      RpcServerUtils.getRpcServerHandlerForThriftService(
                          thriftService, eventHandlers))
              .collect(Collectors.toList());

      if (rpcServerHandlers.size() == 1) {
        return rpcServerHandlers.get(0);
      } else {
        return new CompositeRpcServerHandler(rpcServerHandlers);
      }
    }
  }
}
