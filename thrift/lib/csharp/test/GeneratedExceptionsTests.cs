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
using NUnit.Framework;
using FBThrift.Tests.Structs;

namespace FBThrift.Tests
{
    /// <summary>
    /// Tests for generated Thrift exceptions.
    /// </summary>
    [TestFixture]
    public class GeneratedExceptionsTests
    {
        // Basic exception behavior

        [Test]
        public void TestExceptionExtendsSystemException()
        {
            var ex = new MyException();
            Assert.IsNotNull(ex);
            Assert.IsInstanceOf<Exception>(ex);
        }

        [Test]
        public void TestExceptionFieldAccess()
        {
            var ex = new MyExceptionWithMessage();
            ex.MyStringField = "Test error";
            Assert.AreEqual("Test error", ex.MyStringField);
        }

        // @thrift.ExceptionMessage — Message property override

        [Test]
        public void TestExceptionMessageReturnsAnnotatedField()
        {
            var ex = new MyExceptionWithMessage();
            ex.MyStringField = "something went wrong";
            Assert.AreEqual("something went wrong", ex.Message);
        }

        [Test]
        public void TestExceptionMessagePolymorphic()
        {
            var ex = new MyExceptionWithMessage();
            ex.MyStringField = "polymorphic error";
            Exception baseEx = ex;
            Assert.AreEqual("polymorphic error", baseEx.Message);
        }

        [Test]
        public void TestExceptionMessageDefaultIsEmpty()
        {
            var ex = new MyExceptionWithMessage();
            Assert.AreEqual("", ex.Message);
        }

        [Test]
        public void TestExceptionMessageUpdatesWithField()
        {
            var ex = new MyExceptionWithMessage();
            ex.MyStringField = "first";
            Assert.AreEqual("first", ex.Message);
            ex.MyStringField = "second";
            Assert.AreEqual("second", ex.Message);
        }

        [Test]
        public void TestExceptionWithoutAnnotationUsesDefaultMessage()
        {
            var ex = new MyException();
            // No @thrift.ExceptionMessage annotation, so Message returns default .NET behavior
            Assert.That(ex.Message, Does.Contain("MyException"));
        }

        // Null-checking — exception fields behave same as struct fields

        [Test]
        public void TestExceptionSetNullStringFieldThrows()
        {
            var ex = new MyException();
            Assert.Throws<ArgumentNullException>(() => ex.MyStringField = null!);
        }
    }
}
