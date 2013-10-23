/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

package com.facebook.thrift.test;

import java.io.IOException;
import java.net.ServerSocket;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import com.facebook.thrift.TApplicationException;
import com.facebook.thrift.TBase;
import com.facebook.thrift.TException;
import com.facebook.thrift.TProcessor;
import com.facebook.thrift.TProcessorEventHandler;
import com.facebook.thrift.TProcessorFactory;
import com.facebook.thrift.direct_server.TDirectServer;
import com.facebook.thrift.protocol.TBinaryProtocol;
import com.facebook.thrift.protocol.THeaderProtocol;
import com.facebook.thrift.protocol.TProtocol;
import com.facebook.thrift.protocol.TProtocolFactory;
import com.facebook.thrift.server.TConnectionContext;
import com.facebook.thrift.server.THsHaServer;
import com.facebook.thrift.server.TNonblockingServer;
import com.facebook.thrift.server.TServer;
import com.facebook.thrift.server.example.TSimpleServer;
import com.facebook.thrift.transport.TFramedTransport;
import com.facebook.thrift.transport.THeaderTransport;
import com.facebook.thrift.transport.TMemoryBuffer;
import com.facebook.thrift.transport.TNonblockingServerSocket;
import com.facebook.thrift.transport.TTransport;
import com.facebook.thrift.transport.TTransportException;
import com.facebook.thrift.transport.TTransportFactory;
import com.facebook.thrift.transport.TServerSocket;
import com.facebook.thrift.transport.TSocket;

import thrift.test.Insanity;
import thrift.test.Numberz;
import thrift.test.ThriftTest;
import thrift.test.Xception;
import thrift.test.Xception2;
import thrift.test.Xtruct;
import thrift.test.Xtruct2;

import org.junit.*;

public class THeaderJava extends junit.framework.TestCase {
  public int TEST_PORT = -1;
  public static final int DIRECT_SERVER_THREADS = 4;
  public static final int DIRECT_SERVER_PENDING_OPS = 12;

  public static final String identity = "me";

  public enum ServerType {
    SIMPLE, NONBLOCKING, HSHA, DIRECT_HSHA, DIRECT_FIBER };

  public class ServerThread implements Runnable {
    private TServer serverEngine;
    private THeaderProtocol.Factory tProtocolFactory;
    private THeaderTransport.Factory tTransportFactory;
    private TServerSocket tServerSocket;
    private TNonblockingServerSocket tNonblockingServerSocket;
    private ThriftTest.Processor testProcessor;
    TProcessorFactory pFactory;

    public ServerThread(ServerType serverType) throws IOException, TException {
      // processor
      TestHandler testHandler = new TestHandler();
      testProcessor = new ThriftTest.Processor(testHandler);
      pFactory = new TestProcessorFactory(testProcessor);

      // Reset transports
      tServerSocket = null;
      tNonblockingServerSocket = null;

      // Protocol factory
      List<THeaderTransport.ClientTypes> clientTypes =
        new ArrayList<THeaderTransport.ClientTypes>();
      tTransportFactory = new THeaderTransport.Factory(clientTypes);
      tProtocolFactory = new THeaderProtocol.Factory();

      if (serverType == ServerType.SIMPLE) {
        makeSimple();
      } else if (serverType == ServerType.NONBLOCKING) {
        makeNonblocking();
      } else if (serverType == ServerType.HSHA) {
        makeHsHa();
      } else if (serverType == ServerType.DIRECT_HSHA) {
        makeDirectHsHa();
      } else if (serverType == ServerType.DIRECT_FIBER) {
        makeDirectFiber();
      }
    }

    private TServerSocket makeServerSocket() throws IOException, TTransportException {
      ServerSocket serverSocket = new ServerSocket(0);
      TEST_PORT = serverSocket.getLocalPort();
      tServerSocket = new TServerSocket(serverSocket);
      return tServerSocket;
    }

    private TNonblockingServerSocket makeNonblockingServerSocket()
        throws TTransportException {
      tNonblockingServerSocket = new TNonblockingServerSocket(0);
      TEST_PORT = tNonblockingServerSocket.getLocalPort();
      return tNonblockingServerSocket;
    }

