package com.facebook.thrift.perf;

import com.facebook.thrift.*;
import com.facebook.thrift.perf.*;
import com.facebook.thrift.protocol.*;
import com.facebook.thrift.transport.*;
import com.facebook.thrift.server.*;
import com.facebook.thrift.server.example.TThreadPoolServer;

public class ThreadPoolServerLoadTester extends LoadTester {

  public static void main(String[] args) throws Exception {
    new ThreadPoolServerLoadTester().run(args);
  }

  public TServer createServer() throws Exception {
    LoadTesterArgumentParser parser = getArgumentParser();

    LoadTest.Iface handler = new LoadTestHandler();
    TProcessor processor = new LoadTest.Processor(handler);
    TProcessorFactory procfactory = new TProcessorFactory(processor);
    TServerTransport transport = new TServerSocket(parser.getListenPort());
    TFramedTransport.Factory tfactory = new TFramedTransport.Factory();
    TProtocolFactory pfactory = new TBinaryProtocol.Factory();

    TThreadPoolServer.Options options = new TThreadPoolServer.Options();
    options.minWorkerThreads =
      options.maxWorkerThreads =
      parser.getNumberOfThreads();

    TServer server = new TThreadPoolServer(procfactory,
                                           transport,
                                           tfactory,
                                           tfactory,
                                           pfactory,
                                           pfactory,
                                           options);

    return server;
  }

}
