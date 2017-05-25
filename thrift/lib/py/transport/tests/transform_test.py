#!/usr/bin/env python3
import unittest

from thrift.transport.TTransport import TMemoryBuffer
from thrift.transport.THeaderTransport import (
    CLIENT_TYPE, THeaderTransport, MAX_FRAME_SIZE, TRANSFORM
)


class Compression(unittest.TestCase):
    def test_zstd(self):
        original = b'ABCD'
        mb = TMemoryBuffer()
        trans = THeaderTransport(mb, client_type=CLIENT_TYPE.HEADER)
        trans.add_transform(TRANSFORM.ZSTD)
        trans.write(original)
        trans.flush()
        frame = mb.getvalue()
        # Cleanup the memory buffer
        mb.close()
        del mb

        # Read it back
        mb = TMemoryBuffer(frame)
        trans = THeaderTransport(mb, client_type=CLIENT_TYPE.HEADER)
        result = trans.read(MAX_FRAME_SIZE)
        mb.close()
        del mb
        self.assertEqual(result, original, 'zstd decompressed different from original')
