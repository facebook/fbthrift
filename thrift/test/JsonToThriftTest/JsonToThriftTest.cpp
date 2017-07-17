/*
 * Copyright 2004-present Facebook, Inc.
 *
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

#include <folly/json.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/myBinaryStruct_types.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/myStringStruct_types.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/mySetStruct_types.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/myDoubleStruct_types.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/myByteStruct_types.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/myI16Struct_types.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/myI32Struct_types.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/myBoolStruct_types.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/myComplexStruct_types.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/myDefaultStruct_types.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/myMixedStruct_types.h>
#include <thrift/test/JsonToThriftTest/gen-cpp/myMapStruct_types.h>
#include <thrift/lib/cpp/Thrift.h>

#include <memory>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/protocol/TJSONProtocol.h>
#include <thrift/lib/cpp/protocol/TSimpleJSONProtocol.h>
#include <fstream>
#include <iostream>
#include <limits>

#include <gtest/gtest.h>

using namespace std;

using apache::thrift::protocol::TProtocol;
using apache::thrift::protocol::TJSONProtocol;
using apache::thrift::protocol::TSimpleJSONProtocol;
using apache::thrift::transport::TMemoryBuffer;

template <typename T>
static void testSimpleJSON(T dataStruct){
   std::string simpleJsonText = serializeJSON(dataStruct);
   T parsedStruct;
   parsedStruct.readFromJson(simpleJsonText.c_str());
   EXPECT_TRUE(parsedStruct == dataStruct);
   parsedStruct.readFromJson(simpleJsonText.c_str(), simpleJsonText.size());
   EXPECT_TRUE(parsedStruct == dataStruct);
}

template <typename T>
static std::string serializeJSON(T dataStruct,
                   bool simpleJSON = true,
                   string fileName = string()
                   ) {

    std::shared_ptr<TMemoryBuffer> buffer(new TMemoryBuffer());

    TProtocol* prot = simpleJSON ? new TSimpleJSONProtocol(buffer) :
                                   new TJSONProtocol(buffer);

    std::shared_ptr<TProtocol> protocol(prot);

    dataStruct.write(protocol.get());

    uint8_t* buf = nullptr;
    uint32_t size;
    buffer->getBuffer(&buf, &size);

    if (!fileName.empty()){
        string jsonFileName = fileName + ".json";
        cout << " Writing JSON to " << fileName;
        FILE* outFile = fopen(jsonFileName.c_str(), "w");
        fwrite(buf, sizeof(uint8_t), size, outFile);
        fclose(outFile);
    }

    std::string ret((char*)buf, size);
    return ret;
}

static void testSimpleDoubleNanJSON(myDoubleStruct dataStruct){
   std::string simpleJsonText = serializeJSON(dataStruct);
   myDoubleStruct parsedStruct;
   parsedStruct.readFromJson(simpleJsonText.c_str());
   EXPECT_TRUE(std::isnan(parsedStruct.a));
   EXPECT_NE(parsedStruct.a, parsedStruct.a);
   EXPECT_TRUE(std::isnan(dataStruct.a));
   EXPECT_NE(dataStruct.a, dataStruct.a);
}

TEST(JsonToThriftTest, SimpleJSON_ComplexSerialization) {
   myComplexStruct thriftComplexObj;
   myMixedStruct thriftMixedObj;

   mySimpleStruct thriftSimpleObj;
   thriftSimpleObj.a = true;
   thriftSimpleObj.b = 120;
   thriftSimpleObj.c = 9990;
   thriftSimpleObj.d = -9990;
   thriftSimpleObj.e = -1;
   thriftSimpleObj.f = 0.9;
   thriftSimpleObj.g = "Simple String";

   mySuperSimpleStruct superSimple;
   superSimple.a = 121;
   thriftMixedObj.a.push_back(18);
   thriftMixedObj.b.push_back(superSimple);
   thriftMixedObj.c.insert(std::make_pair("flame",-8));
   thriftMixedObj.c.insert(std::make_pair("fire",-191));
   thriftMixedObj.d.insert(std::make_pair("key1",superSimple));
   thriftMixedObj.e.insert(88);thriftMixedObj.e.insert(89);

   thriftComplexObj.a = thriftSimpleObj;
   thriftComplexObj.b.push_back(25);
   thriftComplexObj.b.push_back(24);

   for(int i=0;i<3;i++) {
      mySimpleStruct obj;
      obj.a = true;
      obj.b = 80 + i;
      obj.c = 7000+i;
      obj.e = -i;
      obj.f = -0.5 * i;
      string elmName = "element" + folly::to<std::string>(i+1);
      obj.g = elmName.c_str();
      thriftComplexObj.c.insert(std::make_pair(elmName, thriftSimpleObj));
   }

   thriftComplexObj.e = EnumTest::EnumTwo;

   testSimpleJSON(thriftMixedObj);
   testSimpleJSON(thriftComplexObj);
}

TEST(JsonToThriftTest, SimpleJSON_DefaultSerialization) {
  myDefaultStruct thriftDefaultObj;
  EXPECT_EQ(thriftDefaultObj.a.size(), 3);
  EXPECT_EQ(thriftDefaultObj.b.size(), 1);
  EXPECT_EQ(thriftDefaultObj.b[0].a.size(), 1);

  mySuperSimpleDefaultStruct superSimple;
  superSimple.a.clear();
  superSimple.a.push_back(121);
  thriftDefaultObj.a.clear();
  thriftDefaultObj.a.push_back(18);
  thriftDefaultObj.b.clear();
  thriftDefaultObj.b.push_back(superSimple);
  thriftDefaultObj.c.clear();
  thriftDefaultObj.c.insert(std::make_pair("flame", -8));
  thriftDefaultObj.c.insert(std::make_pair("fire", -191));
  thriftDefaultObj.d.clear();
  thriftDefaultObj.d.insert(std::make_pair("key1", superSimple));
  thriftDefaultObj.e.clear();
  thriftDefaultObj.e.insert(88);
  thriftDefaultObj.e.insert(89);

  testSimpleJSON(thriftDefaultObj);
}

TEST(JsonToThriftTest, SimpleJSON_BasicSerialization) {

   mySimpleStruct thriftSimpleObj;
   myDoubleStruct thriftDoubleObj;
   myBoolStruct thriftBoolObj1, thriftBoolObj2;
   myByteStruct thriftByteObj;
   myStringStruct thriftStringObj;
   myI16Struct thriftI16Obj;
   myI32Struct thriftI32Obj;

   thriftSimpleObj.a = false;
   thriftSimpleObj.b = 87;
   thriftSimpleObj.c = 7880;
   thriftSimpleObj.d = -7880;
   thriftSimpleObj.e = -1;
   thriftSimpleObj.f = -0.1;
   thriftSimpleObj.g = "T-bone";

   thriftDoubleObj.a = 100.5;
   testSimpleJSON(thriftDoubleObj);
   thriftDoubleObj.a = numeric_limits<double>::infinity();
   testSimpleJSON(thriftDoubleObj);
   thriftDoubleObj.a = -numeric_limits<double>::infinity();
   testSimpleJSON(thriftDoubleObj);
   // We need a special test for double since
   // numeric_limits<double>::quiet_NaN() !=
   // numeric_limits<double>::quiet_NaN()
   thriftDoubleObj.a = numeric_limits<double>::quiet_NaN();
   testSimpleDoubleNanJSON(thriftDoubleObj);

   thriftBoolObj1.a = true;
   thriftBoolObj2.a = false;

   thriftByteObj.a = 115;
   thriftStringObj.a = "testing";

   thriftI16Obj.a = 4567;
   thriftI32Obj.a = 12131415;

   testSimpleJSON(thriftSimpleObj);
   testSimpleJSON(thriftBoolObj1);
   testSimpleJSON(thriftBoolObj2);
   testSimpleJSON(thriftByteObj);
   testSimpleJSON(thriftStringObj);
   testSimpleJSON(thriftI16Obj);
   testSimpleJSON(thriftI32Obj);
}

TEST(JsonToThriftTest, SimpleStructMissingNonRequiredField) {

  // jsonSimpleT1 is to test whether __isset is set properly, given
  // that all the required field has value: field a's value is missing
  string jsonSimpleT("{\"c\":16,\"d\":32,\"e\":64,\"b\":8,"
      "\"f\":0.99,\"g\":\"Hello\"}");
  mySimpleStruct thriftSimpleObj;

  thriftSimpleObj.readFromJson(jsonSimpleT.c_str());


  EXPECT_TRUE(!thriftSimpleObj.__isset.a);
  EXPECT_EQ(thriftSimpleObj.b, 8);
  EXPECT_TRUE(thriftSimpleObj.__isset.b);
  // field c doesn't have __isset field, since it is required.
  EXPECT_EQ(thriftSimpleObj.c, 16);
  EXPECT_EQ(thriftSimpleObj.d, 32);
  EXPECT_TRUE(thriftSimpleObj.__isset.d);
  EXPECT_EQ(thriftSimpleObj.e, 64);
  EXPECT_TRUE(thriftSimpleObj.__isset.e);
  EXPECT_EQ(thriftSimpleObj.f, 0.99);
  EXPECT_TRUE(thriftSimpleObj.__isset.f);
  EXPECT_EQ(thriftSimpleObj.g, "Hello");
  EXPECT_TRUE(thriftSimpleObj.__isset.g);
}

TEST(JsonToThriftTest, NegativeBoundaryCase) {

  string jsonByteTW("{\"a\":-128}");
  myByteStruct thriftByteObjW;
  try {
    thriftByteObjW.readFromJson(jsonByteTW.c_str());
    ADD_FAILURE();
  } catch (apache::thrift::TException &e) {
  }

  string jsonByteT("{\"a\":-127}");
  myByteStruct thriftByteObj;
  thriftByteObj.readFromJson(jsonByteT.c_str());
  EXPECT_EQ(thriftByteObj.a, (-1)*0x7f);

  string jsonI16TW("{\"a\":-32768}");
  myI16Struct thriftI16ObjW;
  try {
    thriftI16ObjW.readFromJson(jsonI16TW.c_str());
    ADD_FAILURE();
  } catch (apache::thrift::TException &e) {
  }

  string jsonI16T("{\"a\":-32767}");
  myI16Struct thriftI16Obj;
  thriftI16Obj.readFromJson(jsonI16T.c_str());
  EXPECT_EQ(thriftI16Obj.a, (-1)*0x7fff);

  string jsonI32TW("{\"a\":-2147483648}");
  myI32Struct thriftI32ObjW;
  try {
    thriftI32ObjW.readFromJson(jsonI32TW.c_str());
    ADD_FAILURE();
  } catch (apache::thrift::TException &e) {
  }

  string jsonI32T("{\"a\":-2147483647}");
  myI32Struct thriftI32Obj;
  thriftI32Obj.readFromJson(jsonI32T.c_str());
  EXPECT_EQ(thriftI32Obj.a, (-1)*0x7fffffff);
}

TEST(JsonToThriftTest, PassingWrongType) {

  string jsonI32T("{\"a\":\"hello\"}");
  myI32Struct thriftI32Obj;
  try {
    thriftI32Obj.readFromJson(jsonI32T.c_str());
    ADD_FAILURE();
  } catch (std::exception &e) {
  }
}

//fields in JSON that are not present in the thrift type spec
TEST(JsonToThriftTest, MissingField) {
  string jsonSimpleT("{\"c\":16,\"d\":32,\"e\":64,\"b\":8,"
      "\"f\":0.99,\"g\":\"Hello\", \"extra\":12}");
  mySimpleStruct thriftSimpleObj;
  thriftSimpleObj.readFromJson(jsonSimpleT.c_str());

  EXPECT_TRUE(!thriftSimpleObj.__isset.a);
  EXPECT_EQ(thriftSimpleObj.b, 8);
  EXPECT_TRUE(thriftSimpleObj.__isset.b);
  // field c doesn't have __isset field, since it is required.
  EXPECT_EQ(thriftSimpleObj.c, 16);
  EXPECT_EQ(thriftSimpleObj.d, 32);
  EXPECT_TRUE(thriftSimpleObj.__isset.d);
  EXPECT_EQ(thriftSimpleObj.e, 64);
  EXPECT_TRUE(thriftSimpleObj.__isset.e);
  EXPECT_EQ(thriftSimpleObj.f, 0.99);
  EXPECT_TRUE(thriftSimpleObj.__isset.f);
  EXPECT_EQ(thriftSimpleObj.g, "Hello");
  EXPECT_TRUE(thriftSimpleObj.__isset.g);
}

TEST(JsonToThriftTest, BoundaryCase) {

  // jsonSimpleT2 is to test whether the generated code throw exeption
  // if the required field doesn't have value : field c's value is
  // missing
  string jsonSimpleT("{\"a\":true,\"d\":32,\"e\":64,\"b\":8,"
      "\"f\":0.99,\"g\":\"Hello\"}");
  mySimpleStruct thriftSimpleObj;
  try {
    thriftSimpleObj.readFromJson(jsonSimpleT.c_str());
    ADD_FAILURE();
  } catch (apache::thrift::TException &e) {
  }

  string jsonByteTW("{\"a\":128}");
  myByteStruct thriftByteObjW;
  try {
    thriftByteObjW.readFromJson(jsonByteTW.c_str());
    ADD_FAILURE();
  } catch (apache::thrift::TException &e) {
  }

  string jsonByteT("{\"a\":127}");
  myByteStruct thriftByteObj;
  thriftByteObj.readFromJson(jsonByteT.c_str());
  EXPECT_EQ(thriftByteObj.a, 0x7f);

  string jsonI16TW("{\"a\":32768}");
  myI16Struct thriftI16ObjW;
  try {
    thriftI16ObjW.readFromJson(jsonI16TW.c_str());
    ADD_FAILURE();
  } catch (apache::thrift::TException &e) {
  }

  string jsonI16T("{\"a\":32767}");
  myI16Struct thriftI16Obj;
  thriftI16Obj.readFromJson(jsonI16T.c_str());
  EXPECT_EQ(thriftI16Obj.a, 0x7fff);

  string jsonI32TW("{\"a\":2147483648}");
  myI32Struct thriftI32ObjW;
  try {
    thriftI32ObjW.readFromJson(jsonI32TW.c_str());
    ADD_FAILURE();
  } catch (apache::thrift::TException &e) {
  }

  string jsonI32T("{\"a\":2147483647}");
  myI32Struct thriftI32Obj;
  thriftI32Obj.readFromJson(jsonI32T.c_str());
  EXPECT_EQ(thriftI32Obj.a, 0x7fffffff);

  string jsonBoolTW("{\"a\":2}");
  myBoolStruct thriftBoolObjW;
  thriftBoolObjW.readFromJson(jsonBoolTW.c_str());
  // readFromJson for primitive types uses the asXxx() functions from folly
  // (e.g. asBool for bool types) which are very forgiving (e.g. 2 => true).
  EXPECT_EQ(thriftBoolObjW.a, true);
}

TEST(JsonToThriftTest, ComplexTypeMissingRequiredFieldInMember) {

  string jsonT("{\"a\":true,\"c\":16,\"d\":32,\"e\":64,\"b\":8,"
      "\"f\":0.99,\"g\":\"Hello\"}");
  string jsonComplexT
    ("{\"a\":" + jsonT +  ", \"b\":[3,2,1], \"c\":{\"key1\":" +
    jsonT + ",\"key2\":{\"d\":320, \"f\":0.001}}}");

  myComplexStruct thriftComplexObj;
  try {
    thriftComplexObj.readFromJson(jsonComplexT.c_str());
    ADD_FAILURE();
  } catch (apache::thrift::TException &e) {
  }
}

TEST(JsonToThriftTest, ComplexTypeTest) {

  string jsonT("{\"a\":true,\"c\":16,\"d\":32,\"e\":64,\"b\":8,"
      "\"f\":0.99,\"g\":\"Hello\"}");
  string jsonComplexT
    ("{\"a\":" + jsonT +  ", \"b\":[3,2,1], \"c\":{\"key1\":" +
    jsonT + ",\"key2\":{\"c\":20, \"d\":320, \"f\":0.001}}}");

  myComplexStruct thriftComplexObj;
  thriftComplexObj.readFromJson(jsonComplexT.c_str());

  EXPECT_EQ(thriftComplexObj.a.b, 8);
  EXPECT_EQ(thriftComplexObj.a.c, 16);
  EXPECT_EQ(thriftComplexObj.a.d, 32);
  EXPECT_EQ(thriftComplexObj.a.e, 64);
  EXPECT_EQ(thriftComplexObj.a.f, 0.99);
  EXPECT_EQ(thriftComplexObj.a.g, "Hello");

  EXPECT_EQ(thriftComplexObj.b[0], 3);
  EXPECT_EQ(thriftComplexObj.b[1], 2);
  EXPECT_EQ(thriftComplexObj.b[2], 1);

  EXPECT_EQ(thriftComplexObj.c["key1"].b, 8);
  EXPECT_EQ(thriftComplexObj.c["key1"].c, 16);
  EXPECT_EQ(thriftComplexObj.c["key1"].d, 32);
  EXPECT_EQ(thriftComplexObj.c["key1"].e, 64);
  EXPECT_EQ(thriftComplexObj.c["key1"].f, 0.99);
  EXPECT_EQ(thriftComplexObj.c["key1"].g, "Hello");
  EXPECT_EQ(thriftComplexObj.c["key2"].c, 20);
  EXPECT_EQ(thriftComplexObj.c["key2"].d, 320);
  EXPECT_EQ(thriftComplexObj.c["key2"].f, 0.001);
}

TEST(JsonToThriftTest, SetTypeTest) {

  string jsonT("{\"a\":[1,2,3]}");
  mySetStruct thriftSetObj;
  thriftSetObj.readFromJson(jsonT.c_str());
  EXPECT_TRUE(thriftSetObj.__isset.a);
  EXPECT_EQ(thriftSetObj.a.size(), 3);
  EXPECT_TRUE(thriftSetObj.a.find(2) != thriftSetObj.a.end());
  EXPECT_TRUE(thriftSetObj.a.find(5) ==  thriftSetObj.a.end());
}

TEST(JsonToThriftTest, MixedStructTest) {
  string jsonT("{\"a\":[1],\"b\":[{\"a\":1}],\"c\":{\"hello\":1},"
      "\"d\":{\"hello\":{\"a\":1}},\"e\":[1]}");
  myMixedStruct thriftMixedObj;
  thriftMixedObj.readFromJson(jsonT.c_str());
  EXPECT_EQ(thriftMixedObj.a[0], 1);
  EXPECT_EQ(thriftMixedObj.b[0].a, 1);
  EXPECT_EQ(thriftMixedObj.c["hello"], 1);
  EXPECT_EQ(thriftMixedObj.d["hello"].a, 1);
  EXPECT_TRUE(thriftMixedObj.e.find(1) != thriftMixedObj.e.end());
}

TEST(JsonToThriftTest, MapTypeTest) {
  string stringJson("\"stringMap\": {\"a\":\"A\", \"b\":\"B\"}");
  string boolJson("\"boolMap\": {\"true\":\"True\", \"false\":\"False\"}");
  string byteJson("\"byteMap\": {\"1\":\"one\", \"2\":\"two\"}");
  string doubleJson("\"doubleMap\": {\"0.1\":\"0.one\", \"0.2\":\"0.two\"}");
  string enumJson("\"enumMap\": {\"1\":\"male\", \"2\":\"female\"}");
  string json = "{" + stringJson + ", " + boolJson + ", " + byteJson + ", " +
    doubleJson + ", " + enumJson + "}";
  myMapStruct mapStruct;
  mapStruct.readFromJson(json.c_str());
  EXPECT_EQ(mapStruct.stringMap.size(), 2);
  EXPECT_EQ(mapStruct.stringMap["a"], "A");
  EXPECT_EQ(mapStruct.stringMap["b"], "B");
  EXPECT_EQ(mapStruct.boolMap.size(), 2);
  EXPECT_EQ(mapStruct.boolMap[true], "True");
  EXPECT_EQ(mapStruct.boolMap[false], "False");
  EXPECT_EQ(mapStruct.byteMap.size(), 2);
  EXPECT_EQ(mapStruct.byteMap[1], "one");
  EXPECT_EQ(mapStruct.byteMap[2], "two");
  EXPECT_EQ(mapStruct.doubleMap.size(), 2);
  EXPECT_EQ(mapStruct.doubleMap[0.1], "0.one");
  EXPECT_EQ(mapStruct.doubleMap[0.2], "0.two");
  EXPECT_EQ(mapStruct.enumMap.size(), 2);
  EXPECT_EQ(mapStruct.enumMap[Gender::MALE], "male");
  EXPECT_EQ(mapStruct.enumMap[Gender::FEMALE], "female");
}

TEST(JsonToThriftTest, EmptyStringTest) {
  string jsonT("{\"a\":\"\"}");
  myStringStruct thriftStringObj;
  thriftStringObj.readFromJson(jsonT.c_str());
  EXPECT_EQ(thriftStringObj.a, "");
}

TEST(JsonToThriftTest, BinaryTypeTest) {
  string jsonT("{\"a\":\"abc\"}");
  myBinaryStruct thriftBinaryObj;
  thriftBinaryObj.readFromJson(jsonT.c_str());
  EXPECT_EQ(thriftBinaryObj.a, "abc");
}
