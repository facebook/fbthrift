package com.facebook.thrift.perf;

import com.facebook.thrift.*;
import com.facebook.thrift.protocol.*;
import com.facebook.thrift.transport.*;
import com.facebook.thrift.server.*;

public class NonblockingServerLoadTester extends LoadTester {

  public static void main(String[] args) throws Exception {
    new NonblockingServerLoadTester().run(args);
  }

  public TServer createServer() throws Exception {
    LoadTesterArgumentParser parser = getArgumentParser();

    LoadTest.Iface handler = new LoadTestHandler();
    TProcessor processor = new LoadTest.Processor(handler);
    TProcessorFactory procfactory = new TProcessorFactory(processor);
    TNonblockingServerTransport transport =
      new TNonblockingServerSocket(parser.getListenPort());
    TFramedTransport.Factory tfactory = new TFramedTransport.Factory();
    TProtocolFactory pfactory = new TBinaryProtocol.Factory();

    TNonblockingServer.Options options = new TNonblockingServer.Options();
    options.minWorkerThreads =
      options.maxWorkerThreads =
      parser.getNumberOfThreads();

    TServer server = new TNonblockingServer(procfactory,
                                            transport,
                                            tfactory,
                                            tfactory,
                                            pfactory,
                                            pfactory,
                                            options);

    return server;
  }

}
