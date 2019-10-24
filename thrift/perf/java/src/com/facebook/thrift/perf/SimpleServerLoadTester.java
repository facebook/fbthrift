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
import com.facebook.thrift.protocol.*;
import com.facebook.thrift.server.*;
import com.facebook.thrift.server.example.TSimpleServer;
import com.facebook.thrift.transport.*;

public class SimpleServerLoadTester extends LoadTester {

  public static void main(String[] args) throws Exception {
    new SimpleServerLoadTester().run(args);
  }

  public TServer createServer() throws Exception {
    LoadTesterArgumentParser parser = getArgumentParser();

    LoadTest.Iface handler = new LoadTestHandler();
    TProcessor processor = new LoadTest.Processor(handler);
    TServerTransport transport = new TServerSocket(parser.getListenPort());
    TTransportFactory tfactory = new TFramedTransport.Factory();
    TProtocolFactory pfactory = new TBinaryProtocol.Factory();

    TServer server = new TSimpleServer(processor, transport, tfactory, pfactory);

    return server;
  }
}
