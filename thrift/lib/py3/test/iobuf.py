#!/usr/bin/env python3
import unittest

from folly.iobuf import IOBuf


class IOBufTests(unittest.TestCase):
    def test_hash(self) -> None:
        x = b"omg"
        y = b"wtf"
        xb = IOBuf(x)
        yb = IOBuf(y)
        hash(xb)
        self.assertNotEqual(hash(xb), hash(yb))
        self.assertEqual(hash(xb), hash(IOBuf(x)))

    def test_empty(self) -> None:
        x = b""
        xb = IOBuf(x)
        self.assertFalse(xb)
        self.assertEqual(len(xb), len(x))

    def test_iter(self) -> None:
        x = b"testtest"
        xb = IOBuf(x)
        self.assertEqual(b''.join(iter(xb)), x)

    def test_bytes(self) -> None:
        x = b"omgwtfbbq"
        xb = IOBuf(x)
        self.assertEqual(bytes(xb), x)
