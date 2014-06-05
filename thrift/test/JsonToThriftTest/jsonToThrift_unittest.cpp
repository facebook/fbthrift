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


#include <boost/test/unit_test.hpp>

#include "folly/json.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/myBinaryStruct_types.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/myStringStruct_types.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/mySetStruct_types.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/myDoubleStruct_types.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/myByteStruct_types.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/myI16Struct_types.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/myI32Struct_types.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/myBoolStruct_types.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/myComplexStruct_types.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/myDefaultStruct_types.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/myMixedStruct_types.h"
#include "thrift/test/JsonToThriftTest/gen-cpp/myMapStruct_types.h"
#include "thrift/lib/cpp/Thrift.h"

#include <boost/lexical_cast.hpp>
#include <memory>
#include "thrift/lib/cpp/transport/TBufferTransports.h"
#include "thrift/lib/cpp/protocol/TJSONProtocol.h"
#include "thrift/lib/cpp/protocol/TSimpleJSONProtocol.h"
#include <fstream>
#include <iostream>
#include <limits>

using namespace std;

using apache::thrift::protocol::TProtocol;
using apache::thrift::protocol::TJSONProtocol;
using apache::thrift::protocol::TSimpleJSONProtocol;
using apache::thrift::transport::TMemoryBuffer;
using std::shared_ptr;

template <typename T>
static void testSimpleJSON(T dataStruct){
   std::string simpleJsonText = serializeJSON(dataStruct);
   T parsedStruct;
   parsedStruct.readFromJson(simpleJsonText.c_str());
   BOOST_CHECK(parsedStruct == dataStruct);
   parsedStruct.readFromJson(simpleJsonText.c_str(), simpleJsonText.size());
   BOOST_CHECK(parsedStruct == dataStruct);
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
   BOOST_CHECK(parsedStruct.a != parsedStruct.a &&
               dataStruct.a != dataStruct.a);
}

BOOST_AUTO_TEST_CASE(SimpleJSON_ComplexSerialization) {
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
      string elmName = "element" + boost::lexical_cast<std::string>(i+1);
      obj.g = elmName.c_str();
      thriftComplexObj.c.insert(std::make_pair(elmName, thriftSimpleObj));
   }

   thriftComplexObj.e = EnumTest::EnumTwo;

   testSimpleJSON(thriftMixedObj);
   testSimpleJSON(thriftComplexObj);
}

