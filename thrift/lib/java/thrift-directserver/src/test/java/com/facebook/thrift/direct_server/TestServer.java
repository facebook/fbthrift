package com.facebook.thrift.direct_server; // NOPMD

/**
 * Copyright 2011 Facebook
 * @author Wei Chen (weichen@fb.com)
 *
 * A simple TDirectServer as code sample/snippet as well as
 * part of load testing tool.
 */

import com.facebook.thrift.direct_server.*;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import com.facebook.fbcode.fb303.*;


public class TestServer {
  static private Logger LOG =
      LoggerFactory.getLogger(TestServer.class);

  static boolean useFiberServer_ = false;


  public static void main(String[] args) {
    SimpleServiceHandler handler = new SimpleServiceHandler();
    JavaSimpleService.Processor processor =
      new JavaSimpleService.Processor(handler);

    // add automatic fb303 counters
    processor.setEventHandler(
        new DirectTProcessorEventHandler((FacebookBase)handler));

    TDirectServer server;
    if (useFiberServer_) {
      server = TDirectServer.asFiberServer(3010, 8, processor);
    } else {
      server = TDirectServer.asHsHaServer(3010, 8, 32, processor);
    }

    server.serve();
  }
};
