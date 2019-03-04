package com.facebook.thrift.direct_server; // NOPMD

/**
 * Copyright 2011 Facebook
 *
 * <p>A simple HsHaServer as code sample/snippet as well as part of load testing tool.
 *
 * @author Wei Chen (weichen@fb.com)
 */

import com.facebook.thrift.protocol.*;
import com.facebook.thrift.server.*;
import com.facebook.thrift.transport.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TestHsHaServer {
  private static int port_ = 3010;

  private static int timeoutMs_ = 10000;

  private static int tcpBacklog_ = 10;

  private static Logger LOG = LoggerFactory.getLogger(TestHsHaServer.class);

  public static void main(String[] args) {
    SimpleServiceHandler handler = new SimpleServiceHandler();
    JavaSimpleService.Processor processor = new JavaSimpleService.Processor(handler);

    try {
      TNonblockingServerSocket tServerSocket =
          new TNonblockingServerSocket(port_, timeoutMs_, tcpBacklog_);
      // Protocol factory
      TProtocolFactory tProtocolFactory = new TBinaryProtocol.Factory();
      // Server options
      THsHaServer.Options options = new THsHaServer.Options();
      options.stopTimeoutVal = Integer.MAX_VALUE;
      options.minWorkerThreads = 8;
      options.maxWorkerThreads = 8;
      options.minHsHaWorkerThreads = 8;
      options.maxHsHaWorkerThreads = 8;
      options.queueSize = 65535;
      options.timeout = 10000;

      THsHaServer serverEngine =
          new THsHaServer(processor, tServerSocket, tProtocolFactory, options);

      serverEngine.serve();
    } catch (Exception e) {
      LOG.warn("Having exception " + e);
    }
  }
};
