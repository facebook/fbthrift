# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import unittest

from thrift.py3 import serialize, deserialize, Protocol
from thrift.test.lazy_deserialization.simple.types import (
    Foo,
    LazyFoo,
    LazyCppRef,
    OptionalFoo,
    OptionalLazyFoo,
)


def gen(Struct):
    return Struct(
        field1=[1] * 10,
        field2=[2] * 20,
        field3=[3] * 30,
        field4=[4] * 40,
    )


class UnitTest(unittest.TestCase):
    def testFooToLazyFoo(self):
        foo = gen(Foo)
        s = serialize(foo, Protocol.COMPACT)
        lazyFoo = deserialize(LazyFoo, s, Protocol.COMPACT)
        self.assertEqual(foo.field1, lazyFoo.field1)
        self.assertEqual(foo.field2, lazyFoo.field2)
        self.assertEqual(foo.field3, lazyFoo.field3)
        self.assertEqual(foo.field4, lazyFoo.field4)

    def testLazyFooToFoo(self):
        lazyFoo = gen(LazyFoo)
        s = serialize(lazyFoo, Protocol.COMPACT)
        foo = deserialize(Foo, s, Protocol.COMPACT)
        self.assertEqual(foo.field1, lazyFoo.field1)
        self.assertEqual(foo.field2, lazyFoo.field2)
        self.assertEqual(foo.field3, lazyFoo.field3)
        self.assertEqual(foo.field4, lazyFoo.field4)

    def testLazyCppRefRoundTrip(self):
        foo = LazyCppRef(
            field1=[1] * 10,
            field2=[2] * 20,
            field3=[3] * 30,
        )
        s = serialize(foo, Protocol.COMPACT)
        bar = deserialize(LazyCppRef, s, Protocol.COMPACT)
        self.assertEqual(bar.field1, [1] * 10)
        self.assertEqual(bar.field2, [2] * 20)
        self.assertEqual(bar.field3, [3] * 30)

    def testEmptyLazyCppRefRoundTrip(self):
        foo = LazyCppRef()
        s = serialize(foo, Protocol.COMPACT)
        bar = deserialize(LazyCppRef, s, Protocol.COMPACT)
        self.assertIsNone(bar.field1)
        self.assertIsNone(bar.field2)
        self.assertIsNone(bar.field3)

    def testComparison(self):
        foo1 = gen(LazyFoo)
        s = serialize(foo1, Protocol.COMPACT)
        foo2 = deserialize(LazyFoo, s, Protocol.COMPACT)
        self.assertEqual(foo1, foo2)
        foo1 = foo1(field4=[])
        self.assertLess(foo1, foo2)
        foo2 = foo2(field4=[])
        self.assertEqual(foo1, foo2)

    def testOptional(self):
        s = serialize(gen(Foo), Protocol.COMPACT)
        foo = deserialize(OptionalFoo, s, Protocol.COMPACT)
        lazyFoo = deserialize(OptionalLazyFoo, s, Protocol.COMPACT)
        self.assertEqual(foo.field1, lazyFoo.field1)
        self.assertEqual(foo.field2, lazyFoo.field2)
        self.assertEqual(foo.field3, lazyFoo.field3)
        self.assertEqual(foo.field4, lazyFoo.field4)
