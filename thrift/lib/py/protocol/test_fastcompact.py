from __future__ import (absolute_import, division,
                         print_function, unicode_literals)

import unittest
import thrift.protocol.fastproto as fastproto

#For test against py impl
import thrift.transport.TTransport as TTransport
import thrift.protocol.TProtocol as TProtocol
import thrift.protocol.TCompactProtocol as TCompactProtocol

import dummy.ttypes as dummy

class Compact(unittest.TestCase):
    def test_true(self):
        dummy_test = dummy.Dummy()
        dummy_test.dummy = {b'test': 42}
        otrans = TTransport.TMemoryBuffer()
        proto = TCompactProtocol.TCompactProtocol(otrans)
        fastproto.encode(proto, dummy_test,
                         (dummy.Dummy, dummy_test.thrift_spec))
        value = otrans.getvalue()

        trans = TTransport.TMemoryBuffer()
        proto = TCompactProtocol.TCompactProtocol(trans)
        dummy_test.write(proto)

        self.assertEqual(value, trans.getvalue())

        itrans = TTransport.TMemoryBuffer(value)
        proto = TCompactProtocol.TCompactProtocol(itrans)
        new_dummy = dummy.Dummy()
        fastproto.decode(proto, new_dummy,
                           (dummy.Dummy, dummy_test.thrift_spec))
        self.assertEqual(new_dummy.dummy[b'test'], 42)
