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

#include <thrift/test/gen-cpp/ReflectionTest_types.h>
#include <thrift/lib/cpp/Reflection.h>

#include <gtest/gtest.h>

using namespace apache::thrift::reflection;
using namespace thrift::test;

TEST(Reflection, Basic) {
  Schema schema;
  ReflectionTestStruct1::_reflection_register(schema);
  EXPECT_EQ(Type::TYPE_STRUCT, getType(ReflectionTestStruct1::_reflection_id));

  auto& s1 = schema.dataTypes.at(ReflectionTestStruct1::_reflection_id);
  EXPECT_EQ("struct ReflectionTest.ReflectionTestStruct1", s1.name);
  EXPECT_EQ(ReflectionTestStruct1::_reflection_id, schema.names.at(s1.name));
  EXPECT_TRUE(s1.__isset.fields);
  EXPECT_EQ(4, s1.fields.size());

  {
    auto& f = s1.fields.at(1);
    EXPECT_TRUE(f.isRequired);
    EXPECT_EQ(Type::TYPE_I32, f.type);
    EXPECT_EQ("a", f.name);
    EXPECT_EQ(1, f.order);
    EXPECT_FALSE(f.__isset.annotations);
  }

  {
    auto& f = s1.fields.at(2);
    EXPECT_FALSE(f.isRequired);
    EXPECT_EQ(Type::TYPE_I32, f.type);
    EXPECT_EQ("b", f.name);
    EXPECT_EQ(2, f.order);
    EXPECT_FALSE(f.__isset.annotations);
  }

  {
    auto& f = s1.fields.at(3);
    // Fields that aren't declared "required" or "optional" are always
    // sent on the wire, so we'll consider them "required" for our purposes.
    EXPECT_TRUE(f.isRequired);
    EXPECT_EQ(Type::TYPE_I32, f.type);
    EXPECT_EQ("c", f.name);
    EXPECT_EQ(0, f.order);
    EXPECT_FALSE(f.__isset.annotations);
  }

  {
    auto& f = s1.fields.at(4);
    EXPECT_TRUE(f.isRequired);
    EXPECT_EQ(Type::TYPE_STRING, f.type);
    EXPECT_EQ("d", f.name);
    EXPECT_EQ(3, f.order);
    EXPECT_TRUE(f.__isset.annotations);
    EXPECT_EQ("hello", f.annotations.at("some.field.annotation"));
    EXPECT_EQ("1", f.annotations.at("some.other.annotation"));
    EXPECT_EQ("1", f.annotations.at("annotation.without.value"));
  }
}

TEST(Reflection, Complex) {
  Schema schema;
  ReflectionTestStruct2::_reflection_register(schema);
  EXPECT_EQ(Type::TYPE_STRUCT, getType(ReflectionTestStruct2::_reflection_id));

  auto& s2 = schema.dataTypes.at(ReflectionTestStruct2::_reflection_id);
  EXPECT_EQ("struct ReflectionTest.ReflectionTestStruct2", s2.name);
  EXPECT_EQ(ReflectionTestStruct2::_reflection_id, schema.names.at(s2.name));
  EXPECT_TRUE(s2.__isset.fields);
  EXPECT_EQ(4, s2.fields.size());

  {
    auto& f = s2.fields.at(1);
    EXPECT_TRUE(f.isRequired);
    EXPECT_EQ(0, f.order);
    EXPECT_EQ(Type::TYPE_MAP, getType(f.type));

    auto& m = schema.dataTypes.at(f.type);
    EXPECT_EQ("map<byte, struct ReflectionTest.ReflectionTestStruct1>", m.name);
    EXPECT_EQ(f.type, schema.names.at(m.name));
    EXPECT_TRUE(m.__isset.mapKeyType);
    EXPECT_EQ(Type::TYPE_BYTE, m.mapKeyType);
    EXPECT_TRUE(m.__isset.valueType);
    EXPECT_EQ(ReflectionTestStruct1::_reflection_id, m.valueType);
  }

  {
    auto& f = s2.fields.at(2);
    EXPECT_TRUE(f.isRequired);
    EXPECT_EQ(1, f.order);
    EXPECT_EQ(Type::TYPE_SET, getType(f.type));

    auto& m = schema.dataTypes.at(f.type);
    EXPECT_EQ("set<string>", m.name);
    EXPECT_EQ(f.type, schema.names.at("set<string>"));
    EXPECT_TRUE(m.__isset.valueType);
    EXPECT_EQ(Type::TYPE_STRING, m.valueType);
  }

  {
    auto& f = s2.fields.at(3);
    EXPECT_TRUE(f.isRequired);
    EXPECT_EQ(2, f.order);
    EXPECT_EQ(Type::TYPE_LIST, getType(f.type));

    auto& m = schema.dataTypes.at(f.type);
    EXPECT_EQ("list<i64>", m.name);
    EXPECT_EQ(f.type, schema.names.at(m.name));
    EXPECT_TRUE(m.__isset.valueType);
    EXPECT_EQ(Type::TYPE_I64, m.valueType);
  }

  {
    auto& f = s2.fields.at(4);
    EXPECT_TRUE(f.isRequired);
    EXPECT_EQ(Type::TYPE_ENUM, getType(f.type));

    auto& m = schema.dataTypes.at(f.type);
    EXPECT_EQ("enum ReflectionTest.ReflectionTestEnum", m.name);
    EXPECT_EQ(f.type, schema.names.at(m.name));
    EXPECT_TRUE(m.__isset.enumValues);
    EXPECT_EQ(2, m.enumValues.size());
    EXPECT_EQ(5, m.enumValues.at("FOO"));
    EXPECT_EQ(4, m.enumValues.at("BAR"));
  }
}
