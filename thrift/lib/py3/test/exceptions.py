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

from testing.types import Color, HardError, SimpleError, UnfriendlyError, UnusedError
from thrift.py3 import Error

from .exception_helper import simulate_HardError, simulate_UnusedError


class ExceptionTests(unittest.TestCase):
    def test_hashability(self) -> None:
        hash(UnusedError())

    def test_creation_optional_from_c(self) -> None:
        msg = "this is what happened"
        x = simulate_UnusedError(msg)
        self.assertIsInstance(x, UnusedError)
        self.assertIn(msg, str(x))
        self.assertIn(msg, x.args)
        self.assertEqual(msg, x.message)
        self.assertEqual(UnusedError(*x.args), x)  # type: ignore

    def test_exception_message_annotation(self) -> None:
        x = UnusedError(message="something broke")
        self.assertEqual(x.message, str(x))
        y = HardError("WAT!", 22)  # type: ignore
        self.assertEqual(y.errortext, str(y))
        z = UnfriendlyError("WAT!", 22)  # type: ignore
        self.assertNotEqual(z.errortext, str(z))
        self.assertNotEqual(str(y), str(z))

    def test_creation_optional_from_python(self) -> None:
        msg = "something broke"
        UnusedError()
        x = UnusedError(msg)  # type: ignore
        y = UnusedError(message=msg)
        self.assertEqual(x, y)
        self.assertEqual(x.args, y.args)
        self.assertEqual(x.message, y.message)
        self.assertEqual(str(x), str(x))

    def test_creation_required_from_c(self) -> None:
        msg = "ack!"
        code = 22
        x = simulate_HardError(msg, code)
        self.assertIsInstance(x, HardError)
        self.assertIn(msg, str(x))
        self.assertIn(msg, x.args)
        self.assertIn(code, x.args)
        self.assertEqual(code, x.code)
        self.assertEqual(msg, x.errortext)
        self.assertEqual(x, HardError(*x.args))  # type: ignore

    def test_creation_required_from_python(self) -> None:
        msg = "ack!"
        code = 22
        w = HardError(msg)  # type: ignore
        self.assertEqual(w.code, 0)
        x = HardError(msg, code)  # type: ignore
        y = HardError(msg, code=code)  # type: ignore
        self.assertEqual(x, y)
        self.assertEqual(x.args, y.args)
        self.assertEqual(x.errortext, y.errortext)
        self.assertEqual(x.code, y.code)
        self.assertEqual(str(x), str(y))

    def test_raise(self) -> None:
        with self.assertRaises(SimpleError):
            raise SimpleError()

        with self.assertRaises(Error):
            raise SimpleError(Color.red)

        with self.assertRaises(Exception):
            raise SimpleError()

        with self.assertRaises(BaseException):
            raise SimpleError()

        x = SimpleError(Color.blue)

        self.assertIsInstance(x, BaseException)
        self.assertIsInstance(x, Exception)
        self.assertIsInstance(x, Error)
        self.assertIsInstance(x, SimpleError)
