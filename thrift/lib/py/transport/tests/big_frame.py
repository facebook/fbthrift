#!/usr/bin/env python3
import unittest
import struct

from thrift.transport.TTransport import TMemoryBuffer
from thrift.transport.THeaderTransport import (
    CLIENT_TYPE, THeaderTransport, MAX_FRAME_SIZE, BIG_FRAME_MAGIC
)

MIN_HEADER_SIZE = 14


class BigFrame(unittest.TestCase):
    def test_round_robin(self):
        original = b'A' * MAX_FRAME_SIZE
        mb = TMemoryBuffer()
        trans = THeaderTransport(mb, client_type=CLIENT_TYPE.HEADER)
        trans.set_max_frame_size(MAX_FRAME_SIZE + MIN_HEADER_SIZE)
        trans.write(original)
        trans.flush()
        frame = mb.getvalue()
        # Cleanup the memory buffer
        mb.close()
        del mb
        # Partial Decode the frame and see if its correct size wise
        sz = struct.unpack('!I', frame[:4])[0]
        self.assertEqual(sz, BIG_FRAME_MAGIC)
        sz = struct.unpack('!Q', frame[4:12])[0]
        self.assertEqual(len(frame), sz + 12)

        # Read it back
        mb = TMemoryBuffer(frame)
        trans = THeaderTransport(mb, client_type=CLIENT_TYPE.HEADER)
        trans.set_max_frame_size(len(frame))
        trans.readFrame(0)
        result = trans.read(MAX_FRAME_SIZE)
        mb.close()
        del mb
        self.assertEqual(result, original, 'round-robin different from original')
