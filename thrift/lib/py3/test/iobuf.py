#!/usr/bin/env python3
import unittest

from folly.iobuf import IOBuf
from iobuf.types import Moo
from .iobuf_helper import get_empty_chain, make_chain

class IOBufTests(unittest.TestCase):
    def test_empty_chain(self) -> None:
        ebuf = get_empty_chain()
        self.assertFalse(ebuf)
        self.assertTrue(ebuf.is_chained)
        self.assertEqual(len(ebuf), 0)
        self.assertEqual(ebuf.chain_size(), 0)
        self.assertEqual(ebuf.chain_count(), 8)
        self.assertEqual(b''.join(ebuf), b'')  # pyre-ignore: T32805991
        self.assertEqual(b'', bytes(ebuf))

    def test_chain(self) -> None:
        control = [b'facebook', b'thrift', b'python3', b'cython']
        chain = make_chain([IOBuf(x) for x in control])
        self.assertTrue(chain.is_chained)
        self.assertTrue(chain)
        self.assertEqual(bytes(chain), control[0])
        self.assertEqual(len(chain), len(control[0]))
        self.assertEqual(chain.chain_size(), sum(len(x) for x in control))
        self.assertEqual(chain.chain_count(), len(control))
        self.assertEqual(memoryview(chain.next), control[1])  # type: ignore
        self.assertEqual(b''.join(chain), b''.join(control))  # pyre-ignore: T32805991

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
        self.assertEqual(memoryview(xb), x)  # type: ignore
        self.assertEqual(bytes(xb), x)
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

    def test_cmp(self) -> None:
        x = IOBuf(b"abc")
        y = IOBuf(b"def")
        z = IOBuf(b"abc")
        self.assertEqual(x, z)
        self.assertNotEqual(x, y)
        self.assertLess(x, y)
        self.assertLessEqual(x, y)
        self.assertLessEqual(x, z)
        self.assertGreater(y, x)
        self.assertGreaterEqual(y, x)

    def test_get_set_struct_field(self) -> None:
        m = Moo(val=3, ptr=IOBuf(b'abcdef'), buf=IOBuf(b'xyzzy'), opt_ptr=IOBuf(b'pqr'))
        m2 = Moo(val=3, ptr=IOBuf(b'abcdef'), buf=IOBuf(b'xyzzy'), opt_ptr=IOBuf(b'pqr'))
        self.assertEqual(m, m2)
        assert m.ptr is not None
        assert m2.ptr is not None
        assert m.opt_ptr is not None
        assert m2.opt_ptr is not None

        self.assertEqual(bytes(m.ptr), bytes(m2.ptr))
        self.assertEqual(b'abcdef', bytes(m.ptr))

        self.assertEqual(bytes(m.buf), bytes(m2.buf))
        self.assertEqual(b'xyzzy', bytes(m.buf))

        self.assertEqual(bytes(m.opt_ptr), bytes(m2.opt_ptr))
        self.assertEqual(b'pqr', bytes(m.opt_ptr))
