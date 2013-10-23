package com.facebook.thrift.perf;

import com.facebook.thrift.*;
import com.facebook.thrift.perf.*;
import com.facebook.thrift.protocol.*;
import com.facebook.thrift.transport.*;
import com.facebook.thrift.server.*;
import com.facebook.thrift.server.example.TSimpleServer;

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
