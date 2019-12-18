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

import com.facebook.thrift.java.test.MySimpleStruct;
import com.facebook.thrift.java.test.MySimpleUnion;
import com.facebook.thrift.protocol.TBinaryProtocol;
import com.facebook.thrift.protocol.TCompactJSONProtocol;
import com.facebook.thrift.protocol.TCompactProtocol;
import com.facebook.thrift.protocol.TProtocol;
import com.facebook.thrift.protocol.TProtocolException;
import com.facebook.thrift.protocol.TProtocolFactory;
import com.facebook.thrift.transport.TMemoryBuffer;
import com.facebook.thrift.transport.TTransportException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import junit.framework.TestCase;
import org.junit.Test;
import thrift.test.Empty;
import thrift.test.RandomStuff;
import thrift.test.StructWithAUnion;
import thrift.test.TestUnion;

public class UnionTest extends TestCase {

  @Test
  public static void testBasic() throws Exception {
    TestUnion union = new TestUnion();

    if (union.isSet()) {
      throw new RuntimeException("new union with default constructor counts as set!");
    }

    if (union.getFieldValue() != null) {
      throw new RuntimeException("unset union didn't return null for value");
    }

    union = new TestUnion(TestUnion.I32_FIELD, 25);

    if ((Integer) union.getFieldValue() != 25) {
      throw new RuntimeException("set i32 field didn't come out as planned");
    }

    if ((Integer) union.getFieldValue(TestUnion.I32_FIELD) != 25) {
      throw new RuntimeException("set i32 field didn't come out of TBase getFieldValue");
    }

    try {
      union.getFieldValue(TestUnion.STRING_FIELD);
      throw new RuntimeException("was expecting an exception around wrong set field");
    } catch (IllegalArgumentException e) {
      // cool!
    }

    System.out.println(union);

    union = new TestUnion();
    union.setI32_field(1);
    if (union.getI32_field() != 1) {
      throw new RuntimeException("didn't get the right value for i32 field!");
    }

    try {
      union.getString_field();
      throw new RuntimeException("should have gotten an exception");
    } catch (Exception e) {
      // sweet
    }
  }

  @Test
  public static void testEquality() throws Exception {
    TestUnion union = new TestUnion(TestUnion.I32_FIELD, 25);

    TestUnion otherUnion = new TestUnion(TestUnion.STRING_FIELD, "blah!!!");

    if (union.equals(otherUnion)) {
      throw new RuntimeException("shouldn't be equal");
    }

    otherUnion = new TestUnion(TestUnion.I32_FIELD, 400);

    if (union.equals(otherUnion)) {
      throw new RuntimeException("shouldn't be equal");
    }

    otherUnion = new TestUnion(TestUnion.OTHER_I32_FIELD, 25);

    if (union.equals(otherUnion)) {
      throw new RuntimeException("shouldn't be equal");
    }
  }

  @Test
  public static void testSerialization() throws Exception {
    TestUnion union = new TestUnion(TestUnion.I32_FIELD, 25);

    TMemoryBuffer buf = new TMemoryBuffer(0);
    TProtocol proto = new TBinaryProtocol(buf);

    union.write(proto);

    TestUnion u2 = new TestUnion();

    u2.read(proto);

    if (!u2.equals(union)) {
      throw new RuntimeException("serialization fails!");
    }

    StructWithAUnion swau = new StructWithAUnion(u2);

    buf = new TMemoryBuffer(0);
    proto = new TBinaryProtocol(buf);

    swau.write(proto);

    StructWithAUnion swau2 = new StructWithAUnion();
    if (swau2.equals(swau)) {
      throw new RuntimeException("objects match before they are supposed to!");
    }
    swau2.read(proto);
    if (!swau2.equals(swau)) {
      throw new RuntimeException("objects don't match when they are supposed to!");
    }

    // this should NOT throw an exception.
    buf = new TMemoryBuffer(0);
    proto = new TBinaryProtocol(buf);

    swau.write(proto);
    new Empty().read(proto);
  }

  @Test
  public static void testJSONSerialization() throws Exception {
    TDeserializer deserializer = new TDeserializer(new TCompactJSONProtocol.Factory());

    TSerializer serializer = new TSerializer(new TCompactJSONProtocol.Factory());

    // Deserialize empty union
    TestUnion emptyUnion = new TestUnion();
    String emptyUnionJSON = "{}";
    TestUnion union = new TestUnion(TestUnion.I32_FIELD, 25);
    deserializer.fromString(union, emptyUnionJSON);

    if (!emptyUnion.equals(union)) {
      throw new RuntimeException("Empty union objects don't match when they are supposed to!");
    }

    // Serialize union then deserialize it. Should be the same.
    TestUnion union2 = new TestUnion(TestUnion.I32_FIELD, 25);

    String unionJSON = serializer.toString(union2, "UTF-8");
    TestUnion union3 = new TestUnion();
    deserializer.fromString(union3, unionJSON);

    if (!union3.equals(union2)) {
      throw new RuntimeException("Union objects don't match when they are supposed to!");
    }

    // Serialize union with inner list then deserialize it. Should be the same.
    List<RandomStuff> randomList = new ArrayList<RandomStuff>();
    randomList.add(new RandomStuff(1, 2, 3, 4, new ArrayList<Integer>(), null, 10l, 10.5));
    TestUnion unionWithList = new TestUnion(TestUnion.STRUCT_LIST, randomList);

    String unionWithListJSON = serializer.toString(unionWithList, "UTF-8");
    TestUnion unionWithList2 = new TestUnion();
    deserializer.fromString(unionWithList2, unionWithListJSON);

    if (!unionWithList2.equals(unionWithList)) {
      throw new RuntimeException("Union list objects don't match when they are supposed to!");
    }

    if (!unionWithList2.getStruct_list().equals(randomList)) {
      throw new RuntimeException("Inner list objects don't match when they are supposed to!");
    }

    // Serialize struct with union then deserialize it. Should be the same.
    StructWithAUnion swau = new StructWithAUnion(union2);

    String swauJSON = serializer.toString(swau, "UTF-8");
    StructWithAUnion swau2 = new StructWithAUnion();
    deserializer.fromString(swau2, swauJSON);

    if (!swau2.equals(swau)) {
      throw new RuntimeException("StructWithAUnion objects don't match when they are supposed to!");
    }
  }

