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
import com.facebook.thrift.server.*;
import java.io.*;
import org.apache.commons.cli.*;

public abstract class LoadTester {

  public abstract TServer createServer() throws Exception;

  public void run(String[] args) throws Exception {
    LoadTesterArgumentParser parser = getArgumentParser();

    try {
      parser.parseOptions(args);
    } catch (Exception e) {
      System.out.println(e.getMessage());
      System.exit(1);
    }

    if (parser.getPrintUsage()) {
      parser.printUsage(this.getClass());
      System.exit(0);
    }

    TServer server = createServer();

    System.out.printf("Listening on port %d\n", parser.getListenPort());
    server.serve();
  }

  protected LoadTesterArgumentParser getArgumentParser() {
    if (parser == null) {
      parser = new LoadTesterArgumentParser();
    }
    return parser;
  }

  private LoadTesterArgumentParser parser;
}