BOOST_AUTO_TEST_CASE(SimpleJSON_DefaultSerialization) {
  myDefaultStruct thriftDefaultObj;
  BOOST_CHECK_EQUAL(thriftDefaultObj.a.size(), 3);
  BOOST_CHECK_EQUAL(thriftDefaultObj.b.size(), 1);
  BOOST_CHECK_EQUAL(thriftDefaultObj.b[0].a.size(), 1);

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

BOOST_AUTO_TEST_CASE(SimpleJSON_BasicSerialization) {

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

BOOST_AUTO_TEST_CASE(SimpleStructMissingNonRequiredField) {

  // jsonSimpleT1 is to test whether __isset is set properly, given
  // that all the required field has value: field a's value is missing
  string jsonSimpleT("{\"c\":16,\"d\":32,\"e\":64,\"b\":8,"
      "\"f\":0.99,\"g\":\"Hello\"}");
  mySimpleStruct thriftSimpleObj;

  thriftSimpleObj.readFromJson(jsonSimpleT.c_str());


  BOOST_CHECK(!thriftSimpleObj.__isset.a);
  BOOST_CHECK_EQUAL(thriftSimpleObj.b, 8);
  BOOST_CHECK(thriftSimpleObj.__isset.b);
  // field c doesn't have __isset field, since it is required.
  BOOST_CHECK_EQUAL(thriftSimpleObj.c, 16);
  BOOST_CHECK_EQUAL(thriftSimpleObj.d, 32);
  BOOST_CHECK(thriftSimpleObj.__isset.d);
  BOOST_CHECK_EQUAL(thriftSimpleObj.e, 64);
  BOOST_CHECK(thriftSimpleObj.__isset.e);
  BOOST_CHECK_EQUAL(thriftSimpleObj.f, 0.99);
  BOOST_CHECK(thriftSimpleObj.__isset.f);
  BOOST_CHECK_EQUAL(thriftSimpleObj.g, "Hello");
  BOOST_CHECK(thriftSimpleObj.__isset.g);
}

BOOST_AUTO_TEST_CASE(NegativeBoundaryCase) {

  string jsonByteTW("{\"a\":-128}");
  myByteStruct thriftByteObjW;
  try {
    thriftByteObjW.readFromJson(jsonByteTW.c_str());
    BOOST_CHECK(false);
  } catch (apache::thrift::TException &e) {
  }

  string jsonByteT("{\"a\":-127}");
  myByteStruct thriftByteObj;
  thriftByteObj.readFromJson(jsonByteT.c_str());
  BOOST_CHECK_EQUAL(thriftByteObj.a, (-1)*0x7f);

  string jsonI16TW("{\"a\":-32768}");
  myI16Struct thriftI16ObjW;
  try {
    thriftI16ObjW.readFromJson(jsonI16TW.c_str());
    BOOST_CHECK(false);
  } catch (apache::thrift::TException &e) {
  }

  string jsonI16T("{\"a\":-32767}");
  myI16Struct thriftI16Obj;
  thriftI16Obj.readFromJson(jsonI16T.c_str());
  BOOST_CHECK_EQUAL(thriftI16Obj.a, (-1)*0x7fff);

  string jsonI32TW("{\"a\":-2147483648}");
  myI32Struct thriftI32ObjW;
  try {
    thriftI32ObjW.readFromJson(jsonI32TW.c_str());
    BOOST_CHECK(false);
  } catch (apache::thrift::TException &e) {
  }

  string jsonI32T("{\"a\":-2147483647}");
  myI32Struct thriftI32Obj;
  thriftI32Obj.readFromJson(jsonI32T.c_str());
  BOOST_CHECK_EQUAL(thriftI32Obj.a, (-1)*0x7fffffff);
}

BOOST_AUTO_TEST_CASE(PassingWrongType) {

  string jsonI32T("{\"a\":\"hello\"}");
  myI32Struct thriftI32Obj;
  try {
    thriftI32Obj.readFromJson(jsonI32T.c_str());
    BOOST_CHECK(false);
  } catch (std::exception &e) {
  }
}

//fields in JSON that are not present in the thrift type spec
BOOST_AUTO_TEST_CASE(MissingField) {
  string jsonSimpleT("{\"c\":16,\"d\":32,\"e\":64,\"b\":8,"
      "\"f\":0.99,\"g\":\"Hello\", \"extra\":12}");
  mySimpleStruct thriftSimpleObj;
  thriftSimpleObj.readFromJson(jsonSimpleT.c_str());

  BOOST_CHECK(!thriftSimpleObj.__isset.a);
  BOOST_CHECK_EQUAL(thriftSimpleObj.b, 8);
  BOOST_CHECK(thriftSimpleObj.__isset.b);
  // field c doesn't have __isset field, since it is required.
  BOOST_CHECK_EQUAL(thriftSimpleObj.c, 16);
  BOOST_CHECK_EQUAL(thriftSimpleObj.d, 32);
  BOOST_CHECK(thriftSimpleObj.__isset.d);
  BOOST_CHECK_EQUAL(thriftSimpleObj.e, 64);
  BOOST_CHECK(thriftSimpleObj.__isset.e);
  BOOST_CHECK_EQUAL(thriftSimpleObj.f, 0.99);
  BOOST_CHECK(thriftSimpleObj.__isset.f);
  BOOST_CHECK_EQUAL(thriftSimpleObj.g, "Hello");
  BOOST_CHECK(thriftSimpleObj.__isset.g);
}

BOOST_AUTO_TEST_CASE(BoundaryCase) {

  // jsonSimpleT2 is to test whether the generated code throw exeption
  // if the required field doesn't have value : field c's value is
  // missing
  string jsonSimpleT("{\"a\":true,\"d\":32,\"e\":64,\"b\":8,"
      "\"f\":0.99,\"g\":\"Hello\"}");
  mySimpleStruct thriftSimpleObj;
  try {
    thriftSimpleObj.readFromJson(jsonSimpleT.c_str());
    BOOST_CHECK(false);
  } catch (apache::thrift::TException &e) {
  }

  string jsonByteTW("{\"a\":128}");
  myByteStruct thriftByteObjW;
  try {
    thriftByteObjW.readFromJson(jsonByteTW.c_str());
    BOOST_CHECK(false);
  } catch (apache::thrift::TException &e) {
  }

  string jsonByteT("{\"a\":127}");
  myByteStruct thriftByteObj;
  thriftByteObj.readFromJson(jsonByteT.c_str());
  BOOST_CHECK_EQUAL(thriftByteObj.a, 0x7f);

  string jsonI16TW("{\"a\":32768}");
  myI16Struct thriftI16ObjW;
  try {
    thriftI16ObjW.readFromJson(jsonI16TW.c_str());
    BOOST_CHECK(false);
  } catch (apache::thrift::TException &e) {
  }

  string jsonI16T("{\"a\":32767}");
  myI16Struct thriftI16Obj;
  thriftI16Obj.readFromJson(jsonI16T.c_str());
  BOOST_CHECK_EQUAL(thriftI16Obj.a, 0x7fff);

  string jsonI32TW("{\"a\":2147483648}");
  myI32Struct thriftI32ObjW;
  try {
    thriftI32ObjW.readFromJson(jsonI32TW.c_str());
    BOOST_CHECK(false);
  } catch (apache::thrift::TException &e) {
  }

  string jsonI32T("{\"a\":2147483647}");
  myI32Struct thriftI32Obj;
  thriftI32Obj.readFromJson(jsonI32T.c_str());
  BOOST_CHECK_EQUAL(thriftI32Obj.a, 0x7fffffff);

  string jsonBoolTW("{\"a\":2}");
  myBoolStruct thriftBoolObjW;
  try {
    thriftBoolObjW.readFromJson(jsonBoolTW.c_str());
    BOOST_CHECK(false);
  } catch (std::exception &e) {
  }

}

BOOST_AUTO_TEST_CASE(ComplexTypeMissingRequiredFieldInMember) {

  string jsonT("{\"a\":true,\"c\":16,\"d\":32,\"e\":64,\"b\":8,"
      "\"f\":0.99,\"g\":\"Hello\"}");
  string jsonComplexT
    ("{\"a\":" + jsonT +  ", \"b\":[3,2,1], \"c\":{\"key1\":" +
    jsonT + ",\"key2\":{\"d\":320, \"f\":0.001}}}");

  myComplexStruct thriftComplexObj;
  try {
    thriftComplexObj.readFromJson(jsonComplexT.c_str());
    BOOST_CHECK(false);
  } catch (apache::thrift::TException &e) {
  }
}

BOOST_AUTO_TEST_CASE(ComplexTypeTest) {

  string jsonT("{\"a\":true,\"c\":16,\"d\":32,\"e\":64,\"b\":8,"
      "\"f\":0.99,\"g\":\"Hello\"}");
  string jsonComplexT
    ("{\"a\":" + jsonT +  ", \"b\":[3,2,1], \"c\":{\"key1\":" +
    jsonT + ",\"key2\":{\"c\":20, \"d\":320, \"f\":0.001}}}");

  myComplexStruct thriftComplexObj;
  thriftComplexObj.readFromJson(jsonComplexT.c_str());

  BOOST_CHECK_EQUAL(thriftComplexObj.a.b, 8);
  BOOST_CHECK_EQUAL(thriftComplexObj.a.c, 16);
  BOOST_CHECK_EQUAL(thriftComplexObj.a.d, 32);
  BOOST_CHECK_EQUAL(thriftComplexObj.a.e, 64);
  BOOST_CHECK_EQUAL(thriftComplexObj.a.f, 0.99);
  BOOST_CHECK_EQUAL(thriftComplexObj.a.g, "Hello");

  BOOST_CHECK_EQUAL(thriftComplexObj.b[0], 3);
  BOOST_CHECK_EQUAL(thriftComplexObj.b[1], 2);
  BOOST_CHECK_EQUAL(thriftComplexObj.b[2], 1);

  BOOST_CHECK_EQUAL(thriftComplexObj.c["key1"].b, 8);
  BOOST_CHECK_EQUAL(thriftComplexObj.c["key1"].c, 16);
  BOOST_CHECK_EQUAL(thriftComplexObj.c["key1"].d, 32);
  BOOST_CHECK_EQUAL(thriftComplexObj.c["key1"].e, 64);
  BOOST_CHECK_EQUAL(thriftComplexObj.c["key1"].f, 0.99);
  BOOST_CHECK_EQUAL(thriftComplexObj.c["key1"].g, "Hello");
  BOOST_CHECK_EQUAL(thriftComplexObj.c["key2"].c, 20);
  BOOST_CHECK_EQUAL(thriftComplexObj.c["key2"].d, 320);
  BOOST_CHECK_EQUAL(thriftComplexObj.c["key2"].f, 0.001);
}

BOOST_AUTO_TEST_CASE(SetTypeTest) {

  string jsonT("{\"a\":[1,2,3]}");
  mySetStruct thriftSetObj;
  thriftSetObj.readFromJson(jsonT.c_str());
  BOOST_CHECK(thriftSetObj.__isset.a);
  BOOST_CHECK_EQUAL(thriftSetObj.a.size(), 3);
  BOOST_CHECK(thriftSetObj.a.find(2) != thriftSetObj.a.end());
  BOOST_CHECK(thriftSetObj.a.find(5) ==  thriftSetObj.a.end());
}

BOOST_AUTO_TEST_CASE(MixedStructTest) {
  string jsonT("{\"a\":[1],\"b\":[{\"a\":1}],\"c\":{\"hello\":1},"
      "\"d\":{\"hello\":{\"a\":1}},\"e\":[1]}");
  myMixedStruct thriftMixedObj;
  thriftMixedObj.readFromJson(jsonT.c_str());
  BOOST_CHECK_EQUAL(thriftMixedObj.a[0], 1);
  BOOST_CHECK_EQUAL(thriftMixedObj.b[0].a, 1);
  BOOST_CHECK_EQUAL(thriftMixedObj.c["hello"], 1);
  BOOST_CHECK_EQUAL(thriftMixedObj.d["hello"].a, 1);
  BOOST_CHECK(thriftMixedObj.e.find(1) != thriftMixedObj.e.end());
}

BOOST_AUTO_TEST_CASE(MapTypeTest) {
  string stringJson("\"stringMap\": {\"a\":\"A\", \"b\":\"B\"}");
  string boolJson("\"boolMap\": {\"true\":\"True\", \"false\":\"False\"}");
  string byteJson("\"byteMap\": {\"1\":\"one\", \"2\":\"two\"}");
  string doubleJson("\"doubleMap\": {\"0.1\":\"0.one\", \"0.2\":\"0.two\"}");
  string enumJson("\"enumMap\": {\"1\":\"male\", \"2\":\"female\"}");
  string json = "{" + stringJson + ", " + boolJson + ", " + byteJson + ", " +
    doubleJson + ", " + enumJson + "}";
  myMapStruct mapStruct;
  mapStruct.readFromJson(json.c_str());
  BOOST_CHECK_EQUAL(mapStruct.stringMap.size(), 2);
  BOOST_CHECK_EQUAL(mapStruct.stringMap["a"], "A");
  BOOST_CHECK_EQUAL(mapStruct.stringMap["b"], "B");
  BOOST_CHECK_EQUAL(mapStruct.boolMap.size(), 2);
  BOOST_CHECK_EQUAL(mapStruct.boolMap[true], "True");
  BOOST_CHECK_EQUAL(mapStruct.boolMap[false], "False");
  BOOST_CHECK_EQUAL(mapStruct.byteMap.size(), 2);
  BOOST_CHECK_EQUAL(mapStruct.byteMap[1], "one");
  BOOST_CHECK_EQUAL(mapStruct.byteMap[2], "two");
  BOOST_CHECK_EQUAL(mapStruct.doubleMap.size(), 2);
  BOOST_CHECK_EQUAL(mapStruct.doubleMap[0.1], "0.one");
  BOOST_CHECK_EQUAL(mapStruct.doubleMap[0.2], "0.two");
  BOOST_CHECK_EQUAL(mapStruct.enumMap.size(), 2);
  BOOST_CHECK_EQUAL(mapStruct.enumMap[MALE], "male");
  BOOST_CHECK_EQUAL(mapStruct.enumMap[FEMALE], "female");
}

BOOST_AUTO_TEST_CASE(EmptyStringTest) {
  string jsonT("{\"a\":\"\"}");
  myStringStruct thriftStringObj;
  thriftStringObj.readFromJson(jsonT.c_str());
  BOOST_CHECK_EQUAL(thriftStringObj.a, "");
}

BOOST_AUTO_TEST_CASE(BinaryTypeTest) {
  string jsonT("{\"a\":\"abc\"}");
  myBinaryStruct thriftBinaryObj;
  thriftBinaryObj.readFromJson(jsonT.c_str());
  BOOST_CHECK_EQUAL(thriftBinaryObj.a, "abc");
}

boost::unit_test::test_suite* init_unit_test_suite(int argc, char* argv[]) {
  boost::unit_test::framework::master_test_suite().p_name.value =
    "json_unittest";

  if (argc != 1) {
    fprintf(stderr, "unexpected arguments: %s\n", argv[1]);
    exit(1);
  }

  return nullptr;
}