    private void makeSimple() throws IOException, TTransportException {
      // Simple Server
      serverEngine = new TSimpleServer(pFactory,
                                       makeServerSocket(),
                                       new TTransportFactory(),
                                       tProtocolFactory);
    }

    private void makeNonblocking() throws TTransportException {
      // ThreadPool Server
      serverEngine = new TNonblockingServer(
        pFactory,
        makeNonblockingServerSocket(),
        tTransportFactory,
        tProtocolFactory);
    }

    private void makeHsHa() throws TTransportException {
      // HsHa Server
      serverEngine = new THsHaServer(pFactory,
                                     makeNonblockingServerSocket(),
                                     tTransportFactory,
                                     tProtocolFactory);
    }

    private void makeDirectHsHa() throws TTransportException {
      // Direct Server in HsHa mode
      serverEngine = TDirectServer.asHsHaServer(0,
                                                DIRECT_SERVER_THREADS,
                                                DIRECT_SERVER_PENDING_OPS,
                                                testProcessor);
      TEST_PORT = ((TDirectServer) serverEngine).directServer().getLocalPort();
    }

    private void makeDirectFiber() throws TTransportException {
      // Direct Server in Fiber mode
      serverEngine = TDirectServer.asFiberServer(0,
                                                 DIRECT_SERVER_THREADS,
                                                 testProcessor);
      TEST_PORT = ((TDirectServer) serverEngine).directServer().getLocalPort();
    }

    public void run() {
      // Run it
      serverEngine.serve();
    }

    public void stop() {
      serverEngine.stop();
      if (tServerSocket != null) {
        tServerSocket.close();
      }
      if (tNonblockingServerSocket != null) {
        tNonblockingServerSocket.close();
      }
    }

  }

  public class TestProcessorFactory extends TProcessorFactory {
    public TestProcessorFactory(TProcessor processor) {
      super(processor);
      processor.setEventHandler(new TestEventHandler());
    }
  }

  int preRead = 0;
  int postRead = 0;
  int preWrite = 0;
  int postWrite = 0;
  int getContext = 0;
  int handlerError = 0;

  public class TestEventHandler extends TProcessorEventHandler {

    @Override
    public Object getContext(String fn_name, TConnectionContext context) {
      TProtocol oprot = context.getOutputProtocol();
      if (oprot instanceof THeaderProtocol) {
        ((THeaderTransport)oprot.getTransport()).setIdentity(identity);
      }
      getContext++;
      return null;
    }

    @Override
    public void preRead(Object handler_context, String fn_name) {
      preRead++;
    }

    @Override
    public void postRead(Object handler_context, String fn_name, TBase args) {
      postRead++;
    }

    @Override
    public void preWrite(Object handler_context, String fn_name, TBase res) {
      preWrite++;
    }

    @Override
    public void postWrite(Object handler_context, String fn_name, TBase res) {
      postWrite++;
    }

    @Override
    public void handlerError(Object handler_context, String fn_name,
            Throwable th) {
      handlerError++;
    }
  }

  public static class TestHandler implements ThriftTest.Iface {

    public TestHandler() {}

    public int testConnectionDestroyed() {
      return 0;
    }

    public int testNewConnection() {
      return 0;
    }

    public int testPreServe() {
      return 0;
    }

    public int testRequestCount() {
      return 0;
    }

    public void testVoid() {
    }

    public String testString(String thing) {
      return thing;
    }

    public byte testByte(byte thing) {
      return thing;
    }

    public int testI32(int thing) {
      return thing;
    }

    public long testI64(long thing) {
      return thing;
    }

    public double testDouble(double thing) {
      return thing;
    }

    public float testFloat(float thing) {
      return thing;
    }

    public Xtruct testStruct(Xtruct thing) {
      return thing;
    }

    public Xtruct2 testNest(Xtruct2 nest) {
      Xtruct thing = nest.struct_thing;
      return nest;
    }

    public Map<Integer,Integer> testMap(Map<Integer,Integer> thing) {
      return thing;
    }

    public Set<Integer> testSet(Set<Integer> thing) {
      return thing;
    }

    public List<Integer> testList(List<Integer> thing) {
      return thing;
    }

    public int testEnum(int thing) {
      return thing;
    }

    public long testTypedef(long thing) {
      return thing;
    }

