/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

using System;
using System.Collections.Generic;
using NUnit.Framework;
using Test.Fixtures.CsharpStructs;

namespace FBThrift.Tests
{
    /// <summary>
    /// Tests for generated Thrift unions.
    /// Uses the csharp-structs fixture which includes both struct and union definitions.
    /// TODO: Delete this test and use the full cross-language fixtures once all codegen lands.
    /// </summary>
    [TestFixture]
    public class GeneratedUnionsTests
    {
        [Test]
        public void TestBasicUnionInstantiation()
        {
            var union = new SimpleUnion();
            Assert.IsNotNull(union);
        }

        [Test]
        public void TestUnionSetIntValue()
        {
            var union = new SimpleUnion();
            union.intValue = 42;
            Assert.AreEqual(42, union.intValue);
        }

        [Test]
        public void TestUnionSetStringValue()
        {
            var union = new SimpleUnion();
            union.stringValue = "hello";
            Assert.AreEqual("hello", union.stringValue);
        }

        [Test]
        public void TestUnionSetBoolValue()
        {
            var union = new SimpleUnion();
            union.boolValue = true;
            Assert.AreEqual(true, union.boolValue);
        }

        [Test]
        public void TestTypesUnionWithLong()
        {
            var union = new TypesUnion();
            union.longValue = 9223372036854775807L;
            Assert.AreEqual(9223372036854775807L, union.longValue);
        }

        [Test]
        public void TestTypesUnionWithDouble()
        {
            var union = new TypesUnion();
            union.doubleValue = 3.14159;
            Assert.AreEqual(3.14159, union.doubleValue, 0.00001);
        }

        [Test]
        public void TestTypesUnionWithList()
        {
            var union = new TypesUnion();
            union.listValue = new List<int> { 1, 2, 3, 4, 5 };
            Assert.AreEqual(5, union.listValue.Count);
            Assert.AreEqual(1, union.listValue[0]);
            Assert.AreEqual(5, union.listValue[4]);
        }

        [Test]
        public void TestTypesUnionWithMap()
        {
            var union = new TypesUnion();
            union.mapValue = new Dictionary<string, int>
            {
                { "one", 1 },
                { "two", 2 }
            };
            Assert.AreEqual(2, union.mapValue.Count);
            Assert.AreEqual(1, union.mapValue["one"]);
            Assert.AreEqual(2, union.mapValue["two"]);
        }

        [Test]
        public void TestStructUnionWithPoint()
        {
            var union = new StructUnion();
            var point = new Point();
            point.x = 10;
            point.y = 20;
            union.pointValue = point;

            Assert.AreEqual(10, union.pointValue.x);
            Assert.AreEqual(20, union.pointValue.y);
        }

        [Test]
        public void TestStructUnionWithPerson()
        {
            var union = new StructUnion();
            var person = new Person();
            person.name = "Alice";
            person.age = 30;
            union.personValue = person;

            Assert.AreEqual("Alice", union.personValue.name);
            Assert.AreEqual(30, union.personValue.age);
        }

        [Test]
        public void TestEnumUnionWithStatus()
        {
            var union = new EnumUnion();
            union.statusValue = Status.ACTIVE;
            Assert.AreEqual(Status.ACTIVE, union.statusValue);
        }

        [Test]
        public void TestEnumUnionWithInt()
        {
            var union = new EnumUnion();
            union.intValue = 42;
            Assert.AreEqual(42, union.intValue);
        }

        [Test]
        public void TestUnionEquality()
        {
            var union1 = new SimpleUnion();
            union1.intValue = 42;

            var union2 = new SimpleUnion();
            union2.intValue = 42;

            Assert.AreEqual(union1, union2);
        }

        [Test]
        public void TestUnionInequality()
        {
            var union1 = new SimpleUnion();
            union1.intValue = 42;

            var union2 = new SimpleUnion();
            union2.intValue = 100;

            Assert.AreNotEqual(union1, union2);
        }

        [Test]
        public void TestUnionToString()
        {
            var union = new SimpleUnion();
            union.intValue = 42;

            var str = union.ToString();
            Assert.That(str, Does.Contain("42"));
        }

        [Test]
        public void TestUnionHashCode()
        {
            var union1 = new SimpleUnion();
            union1.intValue = 42;

            var union2 = new SimpleUnion();
            union2.intValue = 42;

            Assert.AreEqual(union1.GetHashCode(), union2.GetHashCode());
        }
    }
}
