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

import java.io.ByteArrayInputStream;

import com.facebook.thrift.*;
import com.facebook.thrift.protocol.*;
import com.facebook.thrift.transport.*;

import thrift.test.*;

public class SerializationBenchmark {
  private final static int HOW_MANY = 10000000;

  public static void main(String[] args) throws Exception {
    TProtocolFactory factory = new TBinaryProtocol.Factory();

    OneOfEach ooe = new OneOfEach();
    ooe.im_true   = true;
    ooe.im_false  = false;
    ooe.a_bite    = (byte)0xd6;
    ooe.integer16 = 27000;
    ooe.integer32 = 1<<24;
    ooe.integer64 = (long)6000 * 1000 * 1000;
    ooe.double_precision = Math.PI;
    ooe.some_characters  = "JSON THIS! \"\u0001";
    ooe.base64 = new byte[]{1,2,3,(byte)255};

    testSerialization(factory, ooe);
    testDeserialization(factory, ooe, OneOfEach.class);
  }

  public static void testSerialization(TProtocolFactory factory, TBase object) throws Exception {
    TTransport trans = new TTransport() {
      public void write(byte[] bin, int x, int y) throws TTransportException {}
      public int read(byte[] bin, int x, int y) throws TTransportException {return 0;}
      public void close() {}
      public void open() {}
      public boolean isOpen() {return true;}
    };

    TProtocol proto = factory.getProtocol(trans);

    long startTime = System.currentTimeMillis();
    for (int i = 0; i < HOW_MANY; i++) {
      object.write(proto);
    }
    long endTime = System.currentTimeMillis();

    System.out.println("Test time: " + (endTime - startTime) + " ms");
  }

  public static <T extends TBase> void testDeserialization(TProtocolFactory factory, T object, Class<T> klass) throws Exception {
    TMemoryBuffer buf = new TMemoryBuffer(0);
    object.write(factory.getProtocol(buf));
    byte[] serialized = new byte[100*1024];
    buf.read(serialized, 0, 100*1024);

    long startTime = System.currentTimeMillis();
    for (int i = 0; i < HOW_MANY; i++) {
      T o2 = klass.newInstance();
      o2.read(factory.getProtocol(new TIOStreamTransport(new ByteArrayInputStream(serialized))));
    }
    long endTime = System.currentTimeMillis();

    System.out.println("Test time: " + (endTime - startTime) + " ms");
  }
}
