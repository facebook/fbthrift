/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

package com.facebook.movies;

import com.facebook.nifty.codec.ThriftFrameCodecFactory;
import com.facebook.nifty.core.NiftyNoOpSecurityFactory;
import com.facebook.nifty.core.NiftyTimer;
import com.facebook.nifty.duplex.TDuplexProtocolFactory;
import com.facebook.nifty.header.codec.HeaderThriftCodecFactory;
import com.facebook.nifty.header.protocol.TDuplexHeaderProtocolFactory;
import com.facebook.swift.codec.ThriftCodecManager;
import com.facebook.swift.service.ThriftEventHandler;
import com.facebook.swift.service.ThriftServer;
import com.facebook.swift.service.ThriftServerConfig;
import com.facebook.swift.service.ThriftServiceProcessor;
import com.google.common.collect.ImmutableMap;
import java.util.ArrayList;
import java.util.List;
import java.util.concurrent.ExecutorService;

public class MoviesServer {
  public static void main(String[] args) {
    ThriftCodecManager codecManager = new ThriftCodecManager();
    List<ThriftEventHandler> eventHandlers = new ArrayList<>();

    MoviesServiceHandler handler = new MoviesServiceHandler();
    ThriftServiceProcessor serviceProcessor =
        new ThriftServiceProcessor(codecManager, eventHandlers, handler);

    ThriftServerConfig serverConfig =
        new ThriftServerConfig()
            .setTransportName("header")
            .setProtocolName("header")
            .setWorkerThreads(5)
            .setPort(7777);

    ThriftServer server =
        new ThriftServer(
            serviceProcessor,
            serverConfig,
            new NiftyTimer("thrift"),
            ImmutableMap.<String, ThriftFrameCodecFactory>of(
                "header", new HeaderThriftCodecFactory()),
            ImmutableMap.<String, TDuplexProtocolFactory>of(
                "header", new TDuplexHeaderProtocolFactory()),
            ImmutableMap.<String, ExecutorService>of(),
            new NiftyNoOpSecurityFactory());

    server.start();
  }
}