  @Test
  public static void testEmptyUnionBinarySerialization() throws Exception {
    TMemoryBuffer buf = new TMemoryBuffer(0);
    TProtocol binaryProto = new TBinaryProtocol(buf);

    TestUnion emptyUnion = new TestUnion();
    // Should throw a TProtocolException when writing an empty union
    try {
      emptyUnion.write(binaryProto);
      throw new RuntimeException("Should not be able to write empty TUnion!");
    } catch (TProtocolException e) {
      // If we get a TProtocolException then the test passed.
    }
  }

  @Test
  public static void testEmptyUnionCompactSerialization() throws Exception {
    TMemoryBuffer buf = new TMemoryBuffer(0);
    TProtocolFactory compactFactory = new TCompactProtocol.Factory();
    TProtocol compactProto = compactFactory.getProtocol(buf);

    TestUnion emptyUnion = new TestUnion();
    // Should throw a TProtocolException when writing an empty union
    try {
      emptyUnion.write(compactProto);
      throw new RuntimeException("Should not be able to write empty TUnion!");
    } catch (TProtocolException e) {
      // If we get a TProtocolException then the test passed.
    }
  }

  @Test
  public static void testEmptyUnionBinaryDeserialization() throws Exception {
    TMemoryBuffer buf = new TMemoryBuffer(0);
    TProtocol binaryProto = new TBinaryProtocol(buf);

    TestUnion emptyUnion = new TestUnion();
    // Should throw a TTransportException when reading no bytes
    try {
      emptyUnion.read(binaryProto);
      throw new RuntimeException("Should throw error reading no bytes to TUnion!");
    } catch (TTransportException e) {
      // If we get a TTransportException then the test passed.
    }
  }

  @Test
  public static void testEmptyUnionCompactDeserialization() throws Exception {
    TMemoryBuffer buf = new TMemoryBuffer(0);
    TProtocolFactory compactFactory = new TCompactProtocol.Factory();
    TProtocol compactProto = compactFactory.getProtocol(buf);

    TestUnion emptyUnion = new TestUnion();
    // Should throw a TTransportException when reading no bytes
    try {
      emptyUnion.read(compactProto);
      throw new RuntimeException("Should throw error reading no bytes to TUnion!");
    } catch (TTransportException e) {
      // If we get a TTransportException then the test passed.
    }
  }

  @Test
  public static void testAndroidUnion() throws Exception {
    TMemoryBuffer buf = new TMemoryBuffer(0);
    TProtocolFactory factory = new TCompactProtocol.Factory();
    TProtocol proto = factory.getProtocol(buf);

    MySimpleUnion union = MySimpleUnion.caseOne(61753);
    union.write(proto);

    com.facebook.thrift.android.test.MySimpleUnion androidUnion =
        new com.facebook.thrift.android.test.MySimpleUnion();
    androidUnion.read(proto);

    assertThat(androidUnion.getCaseOne(), equalTo(union.getCaseOne()));
  }

  @Test
  public static void testInvalidUnion() throws Exception {
    try {
      MySimpleUnion invalidUnion = new MySimpleUnion(1, null);
      assertFalse(true);
    } catch (IllegalArgumentException e) {
      assertThat(e.getMessage(), equalTo("TUnion value for field id '1' can't be null!"));
    }
  }

  @Test
  public static void testToStringOnEmptyUnion() throws Exception {
    MySimpleUnion union = new MySimpleUnion();
    assertThat(union.toString(), equalTo("<MySimpleUnion uninitialized>"));

    com.facebook.thrift.android.test.MySimpleUnion androidUnion =
        new com.facebook.thrift.android.test.MySimpleUnion();
    assertThat(union.toString(), equalTo("<MySimpleUnion uninitialized>"));
  }

  @Test
  public static void testUnionConstructor() throws Exception {
    MySimpleUnion union =
        new MySimpleUnion(MySimpleUnion.CASEFOUR, new MySimpleStruct(1L, "blabla"));
    com.facebook.thrift.android.test.MySimpleUnion androidUnion =
        new com.facebook.thrift.android.test.MySimpleUnion(
            MySimpleUnion.CASEFOUR,
            new com.facebook.thrift.android.test.MySimpleStruct(1L, "blabla"));

    MySimpleUnion union5 = new MySimpleUnion(MySimpleUnion.CASEFIVE, Arrays.asList("foo", "bar"));
    com.facebook.thrift.android.test.MySimpleUnion androidUnion5 =
        new com.facebook.thrift.android.test.MySimpleUnion(
            MySimpleUnion.CASEFIVE, Arrays.asList("foo", "bar"));
  }
}
