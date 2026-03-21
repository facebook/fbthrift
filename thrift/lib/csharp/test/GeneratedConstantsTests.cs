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
using test.fixtures.basic;

namespace FBThrift.Tests
{
    /// <summary>
    /// Tests for generated Thrift constants using the cross-language basic fixture.
    /// </summary>
    [TestFixture]
    public class GeneratedConstantsTests
    {
        [Test]
        public void TestBoolConstant()
        {
            Assert.IsTrue(Constants.FLAG);
        }

        [Test]
        public void TestIntConstants()
        {
            Assert.AreEqual(-10, Constants.OFFSET);
            Assert.AreEqual(200, Constants.COUNT);
            Assert.AreEqual(0xFA12EE, Constants.MASK);
        }

        [Test]
        public void TestDoubleConstant()
        {
            Assert.AreEqual(2.718281828459, Constants.E, 1e-12);
        }

        [Test]
        public void TestStringConstant()
        {
            Assert.AreEqual("June 28, 2017", Constants.DATE);
        }

        [Test]
        public void TestListConstant()
        {
            Assert.AreEqual(4, Constants.AList.Count);
            Assert.AreEqual(2, Constants.AList[0]);
            Assert.AreEqual(3, Constants.AList[1]);
            Assert.AreEqual(5, Constants.AList[2]);
            Assert.AreEqual(7, Constants.AList[3]);
        }

        [Test]
        public void TestSetConstant()
        {
            Assert.AreEqual(3, Constants.ASet.Count);
            Assert.IsTrue(Constants.ASet.Contains("foo"));
            Assert.IsTrue(Constants.ASet.Contains("bar"));
            Assert.IsTrue(Constants.ASet.Contains("baz"));
        }

        [Test]
        public void TestMapConstant()
        {
            Assert.AreEqual(2, Constants.AMap.Count);
            Assert.IsTrue(Constants.AMap.ContainsKey("foo"));
            Assert.IsTrue(Constants.AMap.ContainsKey("bar"));
            Assert.AreEqual(4, Constants.AMap["foo"].Count);
            Assert.AreEqual(3, Constants.AMap["bar"].Count);
        }
    }
}
