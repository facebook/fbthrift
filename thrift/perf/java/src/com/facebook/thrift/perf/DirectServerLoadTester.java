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

package com.facebook.thrift.perf;

import com.facebook.thrift.*;
import com.facebook.thrift.direct_server.*;
import com.facebook.thrift.server.*;
import org.apache.commons.cli.*;

public class DirectServerLoadTester extends LoadTester {

  public static void main(String[] args) throws Exception {
    new DirectServerLoadTester().run(args);
  }

  public TServer createServer() throws Exception {
    DirectServerLoadTesterArgumentParser parser = getArgumentParser();
    LoadTest.Iface handler = new LoadTestHandler();
    TProcessor processor = new LoadTest.Processor(handler);

    TServer server;
    if (parser.getHsHaMode()) {
      server =
          TDirectServer.asHsHaServer(
              parser.getListenPort(),
              parser.getNumberOfThreads(),
              parser.getNumberOfPendingOperations(),
              processor);
    } else {
      server =
          TDirectServer.asFiberServer(
              parser.getListenPort(), parser.getNumberOfThreads(), processor);
    }
    return server;
  }

  protected DirectServerLoadTesterArgumentParser getArgumentParser() {
    if (parser == null) {
      parser = new DirectServerLoadTesterArgumentParser();
    }
    return parser;
  }

  private DirectServerLoadTesterArgumentParser parser;
}
