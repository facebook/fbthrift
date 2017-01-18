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
import com.facebook.thrift.protocol.TBinaryProtocol;
import com.facebook.thrift.protocol.THeaderProtocol;
import com.facebook.thrift.protocol.TProtocol;
import com.facebook.thrift.protocol.TProtocolFactory;
import com.facebook.thrift.server.TConnectionContext;
import com.facebook.thrift.server.THsHaServer;
import com.facebook.thrift.server.TNonblockingServer;
import com.facebook.thrift.server.TServer;
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

public class ClientHandlerTest extends junit.framework.TestCase {
  public int TEST_PORT = -1;

  public static final String identity = "me";

  public enum ServerType { NONBLOCKING, HSHA };

  public class ServerThread implements Runnable {
    private TServer serverEngine;
    private THeaderProtocol.Factory tProtocolFactory;
    private THeaderTransport.Factory tTransportFactory;
    private TNonblockingServerSocket tNonblockingServerSocket;
    private ThriftTest.Processor testProcessor;
    TProcessorFactory pFactory;

    public ServerThread(ServerType serverType) throws TException {
      // processor
      TestHandler testHandler = new TestHandler();
      testProcessor = new ThriftTest.Processor(testHandler);
      pFactory = new TProcessorFactory(testProcessor);

      // Transport
      tNonblockingServerSocket =  new TNonblockingServerSocket(0);
      TEST_PORT = tNonblockingServerSocket.getLocalPort();

      // Protocol factory
      List<THeaderTransport.ClientTypes> clientTypes =
        new ArrayList<THeaderTransport.ClientTypes>();
      tTransportFactory = new THeaderTransport.Factory(clientTypes);
      tProtocolFactory = new THeaderProtocol.Factory();

      if (serverType == ServerType.NONBLOCKING) {
        makeNonblocking();
      } else if (serverType == ServerType.HSHA) {
        makeHsHa();
      }
    }

    public void makeNonblocking() throws TTransportException {
      // ThreadPool Server
      serverEngine = new TNonblockingServer(pFactory,
                                            tNonblockingServerSocket,
                                            tTransportFactory,
                                            tProtocolFactory);
    }

    public void makeHsHa() throws TTransportException {
      // ThreadPool Server
      serverEngine = new THsHaServer(pFactory,
                                     tNonblockingServerSocket,
                                     tTransportFactory,
                                     tProtocolFactory);
    }

    public void run() {
      // Run it
      serverEngine.serve();
    }

    public void stop() {
      serverEngine.stop();
      tNonblockingServerSocket.close();
    }
  }

  long preRead = 0;
  long postRead = 0;
  long preWrite = 0;
  long postWrite = 0;
  long getContext = 0;
  long handlerError = 0;

  public class ClientEventHandler extends TProcessorEventHandler {
    long handlerCalls = 0;

    @Override
    public Object getContext(String fn_name, TConnectionContext context) {
      handlerCalls++;
      getContext++;
      assertEquals(fn_name, "ThriftTest.testVoid");
      assertEquals(handlerCalls, 1);
      return new Object();
    }

    @Override
    public void preWrite(Object handler_context, String fn_name, TBase res) {
      handlerCalls++;
      preWrite++;
      assertEquals(fn_name, "ThriftTest.testVoid");
      assertEquals(handlerCalls, 2);
    }

    @Override
    public void postWrite(Object handler_context, String fn_name, TBase res) {
      handlerCalls++;
      postWrite++;
      assertEquals(fn_name, "ThriftTest.testVoid");
      assertEquals(handlerCalls, 3);
    }

    @Override
    public void preRead(Object handler_context, String fn_name) {
      handlerCalls++;
      preRead++;
      assertEquals(fn_name, "ThriftTest.testVoid");
      assertEquals(handlerCalls, 4);
    }

    @Override
    public void postRead(Object handler_context, String fn_name, TBase args) {
      handlerCalls++;
      postRead++;
      assertEquals(fn_name, "ThriftTest.testVoid");
      assertEquals(handlerCalls, 5);
    }

    @Override
    public void handlerError(Object handler_context, String fn_name,
            Throwable th) {
      handlerCalls++;
      handlerError++;
      assertEquals(fn_name, "ThriftTest.testVoid");
    }

    public void checkContextCalls() {
      assertEquals(handlerCalls, 5);
    }
  }

  public static class TestHandler implements ThriftTest.Iface {

    public TestHandler() {}

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

  @Test
  public void testClientEventHandler() throws Exception {
    doServerClient(ServerType.NONBLOCKING, false, true, false);
  }

  public void doServerClient(ServerType serverType, boolean unframed,
                             boolean framed, boolean header)
    throws TException, InterruptedException {
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
    //rohan
    ClientEventHandler handler = new ClientEventHandler();
    testClient.addEventHandler(handler);
    Insanity insane = new Insanity();

    /**
     * CONNECT TEST
     */
    transport.open();

    //long start = System.nanoTime();

    /**
     * VOID TEST
     */
    testClient.testVoid();

    // Just do some basic testing that the event handler is called.
    handler.checkContextCalls();
    assertTrue(preRead > 0);
    assertTrue(postRead > 0);
    assertTrue(preWrite > 0);
    assertTrue(postWrite > 0);
    assertTrue(getContext > 0);
    assertEquals(0, handlerError);

    transport.close();
  }

}
