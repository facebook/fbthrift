package com.facebook.thrift.perf;

import org.apache.commons.cli.*;
import com.facebook.thrift.*;
import com.facebook.thrift.server.*;
import com.facebook.thrift.direct_server.*;

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
        TDirectServer.asHsHaServer(parser.getListenPort(),
                                   parser.getNumberOfThreads(),
                                   parser.getNumberOfPendingOperations(),
                                   processor);
    } else {
      server =
        TDirectServer.asFiberServer(parser.getListenPort(),
                                    parser.getNumberOfThreads(),
                                    processor);
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
