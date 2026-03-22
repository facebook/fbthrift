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
using FBThrift.Tests.Structs;

namespace FBThrift.Tests
{
    /// <summary>
    /// Tests for generated Thrift structs.
    /// </summary>
    [TestFixture]
    public class GeneratedStructsTests
    {
        [Test]
        public void TestStructInstantiation()
        {
            var s = new MyStruct();
            Assert.IsNotNull(s);
        }

        [Test]
        public void TestStructFieldAccess()
        {
            var s = new MyStruct();
            s.MyIntField = 42;
            s.MyStringField = "hello";
            Assert.AreEqual(42, s.MyIntField);
            Assert.AreEqual("hello", s.MyStringField);
        }

        [Test]
        public void TestStructWithEnumField()
        {
            var s = new MyStruct();
            s.myEnum = MyEnum.MyValue2;
            Assert.AreEqual(MyEnum.MyValue2, s.myEnum);
        }

        [Test]
        public void TestStructClear()
        {
            var s = new MyStruct();
            s.MyIntField = 42;
            s.MyStringField = "hello";
            s.__fbthrift_clear();
            Assert.AreEqual(0, s.MyIntField);
            Assert.AreEqual(string.Empty, s.MyStringField);
        }

        [Test]
        public void TestStructIsEmpty()
        {
            var s = new MyStruct();
            Assert.IsTrue(s.__fbthrift_is_empty());
            s.MyIntField = 42;
            Assert.IsFalse(s.__fbthrift_is_empty());
        }

        [Test]
        public void TestStructEquality()
        {
            var s1 = new MyStruct();
            s1.MyIntField = 42;
            s1.MyStringField = "test";

            var s2 = new MyStruct();
            s2.MyIntField = 42;
            s2.MyStringField = "test";

            Assert.AreEqual(s1, s2);
        }

        [Test]
        public void TestStructInequality()
        {
            var s1 = new MyStruct();
            s1.MyIntField = 42;

            var s2 = new MyStruct();
            s2.MyIntField = 100;

            Assert.AreNotEqual(s1, s2);
        }

        [Test]
        public void TestStructToString()
        {
            var s = new MyStruct();
            s.MyIntField = 42;
            var str = s.ToString();
            Assert.That(str, Does.Contain("42"));
        }

        [Test]
        public void TestStructHashCode()
        {
            var s1 = new MyStruct();
            s1.MyIntField = 42;
            s1.MyStringField = "test";

            var s2 = new MyStruct();
            s2.MyIntField = 42;
            s2.MyStringField = "test";

            Assert.AreEqual(s1.GetHashCode(), s2.GetHashCode());
        }

        [Test]
        public void TestNestedStruct()
        {
            var data = new MyDataItem();

            var s = new MyStruct();
            s.MyDataField = data;

            Assert.IsNotNull(s.MyDataField);
        }

        [Test]
        public void TestContainersStruct()
        {
            var c = new Containers();
            c.I32List = new List<int> { 1, 2, 3 };
            c.StringSet = new HashSet<string> { "a", "b" };
            c.StringToI64Map = new Dictionary<string, long> { { "key", 42L } };

            Assert.AreEqual(3, c.I32List.Count);
            Assert.AreEqual(2, c.StringSet.Count);
            Assert.AreEqual(42L, c.StringToI64Map["key"]);
        }

        [Test]
        public void TestEnumValues()
        {
            Assert.AreEqual(0, (int)MyEnum.MyValue1);
            Assert.AreEqual(1, (int)MyEnum.MyValue2);
        }

        [Test]
        public void TestExceptionStruct()
        {
            var ex = new MyException();
            Assert.IsNotNull(ex);
            Assert.IsInstanceOf<Exception>(ex);
        }

        [Test]
        public void TestExceptionWithMessage()
        {
            var ex = new MyExceptionWithMessage();
            ex.MyStringField = "Test error";
            Assert.AreEqual("Test error", ex.MyStringField);
        }
    }
}
