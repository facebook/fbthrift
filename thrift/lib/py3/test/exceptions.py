#!/usr/bin/env python3
import unittest

from testing.types import UnusedError, HardError
from .exception_helper import simulate_UnusedError, simulate_HardError


class ExceptionTests(unittest.TestCase):
    def test_creation_optional_from_c(self) -> None:
        msg = 'this is what happened'
        x = simulate_UnusedError(msg)
        self.assertIsInstance(x, UnusedError)
        self.assertIn(msg, str(x))
        self.assertIn(msg, x.args)
        self.assertEqual(msg, x.message)
        self.assertEqual(UnusedError(*x.args), x)  # type: ignore

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
        with self.assertRaises(TypeError):
            HardError(msg)  # type: ignore
        x = HardError(msg, code)  # type: ignore
        y = HardError(msg, code=code)  # type: ignore
        self.assertEqual(x, y)
        self.assertEqual(x.args, y.args)
        self.assertEqual(x.errortext, y.errortext)
        self.assertEqual(x.code, y.code)
        self.assertEqual(str(x), str(y))