    public Map<Integer,Map<Integer,Integer>> testMapMap(int hello) {
      Map<Integer,Map<Integer,Integer>> mapmap =
        new HashMap<Integer,Map<Integer,Integer>>();

      HashMap<Integer,Integer> pos = new HashMap<Integer,Integer>();
      HashMap<Integer,Integer> neg = new HashMap<Integer,Integer>();
      for (int i = 1; i < 5; i++) {
        pos.put(i, i);
        neg.put(-i, -i);
      }

      mapmap.put(4, pos);
      mapmap.put(-4, neg);

      return mapmap;
    }

    public Map<Long, Map<Integer,Insanity>> testInsanity(Insanity argument) {

      Xtruct hello = new Xtruct();
      hello.string_thing = "Hello2";
      hello.byte_thing = 2;
      hello.i32_thing = 2;
      hello.i64_thing = 2;

      Xtruct goodbye = new Xtruct();
      goodbye.string_thing = "Goodbye4";
      goodbye.byte_thing = (byte)4;
      goodbye.i32_thing = 4;
      goodbye.i64_thing = (long)4;

      Insanity crazy = new Insanity();
      crazy.userMap = new HashMap<Integer, Long>();
      crazy.xtructs = new ArrayList<Xtruct>();

      crazy.userMap.put(Numberz.EIGHT, (long)8);
      crazy.xtructs.add(goodbye);

      Insanity looney = new Insanity();
      crazy.userMap.put(Numberz.FIVE, (long)5);
      crazy.xtructs.add(hello);

      HashMap<Integer,Insanity> first_map = new HashMap<Integer, Insanity>();
      HashMap<Integer,Insanity> second_map = new HashMap<Integer, Insanity>();;

      first_map.put(Numberz.TWO, crazy);
      first_map.put(Numberz.THREE, crazy);

      second_map.put(Numberz.SIX, looney);

      Map<Long,Map<Integer,Insanity>> insane =
        new HashMap<Long, Map<Integer,Insanity>>();
      insane.put((long)1, first_map);
      insane.put((long)2, second_map);

      return insane;
    }

    public Xtruct testMulti(byte arg0, int arg1, long arg2, Map<Short,String> arg3, int arg4, long arg5) {

      Xtruct hello = new Xtruct();;
      hello.string_thing = "Hello2";
      hello.byte_thing = arg0;
      hello.i32_thing = arg1;
      hello.i64_thing = arg2;
      return hello;
    }

    public void testException(String arg) throws Xception {
      if (arg.equals("Xception")) {
        Xception x = new Xception();
        x.errorCode = 1001;
        x.message = "Xception";
        throw x;
      }
      return;
    }

    public Xtruct testMultiException(String arg0, String arg1) throws Xception, Xception2 {
      if (arg0.equals("Xception")) {
        Xception x = new Xception();
        x.errorCode = 1001;
        x.message = "This is an Xception";
        throw x;
      } else if (arg0.equals("Xception2")) {
        Xception2 x = new Xception2();
        x.errorCode = 2002;
        x.struct_thing = new Xtruct();
        x.struct_thing.string_thing = "This is an Xception2";
        throw x;
      }

      Xtruct result = new Xtruct();
      result.string_thing = arg1;
      return result;
    }

    public void testOneway(int sleepFor) {
      try {
        Thread.sleep(sleepFor * 1000);
      } catch (InterruptedException ie) {
        throw new RuntimeException(ie);
      }
    }

  } // class TestHandler

  // Note these are all separate methods to make it easy to view in the
  // unittest runner
  @Test
  public void testSimpleServerUnframed() throws Exception {
    doServerClient(ServerType.SIMPLE, true, false, false);
  }

  @Test
  public void testNonblockingServerFramed() throws Exception {
    doServerClient(ServerType.SIMPLE, false, true, false);
  }

  @Test
  public void testNonblockingServerHeader() throws Exception {
    doServerClient(ServerType.SIMPLE, false, false, true);
  }

  @Test
  public void testHsHaServerFramed() throws Exception {
    doServerClient(ServerType.HSHA, false, true, false);
  }

  @Test
  public void testHsHaServerHeader() throws Exception {
    doServerClient(ServerType.HSHA, false, false, true);
  }

  // TODO: get the header tests working for TDirectServer

