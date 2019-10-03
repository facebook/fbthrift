#!/usr/bin/env python3
import unittest

from folly.iobuf import IOBuf
from iobuf.types import Moo


class IOBufTests(unittest.TestCase):
    def test_get_set_struct_field(self) -> None:
        m = Moo(val=3, ptr=IOBuf(b"abcdef"), buf=IOBuf(b"xyzzy"), opt_ptr=IOBuf(b"pqr"))
        m2 = Moo(
            val=3, ptr=IOBuf(b"abcdef"), buf=IOBuf(b"xyzzy"), opt_ptr=IOBuf(b"pqr")
        )
        self.assertEqual(m, m2)
        assert m.ptr is not None
        assert m2.ptr is not None
        assert m.opt_ptr is not None
        assert m2.opt_ptr is not None

        self.assertEqual(bytes(m.ptr), bytes(m2.ptr))
        self.assertEqual(b"abcdef", bytes(m.ptr))

        self.assertEqual(bytes(m.buf), bytes(m2.buf))
        self.assertEqual(b"xyzzy", bytes(m.buf))

        self.assertEqual(bytes(m.opt_ptr), bytes(m2.opt_ptr))
        self.assertEqual(b"pqr", bytes(m.opt_ptr))
