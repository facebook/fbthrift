package com.facebook.thrift.perf;

import java.io.*;
import com.facebook.thrift.*;
import com.facebook.thrift.server.*;
import org.apache.commons.cli.*;

public abstract class LoadTester {

  public abstract TServer createServer() throws Exception;

  public void run(String[] args) throws Exception {
    LoadTesterArgumentParser parser = getArgumentParser();

    try {
      parser.parseOptions(args);
    }
    catch (Exception e) {
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