  // @Test
  // public void testDirectHsHaServerFramed() throws Exception {
  //   doServerClient(ServerType.DIRECT_HSHA, false, true, false);
  // }

  // @Test
  // public void testDirectHsHaServerHeader() throws Exception {
  //   doServerClient(ServerType.DIRECT_HSHA, false, false, true);
  // }

  // @Test
  // public void testDirectFiberServerFramed() throws Exception {
  //   doServerClient(ServerType.DIRECT_FIBER, false, true, false);
  // }

  // @Test
  // public void testDirectFiberServerHeader() throws Exception {
  //   doServerClient(ServerType.DIRECT_FIBER, false, false, true);
  // }

  @Test
  public void testkeyValueHeader() throws TException {
    TMemoryBuffer buf = new TMemoryBuffer(200);
    THeaderTransport trans = new THeaderTransport(buf);
    TBinaryProtocol prot = new TBinaryProtocol(trans);
    Xtruct out = new Xtruct();

    trans.setHeader("test1", "value1");
    trans.setHeader("test2", "value2");
    out.write(prot);
    trans.flush();

    Xtruct in = new Xtruct();
    in.read(prot);
    HashMap<String, String> headers = trans.getHeaders();
    assertEquals(2, headers.size());
    assertTrue(headers.containsKey("test1"));
    assertTrue(headers.containsKey("test2"));
    assertEquals("value1", headers.get("test1"));
    assertEquals("value2", headers.get("test2"));
  }


  public void testTransform(THeaderTransport.Transforms transform)
      throws TException {
    TMemoryBuffer buf = new TMemoryBuffer(200);
    THeaderTransport writer = new THeaderTransport(buf);
    writer.addTransform(transform);
    String frost = "Whose woods these are I think I know";
    byte[] testBytes = frost.getBytes();
    writer.write(testBytes, 0, testBytes.length);
    writer.flush();

    THeaderTransport reader = new THeaderTransport(buf);
    byte[] receivedBytes = new byte[testBytes.length];
    reader.read(receivedBytes, 0, receivedBytes.length);
    Assert.assertArrayEquals(testBytes, receivedBytes);
  }

  @Test
  public void testZlibTransform() throws TException {
    testTransform(
      THeaderTransport.Transforms.ZLIB_TRANSFORM);
  }

  @Test
  public void testSnappyTransform() throws TException {
    testTransform(
      THeaderTransport.Transforms.SNAPPY_TRANSFORM);
  }

  public void doServerClient(ServerType serverType, boolean unframed,
                             boolean framed, boolean header)
    throws TException, IOException, InterruptedException {
    ServerThread st = new ServerThread(serverType);
    Thread r = new Thread(st);
    r.start();
    doTransports(unframed, framed, header);
    st.stop();
    r.interrupt();
    r.join();
  }

  public void doTransports(boolean unframed,
                            boolean framed, boolean header) throws TException{
    TTransport transport;
    TSocket socket = new TSocket("localhost", TEST_PORT);
    socket.setTimeout(1000);
    transport = socket;
    TProtocol prot = new TBinaryProtocol(transport);
    if (unframed) {
      testClient(transport, prot); // Unframed
    }
    List<THeaderTransport.ClientTypes> clientTypes =
      new ArrayList<THeaderTransport.ClientTypes>();
    prot = new THeaderProtocol(transport, clientTypes);
    if (header) {
      testClient(transport, prot); // Header w/compact
    }
    TFramedTransport framedTransport = new TFramedTransport(transport);
    prot = new TBinaryProtocol(framedTransport);
    if (framed) {
      testClient(transport, prot); // Framed
    }
  }

