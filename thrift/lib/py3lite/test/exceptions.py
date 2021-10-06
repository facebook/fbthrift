#!/usr/bin/env python3
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

from testing.lite_types import (
    Color,
    HardError,
    SimpleError,
    UnfriendlyError,
    UnusedError,
    ValueOrError,
)
from thrift.py3lite.exceptions import Error
from thrift.py3lite.serializer import (
    deserialize,
    serialize_iobuf,
)


class ExceptionTests(unittest.TestCase):
    def test_hashability(self) -> None:
        hash(UnusedError())

    def test_exception_message_annotation(self) -> None:
        x = UnusedError(message="something broke")
        self.assertEqual(x.message, str(x))
        y = HardError(errortext="WAT!", code=22)
        self.assertEqual(y.errortext, str(y))
        z = UnfriendlyError(errortext="WAT!", code=22)
        self.assertNotEqual(z.errortext, str(z))
        self.assertNotEqual(str(y), str(z))

    def test_creation(self) -> None:
        msg = "something broke"
        UnusedError()
        # pyre-ignore[19]: for test
        x = UnusedError(msg)
        y = UnusedError(message=msg)
        self.assertEqual(x, y)
        self.assertEqual(x.args, y.args)
        self.assertEqual(x.message, y.message)
        self.assertEqual(str(x), str(x))

    def test_raise(self) -> None:
        with self.assertRaises(SimpleError):
            raise SimpleError()

        with self.assertRaises(Error):
            raise SimpleError(Color.red)

        with self.assertRaises(Exception):  # noqa: B017
            raise SimpleError()

        with self.assertRaises(BaseException):
            raise SimpleError()

        x = SimpleError(Color.blue)

        self.assertIsInstance(x, BaseException)
        self.assertIsInstance(x, Exception)
        self.assertIsInstance(x, Error)
        self.assertIsInstance(x, SimpleError)

    def test_str(self) -> None:
        x = UnusedError()
        self.assertEqual(str(x), "")
        x2 = UnusedError(message="hello")
        self.assertEqual(str(x2), "hello")
        y = SimpleError()
        self.assertEqual(str(y), "Color.red")
        y2 = SimpleError(color=Color.red)
        self.assertEqual(str(y2), "Color.red")

    def test_serialize_deserialize(self) -> None:
        err = HardError(errortext="err", code=2)
        x = ValueOrError(error=err)
        serialized = serialize_iobuf(x)
        y = deserialize(ValueOrError, serialized)
        self.assertIsNot(x, y)
        self.assertEqual(x, y)
