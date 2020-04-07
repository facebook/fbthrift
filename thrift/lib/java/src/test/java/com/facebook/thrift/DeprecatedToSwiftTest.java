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

import static org.hamcrest.CoreMatchers.is;
import static org.hamcrest.Matchers.equalTo;
import static org.junit.Assert.assertThat;

import com.facebook.thrift.javaswift.test.ComplexNestedStruct;
import com.facebook.thrift.javaswift.test.MyOptioalStruct;
import com.facebook.thrift.javaswift.test.MySimpleStruct;
import com.facebook.thrift.javaswift.test.MySimpleUnion;
import com.facebook.thrift.javaswift.test.SimpleCollectionStruct;
import com.facebook.thrift.javaswift.test.SimpleStructTypes;
import com.facebook.thrift.javaswift.test.SmallEnum;
import com.facebook.thrift.javaswift.test.TypeRemapped;
import it.unimi.dsi.fastutil.ints.Int2LongArrayMap;
import it.unimi.dsi.fastutil.ints.Int2ObjectArrayMap;
import it.unimi.dsi.fastutil.longs.Long2ObjectArrayMap;
import java.nio.ByteBuffer;
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
    testSwiftWriteVsDeprecatedReadNestedStructs();
    testSwiftReadVsWriteNestedStructs();
    testReadEqualWriteStructTypes();
    testReadEqualWriteCollectionStructs();
    testReadEqualWriteSimpleUnion();
    testReadEqualWriteOptionalFields();
    testSwiftTypeAnnontationsFields();
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

  public static void testReadEqualWriteSimpleUnion() throws Exception {
    long idValue = 4444444444444444444L;
    String nameValue = "Hello Thrift Team";

    org.apache.thrift.transport.TMemoryBuffer buf =
        new org.apache.thrift.transport.TMemoryBuffer(0);
    org.apache.thrift.protocol.TProtocolFactory factory =
        new org.apache.thrift.protocol.TCompactProtocol.Factory();
    org.apache.thrift.protocol.TProtocol apacheProto = factory.getProtocol(buf);

    MySimpleStruct struct1 = new MySimpleStruct(idValue, nameValue);
    MySimpleUnion union = MySimpleUnion.fromCaseFour(struct1);
    union.write0(apacheProto);
    MySimpleUnion unionCreatedFromRead = MySimpleUnion.read0(apacheProto);
    assertThat(unionCreatedFromRead.getCaseFour(), equalTo(struct1));
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

  public static void testReadEqualWriteStructTypes() throws Exception {
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

    SimpleStructTypes structCreatedFromRead = SimpleStructTypes.read0(apacheProto);
    assertThat(structCreatedFromRead.getMsg(), equalTo(msg));
    assertThat(structCreatedFromRead.isB(), equalTo(b));
    assertThat(structCreatedFromRead.getY(), equalTo(y));
    assertThat(structCreatedFromRead.getI(), equalTo(i));
    assertThat(structCreatedFromRead.getJ(), equalTo(j));
    assertThat(structCreatedFromRead.getK(), equalTo(k));
    assertThat(structCreatedFromRead.getD(), equalTo(d));
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

  public static void testSwiftWriteVsDeprecatedReadNestedStructs() throws Exception {

    Set<Set<Integer>> setOfSetOfInt = new HashSet<Set<Integer>>();
    Set<Integer> sIntOne = new HashSet<Integer>();
    sIntOne.add(-1);
    sIntOne.add(0);
    Set<Integer> sIntTwo = new HashSet<Integer>();
    sIntTwo.add(1);
    sIntTwo.add(2);
    setOfSetOfInt.add(sIntOne);
    setOfSetOfInt.add(sIntTwo);

    List<List<SmallEnum>> listOfListOfEnum = new ArrayList<List<SmallEnum>>();
    List<SmallEnum> lENUMOne = new ArrayList<SmallEnum>();
    lENUMOne.add(SmallEnum.RED);
    lENUMOne.add(SmallEnum.BLUE);
    List<SmallEnum> lEnumTwo = new ArrayList<SmallEnum>();
    lEnumTwo.add(SmallEnum.GREEN);
    listOfListOfEnum.add(lENUMOne);
    listOfListOfEnum.add(lEnumTwo);
    List<SmallEnum> lEnumThree = new ArrayList<SmallEnum>();
    lEnumThree.add(SmallEnum.GREEN);
    lEnumThree.add(SmallEnum.GREEN);
    List<List<SmallEnum>> listOfListOfEnumTwo = new ArrayList<List<SmallEnum>>();
    listOfListOfEnumTwo.add(lEnumThree);

    List<List<MySimpleStruct>> listOfListOfMyStruct = new ArrayList<List<MySimpleStruct>>();
    long idValueOne = 4444444444444444444L;
    String nameValueOne = "Hello Thrift Team";
    MySimpleStruct structOne = new MySimpleStruct(idValueOne, nameValueOne);
    long idValueTwo = 5L;
    String nameValueTwo = "Hello Batman!";
    MySimpleStruct structTwo = new MySimpleStruct(idValueTwo, nameValueTwo);
    List<MySimpleStruct> lStructOne = new ArrayList<MySimpleStruct>();
    lStructOne.add(structOne);
    List<MySimpleStruct> lStructTwo = new ArrayList<MySimpleStruct>();
    lStructTwo.add(structTwo);
    listOfListOfMyStruct.add(lStructOne);
    listOfListOfMyStruct.add(lStructTwo);

    Set<List<List<String>>> setOfListOfListOfString = new HashSet<List<List<String>>>();
    List<String> lstringOne = new ArrayList<String>();
    List<String> lstringTwo = new ArrayList<String>();
    lstringOne.add(nameValueOne);
    lstringTwo.add(nameValueTwo);
    List<List<String>> listOfListString = new ArrayList<List<String>>();
    listOfListString.add(lstringOne);
    listOfListString.add(lstringTwo);
    List<List<String>> listOfListStringEmpty = new ArrayList<List<String>>();
    setOfListOfListOfString.add(listOfListString);
    setOfListOfListOfString.add(listOfListStringEmpty);

    HashMap<Integer, List<List<SmallEnum>>> mapKeyIntValListOfListOfEnum =
        new HashMap<Integer, List<List<SmallEnum>>>();
    mapKeyIntValListOfListOfEnum.put(101, listOfListOfEnum);
    mapKeyIntValListOfListOfEnum.put(100, listOfListOfEnumTwo);

    Map<Map<Integer, String>, Map<String, String>> mapKeyMapValMap =
        new HashMap<Map<Integer, String>, Map<String, String>>();
    Map<Integer, String> mIntegerString = new HashMap<Integer, String>();
    mIntegerString.put(-1, "Hello");
    mIntegerString.put(0, "Thrift");
    mIntegerString.put(1, "Team");
    Map<String, String> mStringString = new HashMap<String, String>();
    mStringString.put("Team", "Hello");
    mStringString.put("Thrift", "Thrift");
    mStringString.put("Batman", "Thanos");
    mapKeyMapValMap.put(mIntegerString, mStringString);

    MySimpleUnion mySimpleUnion = MySimpleUnion.fromCaseOne(idValueOne);

    org.apache.thrift.transport.TMemoryBuffer buf =
        new org.apache.thrift.transport.TMemoryBuffer(0);
    org.apache.thrift.protocol.TProtocolFactory factory =
        new org.apache.thrift.protocol.TCompactProtocol.Factory();

    org.apache.thrift.protocol.TProtocol apacheProto = factory.getProtocol(buf);
    ComplexNestedStruct complexNestedStruct =
        new ComplexNestedStruct(
            setOfSetOfInt,
            listOfListOfEnum,
            listOfListOfMyStruct,
            setOfListOfListOfString,
            mapKeyIntValListOfListOfEnum,
            mapKeyMapValMap,
            mySimpleUnion);

    complexNestedStruct.write0(apacheProto);
    com.facebook.thrift.java.test.ComplexNestedStruct structJavaDeprecated =
        new com.facebook.thrift.java.test.ComplexNestedStruct();

    ApacheToFacebookProtocolAdapter protocol = new ApacheToFacebookProtocolAdapter(apacheProto);
    structJavaDeprecated.read(protocol);

    assertThat(structJavaDeprecated.getSetOfSetOfInt(), equalTo(setOfSetOfInt));
    assertThat(structJavaDeprecated.getSetOfListOfListOfString(), equalTo(setOfListOfListOfString));
    assertThat(structJavaDeprecated.getMapKeyMapValMap(), equalTo(mapKeyMapValMap));
    assertThat(structJavaDeprecated.getMyUnion().getCaseOne(), equalTo(idValueOne));
    // First list of listOfListOfMyStruct
    assertThat(
        structJavaDeprecated.getListOfListOfMyStruct().get(0).get(0).getId(), equalTo(idValueOne));
    assertThat(
        structJavaDeprecated.getListOfListOfMyStruct().get(0).get(0).getName(),
        equalTo(nameValueOne));
    // Second list of listOfListOfMyStruct
    assertThat(
        structJavaDeprecated.getListOfListOfMyStruct().get(1).get(0).getId(), equalTo(idValueTwo));
    assertThat(
        structJavaDeprecated.getListOfListOfMyStruct().get(1).get(0).getName(),
        equalTo(nameValueTwo));

    // red=1, blue=2, green=3
    assertThat(structJavaDeprecated.getListOfListOfEnum().get(0).get(0).getValue(), equalTo(1));
    assertThat(structJavaDeprecated.getListOfListOfEnum().get(0).get(1).getValue(), equalTo(2));
    assertThat(structJavaDeprecated.getListOfListOfEnum().get(1).get(0).getValue(), equalTo(3));
    assertThat(
        structJavaDeprecated.getMapKeyIntValListOfListOfEnum().get(101).get(0).get(0).getValue(),
        equalTo(1));
    assertThat(
        structJavaDeprecated.getMapKeyIntValListOfListOfEnum().get(101).get(0).get(1).getValue(),
        equalTo(2));
    assertThat(
        structJavaDeprecated.getMapKeyIntValListOfListOfEnum().get(101).get(1).get(0).getValue(),
        equalTo(3));
    assertThat(
        structJavaDeprecated.getMapKeyIntValListOfListOfEnum().get(100).get(0).get(0).getValue(),
        equalTo(3));
    assertThat(
        structJavaDeprecated.getMapKeyIntValListOfListOfEnum().get(100).get(0).get(1).getValue(),
        equalTo(3));
  }

  public static void testSwiftReadVsWriteNestedStructs() throws Exception {
    Set<Set<Integer>> setOfSetOfInt = new HashSet<Set<Integer>>();
    Set<Integer> sIntOne = new HashSet<Integer>();
    sIntOne.add(-1);
    sIntOne.add(0);
    Set<Integer> sIntTwo = new HashSet<Integer>();
    sIntTwo.add(1);
    sIntTwo.add(2);
    setOfSetOfInt.add(sIntOne);
    setOfSetOfInt.add(sIntTwo);

    List<List<SmallEnum>> listOfListOfEnum = new ArrayList<List<SmallEnum>>();
    List<SmallEnum> lENUMOne = new ArrayList<SmallEnum>();
    lENUMOne.add(SmallEnum.RED);
    lENUMOne.add(SmallEnum.BLUE);
    List<SmallEnum> lEnumTwo = new ArrayList<SmallEnum>();
    lEnumTwo.add(SmallEnum.GREEN);
    listOfListOfEnum.add(lENUMOne);
    listOfListOfEnum.add(lEnumTwo);
    List<SmallEnum> lEnumThree = new ArrayList<SmallEnum>();
    lEnumThree.add(SmallEnum.GREEN);
    lEnumThree.add(SmallEnum.GREEN);
    List<List<SmallEnum>> listOfListOfEnumTwo = new ArrayList<List<SmallEnum>>();
    listOfListOfEnumTwo.add(lEnumThree);

    List<List<MySimpleStruct>> listOfListOfMyStruct = new ArrayList<List<MySimpleStruct>>();
    long idValueOne = 4444444444444444444L;
    String nameValueOne = "Hello Thrift Team";
    MySimpleStruct structOne = new MySimpleStruct(idValueOne, nameValueOne);
    long idValueTwo = 5L;
    String nameValueTwo = "Hello Batman!";
    MySimpleStruct structTwo = new MySimpleStruct(idValueTwo, nameValueTwo);
    List<MySimpleStruct> lStructOne = new ArrayList<MySimpleStruct>();
    lStructOne.add(structOne);
    List<MySimpleStruct> lStructTwo = new ArrayList<MySimpleStruct>();
    lStructTwo.add(structTwo);
    listOfListOfMyStruct.add(lStructOne);
    listOfListOfMyStruct.add(lStructTwo);

    Set<List<List<String>>> setOfListOfListOfString = new HashSet<List<List<String>>>();
    List<String> lstringOne = new ArrayList<String>();
    List<String> lstringTwo = new ArrayList<String>();
    lstringOne.add(nameValueOne);
    lstringTwo.add(nameValueTwo);
    List<List<String>> listOfListString = new ArrayList<List<String>>();
    listOfListString.add(lstringOne);
    listOfListString.add(lstringTwo);
    List<List<String>> listOfListStringEmpty = new ArrayList<List<String>>();
    setOfListOfListOfString.add(listOfListString);
    setOfListOfListOfString.add(listOfListStringEmpty);

    HashMap<Integer, List<List<SmallEnum>>> mapKeyIntValListOfListOfEnum =
        new HashMap<Integer, List<List<SmallEnum>>>();
    mapKeyIntValListOfListOfEnum.put(101, listOfListOfEnum);
    mapKeyIntValListOfListOfEnum.put(100, listOfListOfEnumTwo);

    Map<Map<Integer, String>, Map<String, String>> mapKeyMapValMap =
        new HashMap<Map<Integer, String>, Map<String, String>>();
    Map<Integer, String> mIntegerString = new HashMap<Integer, String>();
    mIntegerString.put(-1, "Hello");
    mIntegerString.put(0, "Thrift");
    mIntegerString.put(1, "Team");
    Map<String, String> mStringString = new HashMap<String, String>();
    mStringString.put("Team", "Hello");
    mStringString.put("Thrift", "Thrift");
    mStringString.put("Batman", "Thanos");
    mapKeyMapValMap.put(mIntegerString, mStringString);

    MySimpleUnion mySimpleUnion = MySimpleUnion.fromCaseOne(idValueOne);

    org.apache.thrift.transport.TMemoryBuffer buf =
        new org.apache.thrift.transport.TMemoryBuffer(0);
    org.apache.thrift.protocol.TProtocolFactory factory =
        new org.apache.thrift.protocol.TCompactProtocol.Factory();

    org.apache.thrift.protocol.TProtocol apacheProto = factory.getProtocol(buf);
    ComplexNestedStruct complexNestedStruct =
        new ComplexNestedStruct(
            setOfSetOfInt,
            listOfListOfEnum,
            listOfListOfMyStruct,
            setOfListOfListOfString,
            mapKeyIntValListOfListOfEnum,
            mapKeyMapValMap,
            mySimpleUnion);

    complexNestedStruct.write0(apacheProto);
    ComplexNestedStruct structCreatedFromRead = ComplexNestedStruct.read0(apacheProto);

    assertThat(structCreatedFromRead.getSetOfSetOfInt(), equalTo(setOfSetOfInt));
    assertThat(
        structCreatedFromRead.getSetOfListOfListOfString(), equalTo(setOfListOfListOfString));
    assertThat(structCreatedFromRead.getMapKeyMapValMap(), equalTo(mapKeyMapValMap));
    assertThat(structCreatedFromRead.getMyUnion().getCaseOne(), equalTo(idValueOne));
    // First list of listOfListOfMyStruct
    assertThat(
        structCreatedFromRead.getListOfListOfMyStruct().get(0).get(0).getId(), equalTo(idValueOne));
    assertThat(
        structCreatedFromRead.getListOfListOfMyStruct().get(0).get(0).getName(),
        equalTo(nameValueOne));
    // Second list of listOfListOfMyStruct
    assertThat(
        structCreatedFromRead.getListOfListOfMyStruct().get(1).get(0).getId(), equalTo(idValueTwo));
    assertThat(
        structCreatedFromRead.getListOfListOfMyStruct().get(1).get(0).getName(),
        equalTo(nameValueTwo));

    // red=1, blue=2, green=3
    assertThat(structCreatedFromRead.getListOfListOfEnum().get(0).get(0).getValue(), equalTo(1));
    assertThat(structCreatedFromRead.getListOfListOfEnum().get(0).get(1).getValue(), equalTo(2));
    assertThat(structCreatedFromRead.getListOfListOfEnum().get(1).get(0).getValue(), equalTo(3));
    assertThat(
        structCreatedFromRead.getMapKeyIntValListOfListOfEnum().get(101).get(0).get(0).getValue(),
        equalTo(1));
    assertThat(
        structCreatedFromRead.getMapKeyIntValListOfListOfEnum().get(101).get(0).get(1).getValue(),
        equalTo(2));
    assertThat(
        structCreatedFromRead.getMapKeyIntValListOfListOfEnum().get(101).get(1).get(0).getValue(),
        equalTo(3));
    assertThat(
        structCreatedFromRead.getMapKeyIntValListOfListOfEnum().get(100).get(0).get(0).getValue(),
        equalTo(3));
    assertThat(
        structCreatedFromRead.getMapKeyIntValListOfListOfEnum().get(100).get(0).get(1).getValue(),
        equalTo(3));
  }

  public static void testReadEqualWriteCollectionStructs() throws Exception {
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
    SimpleCollectionStruct exampleStruct =
        new SimpleCollectionStruct(lDouble, lShort, mIntegerString, mStringString, sLong);
    exampleStruct.write0(apacheProto);

    SimpleCollectionStruct structCreatedFromRead = SimpleCollectionStruct.read0(apacheProto);
    assertThat(structCreatedFromRead.getLDouble(), equalTo(lDouble));
    assertThat(structCreatedFromRead.getLShort(), equalTo(lShort));
    assertThat(structCreatedFromRead.getMIntegerString(), equalTo(mIntegerString));
    assertThat(structCreatedFromRead.getSLong(), equalTo(sLong));
  }

  public static void testReadEqualWriteOptionalFields() throws Exception {
    Map<Integer, String> mIntegerString = new HashMap<Integer, String>();
    mIntegerString.put(-1, "Hello");
    mIntegerString.put(0, "Thrift");
    mIntegerString.put(1, "Team");

    long idValue = 4444444444444444444L;
    String nameValue = "Hello Thrift Team";
    MySimpleStruct structOne = new MySimpleStruct(idValue, nameValue);

    org.apache.thrift.transport.TMemoryBuffer buf =
        new org.apache.thrift.transport.TMemoryBuffer(0);
    org.apache.thrift.protocol.TProtocolFactory factory =
        new org.apache.thrift.protocol.TCompactProtocol.Factory();

    org.apache.thrift.protocol.TProtocol apacheProto = factory.getProtocol(buf);
    MyOptioalStruct myOptioalStruct =
        new MyOptioalStruct.Builder()
            .setId(idValue)
            .setName("Steven")
            .setAgeShort((short) 15)
            .setAgeShortOptional((short) 15)
            .setAgeLong(44L)
            .setAgeLongOptional(44L)
            .setMySimpleStruct(structOne)
            .setMIntegerString(mIntegerString)
            .build();

    myOptioalStruct.write0(apacheProto);
    MyOptioalStruct structCreatedFromRead = MyOptioalStruct.read0(apacheProto);

    assertThat(structCreatedFromRead.getAgeLong(), equalTo(44L));
    assertThat(structCreatedFromRead.getAgeLongOptional(), equalTo(44L));
    assertThat(structCreatedFromRead.getAgeLongOptional().getClass().isPrimitive(), is(false));
    assertThat(structCreatedFromRead.getAgeShort(), equalTo((short) 15));
    assertThat(structCreatedFromRead.getAgeShortOptional(), equalTo((short) 15));
    assertThat(structCreatedFromRead.getAgeShortOptional().getClass().isPrimitive(), is(false));
    assertThat(structCreatedFromRead.getMIntegerStringOptional(), equalTo(null));
    assertThat(structCreatedFromRead.getSmallEnumOptional(), equalTo(null));
    assertThat(structCreatedFromRead.getMySmallEnum().getValue(), equalTo(0));
  }

  public static void testSwiftTypeAnnontationsFields() throws Exception {
    long idValue = 4444444444444444444L;
    String nameValue = "Hello Thrift Team";
    Long2ObjectArrayMap<String> longStrMap = new Long2ObjectArrayMap<String>();
    longStrMap.put(idValue, nameValue);
    Int2LongArrayMap intLongMap = new Int2LongArrayMap();
    intLongMap.put(1, 1L);
    intLongMap.put(100, 100L);
    Int2ObjectArrayMap<Int2LongArrayMap> ioMap = new Int2ObjectArrayMap<Int2LongArrayMap>();
    ioMap.put(123, intLongMap);
    List<Int2LongArrayMap> listOfFMap = new ArrayList<Int2LongArrayMap>();
    listOfFMap.add(intLongMap);

    int capacity = 4;
    ByteBuffer byteBuffer = ByteBuffer.allocate(capacity);
    byteBuffer.put((byte) 20);
    byteBuffer.put((byte) 30);
    byteBuffer.put((byte) 40);
    byteBuffer.put((byte) 50);

    TypeRemapped typeRemapped =
        new TypeRemapped.Builder()
            .setLsMap(longStrMap)
            .setIoMap(ioMap)
            .setMyListOfFMaps(listOfFMap)
            .setMyListOfFMaps(listOfFMap)
            .setByteBufferForBinary(byteBuffer)
            .build();

    org.apache.thrift.transport.TMemoryBuffer buf =
        new org.apache.thrift.transport.TMemoryBuffer(0);
    org.apache.thrift.protocol.TProtocolFactory factory =
        new org.apache.thrift.protocol.TCompactProtocol.Factory();
    org.apache.thrift.protocol.TProtocol apacheProto = factory.getProtocol(buf);
    typeRemapped.write0(apacheProto);
    TypeRemapped typeRemappedRead = TypeRemapped.read0(apacheProto);

    assertThat(typeRemappedRead.getLsMap(), equalTo(longStrMap));
    assertThat(typeRemappedRead.getLsMap().getClass(), equalTo(Long2ObjectArrayMap.class));
    assertThat(typeRemappedRead.getIoMap(), equalTo(ioMap));
    assertThat(typeRemappedRead.getIoMap().getClass(), equalTo(Int2ObjectArrayMap.class));
    assertThat(typeRemappedRead.getMyListOfFMaps(), equalTo(listOfFMap));
    assertThat(typeRemappedRead.getMyListOfFMaps().getClass(), equalTo(ArrayList.class));
    assertThat(typeRemappedRead.getByteBufferForBinary(), equalTo(byteBuffer));
    assertThat(
        ByteBuffer.class.isAssignableFrom(typeRemappedRead.getByteBufferForBinary().getClass()),
        equalTo(true));
  }
}