  public void testClient(TTransport transport, TProtocol prot)
      throws TException {
    ThriftTest.Client testClient =
      new ThriftTest.Client(prot);
    Insanity insane = new Insanity();

    /**
     * CONNECT TEST
     */
    transport.open();

    long start = System.nanoTime();

    /**
     * VOID TEST
     */
    testClient.testVoid();

    /**
     * STRING TEST
     */
    String s = testClient.testString("Test");

    /**
     * IDENTITY TEST
     */
    if (prot instanceof THeaderProtocol) {
      assertEquals(identity,
                   ((THeaderTransport)prot.getTransport()).getPeerIdentity());
    }

    /**
     * BYTE TEST
     */
    byte i8 = testClient.testByte((byte)1);

    /**
     * I32 TEST
     */
    int i32 = testClient.testI32(-1);

    /**
     * I64 TEST
     */
    long i64 = testClient.testI64(-34359738368L);

    /**
     * DOUBLE TEST
     */
    double t = 5.325098235;
    double dub = testClient.testDouble(t);
    assertEquals(t, dub);

    /**
     * STRUCT TEST
     */
    Xtruct out = new Xtruct();
    out.string_thing = "Zero";
    out.byte_thing = (byte) 1;
    out.i32_thing = -3;
    out.i64_thing = -5;
    Xtruct in = testClient.testStruct(out);
    assertEquals(out, in);

    /**
     * NESTED STRUCT TEST
     */
    Xtruct2 out2 = new Xtruct2();
    out2.byte_thing = (short)1;
    out2.struct_thing = out;
    out2.i32_thing = 5;
    Xtruct2 in2 = testClient.testNest(out2);
    in = in2.struct_thing;
    assertEquals(out2, in2);

    /**
     * MAP TEST
     */
    Map<Integer,Integer> mapout = new HashMap<Integer,Integer>();
    for (int i = 0; i < 5; ++i) {
      mapout.put(i, i - 10);
    }
    Map<Integer,Integer> mapin = testClient.testMap(mapout);
    assertEquals(mapout, mapin);

    /**
     * SET TEST
     */
    Set<Integer> setout = new HashSet<Integer>();
    for (int i = -2; i < 3; ++i) {
      setout.add(i);
    }
    Set<Integer> setin = testClient.testSet(setout);
    assertEquals(setout, setin);

    /**
     * LIST TEST
     */
    List<Integer> listout = new ArrayList<Integer>();
    for (int i = -2; i < 3; ++i) {
      listout.add(i);
    }
    List<Integer> listin = testClient.testList(listout);
    assertEquals(listout, listin);

    /**
     * ENUM TEST
     */
    int ret = testClient.testEnum(Numberz.ONE);

    ret = testClient.testEnum(Numberz.TWO);

    ret = testClient.testEnum(Numberz.THREE);

    ret = testClient.testEnum(Numberz.FIVE);

    ret = testClient.testEnum(Numberz.EIGHT);

    /**
     * TYPEDEF TEST
     */
    long uid = testClient.testTypedef(309858235082523L);

    /**
     * NESTED MAP TEST
     */
    Map<Integer,Map<Integer,Integer>> mm =
      testClient.testMapMap(1);

    /**
     * INSANITY TEST
     */
    insane = new Insanity();
    insane.userMap = new HashMap<Integer, Long>();
    insane.userMap.put(Numberz.FIVE, (long)5000);
    Xtruct truck = new Xtruct();
    truck.string_thing = "Truck";
    truck.byte_thing = (byte)8;
    truck.i32_thing = 8;
    truck.i64_thing = 8;
    insane.xtructs = new ArrayList<Xtruct>();
    insane.xtructs.add(truck);
    Map<Long,Map<Integer,Insanity>> whoa =
      testClient.testInsanity(insane);
    for (long key : whoa.keySet()) {
      Map<Integer,Insanity> val = whoa.get(key);

      for (int k2 : val.keySet()) {
        Insanity v2 = val.get(k2);
        Map<Integer, Long> userMap = v2.userMap;
        if (userMap != null) {
          for (int k3 : userMap.keySet()) {
          }
        }

        List<Xtruct> xtructs = v2.xtructs;
        if (xtructs != null) {
          for (Xtruct x : xtructs) {
          }
        }
      }
    }

    // Test oneway
    long startOneway = System.nanoTime();
    testClient.testOneway(3);
    long onewayElapsedMillis = (System.nanoTime() - startOneway) / 1000000;
    if (onewayElapsedMillis > 200) {
      throw new TException("Oneway test failed: took " +
                          Long.toString(onewayElapsedMillis) +
                          "ms");
    }

    // Just do some basic testing that the event handler is called.
    assertTrue(preRead > 0);
    assertTrue(postRead > 0);
    assertTrue(preWrite > 0);
    assertTrue(postWrite > 0);
    assertTrue(getContext > 0);
    assertEquals(0, handlerError);

    transport.close();
  }

}
