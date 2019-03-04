package com.facebook.thrift.direct_server; // NOPMD

/**
 * Copyright 2011 Facebook
 *
 * <p>A simple TDirectServer as code sample/snippet as well as part of load testing tool.
 *
 * @author Wei Chen (weichen@fb.com)
 */

import com.facebook.fbcode.fb303.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

public class TestServer {
  private static Logger LOG = LoggerFactory.getLogger(TestServer.class);

  static boolean useFiberServer_ = false;

  public static void main(String[] args) {
    SimpleServiceHandler handler = new SimpleServiceHandler();
    JavaSimpleService.Processor processor = new JavaSimpleService.Processor(handler);

    // add automatic fb303 counters
    processor.setEventHandler(new DirectTProcessorEventHandler((FacebookBase) handler));

    TDirectServer server;
    if (useFiberServer_) {
      server = TDirectServer.asFiberServer(3010, 8, processor);
    } else {
      server = TDirectServer.asHsHaServer(3010, 8, 32, processor);
    }

    server.serve();
  }
};
