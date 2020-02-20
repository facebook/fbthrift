/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.facebook.thrift;

import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

import com.facebook.thrift.javaswift.test.MySimpleStruct;
import com.facebook.thrift.javaswift.test.MySimpleUnion;
import com.facebook.thrift.javaswift.test.SimpleCollectionStruct;
import com.facebook.thrift.javaswift.test.SimpleStructTypes;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

public class DeprecatedToSwiftTest extends junit.framework.TestCase {

  public static void main(String[] args) throws Exception {
    testWriteEqualDeprecatedReadSimple();
    testWriteEqualDeprecatedReadStructs();
    testWriteEqualDeprecatedCollectionStructs();
    testWriteEqualDeprecatedReadSimpleUnion();
  }

  public static void testWriteEqualDeprecatedReadSimple() throws Exception {
    long idValue = 4444444444444444444L;
    String nameValue = "Hello Thrift Team";

    org.apache.thrift.transport.TMemoryBuffer buf =
        new org.apache.thrift.transport.TMemoryBuffer(0);
    org.apache.thrift.protocol.TProtocolFactory factory =
        new org.apache.thrift.protocol.TCompactProtocol.Factory();

    org.apache.thrift.protocol.TProtocol apacheProto = factory.getProtocol(buf);
    MySimpleStruct struct1 = new MySimpleStruct(idValue, nameValue);
    struct1.write0(apacheProto);
    com.facebook.thrift.java.test.MySimpleStruct structJavaDeprecated =
        new com.facebook.thrift.java.test.MySimpleStruct();

    ApacheToFacebookProtocolAdapter protocol = new ApacheToFacebookProtocolAdapter(apacheProto);
    structJavaDeprecated.read(protocol);

    assertThat(structJavaDeprecated.getId(), equalTo(idValue));
    assertThat(structJavaDeprecated.getName(), equalTo(nameValue));
  }

  public static void testWriteEqualDeprecatedReadSimpleUnion() throws Exception {
    long idValue = 4444444444444444444L;
    org.apache.thrift.transport.TMemoryBuffer buf =
        new org.apache.thrift.transport.TMemoryBuffer(0);
    org.apache.thrift.protocol.TProtocolFactory factory =
        new org.apache.thrift.protocol.TCompactProtocol.Factory();

    org.apache.thrift.protocol.TProtocol apacheProto = factory.getProtocol(buf);
    MySimpleUnion union = MySimpleUnion.fromCaseOne(idValue);
    union.write0(apacheProto);

    com.facebook.thrift.java.test.MySimpleUnion unionJavaDeprecated =
        new com.facebook.thrift.java.test.MySimpleUnion();
    ApacheToFacebookProtocolAdapter protocol = new ApacheToFacebookProtocolAdapter(apacheProto);
    unionJavaDeprecated.read(protocol);

    assertThat(unionJavaDeprecated.getCaseOne(), equalTo(idValue));
  }

  public static void testWriteEqualDeprecatedReadStructs() throws Exception {
    String msg = "Hello Thrift Team";
    boolean b = true;
    byte y = 111;
    short i = 0;
    int j = Integer.MAX_VALUE;
    long k = 4444444444444444444L;
    double d = -5555555.5555;

    org.apache.thrift.transport.TMemoryBuffer buf =
        new org.apache.thrift.transport.TMemoryBuffer(0);
    org.apache.thrift.protocol.TProtocolFactory factory =
        new org.apache.thrift.protocol.TCompactProtocol.Factory();

    org.apache.thrift.protocol.TProtocol apacheProto = factory.getProtocol(buf);
    SimpleStructTypes struct1 = new SimpleStructTypes(msg, b, y, i, j, k, d);
    struct1.write0(apacheProto);
    com.facebook.thrift.java.test.SimpleStructTypes structJavaDeprecated =
        new com.facebook.thrift.java.test.SimpleStructTypes();

    ApacheToFacebookProtocolAdapter protocol = new ApacheToFacebookProtocolAdapter(apacheProto);
    structJavaDeprecated.read(protocol);

    assertThat(structJavaDeprecated.getMsg(), equalTo(msg));
    assertThat(structJavaDeprecated.isB(), equalTo(b));
    assertThat(structJavaDeprecated.getY(), equalTo(y));
    assertThat(structJavaDeprecated.getI(), equalTo(i));
    assertThat(structJavaDeprecated.getJ(), equalTo(j));
    assertThat(structJavaDeprecated.getK(), equalTo(k));
    assertThat(structJavaDeprecated.getD(), equalTo(d));
  }

  public static void testWriteEqualDeprecatedCollectionStructs() throws Exception {
    List<Double> lDouble = new ArrayList<Double>();
    lDouble.add(5555555.5555);
    lDouble.add(0.0);
    lDouble.add(-5555555.5555);
    List<Short> lShort = new ArrayList<Short>();
    lShort.add((short) 111);
    lShort.add((short) 0);
    lShort.add((short) -111);
    Map<Integer, String> mIntegerString = new HashMap<Integer, String>();
    mIntegerString.put(-1, "Hello");
    mIntegerString.put(0, "Thrift");
    mIntegerString.put(1, "Team");
    Map<String, String> mStringString = new HashMap<String, String>();
    mStringString.put("Team", "Hello");
    mStringString.put("Thrift", "Thrift");
    mStringString.put("Hello", "Team");
    Set<Long> sLong = new HashSet<Long>();
    sLong.add(4444444444444444444L);
    sLong.add(0L);
    sLong.add(-4444444444444444444L);

    org.apache.thrift.transport.TMemoryBuffer buf =
        new org.apache.thrift.transport.TMemoryBuffer(0);
    org.apache.thrift.protocol.TProtocolFactory factory =
        new org.apache.thrift.protocol.TCompactProtocol.Factory();

    org.apache.thrift.protocol.TProtocol apacheProto = factory.getProtocol(buf);
    SimpleCollectionStruct struct1 =
        new SimpleCollectionStruct(lDouble, lShort, mIntegerString, mStringString, sLong);
    struct1.write0(apacheProto);
    com.facebook.thrift.java.test.SimpleCollectionStruct structJavaDeprecated =
        new com.facebook.thrift.java.test.SimpleCollectionStruct();

    ApacheToFacebookProtocolAdapter protocol = new ApacheToFacebookProtocolAdapter(apacheProto);
    structJavaDeprecated.read(protocol);

    assertThat(structJavaDeprecated.getLDouble(), equalTo(lDouble));
    assertThat(structJavaDeprecated.getLShort(), equalTo(lShort));
    assertThat(structJavaDeprecated.getMIntegerString(), equalTo(mIntegerString));
    assertThat(structJavaDeprecated.getMStringString(), equalTo(mStringString));
    assertThat(structJavaDeprecated.getSLong(), equalTo(sLong));
  }
}
