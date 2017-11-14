#!/usr/bin/env python3
import unittest
import pickle
from thrift.py3 import serialize, deserialize, Protocol

from testing.types import easy


class SerializerTests(unittest.TestCase):
    def test_thrift_serialization_round_robin(self):
        control = easy(val=5, val_list=[1, 2, 3, 4])

        for proto in Protocol:
            encoded = serialize(control, protocol=proto)
            self.assertIsInstance(encoded, bytes)
            decoded = deserialize(easy, encoded, protocol=proto)
            self.assertIsInstance(decoded, easy)
            self.assertEqual(control, decoded)

    def test_pickle_round_robin(self):
        control = easy(val=0, val_list=[5, 6, 7])
        encoded = pickle.dumps(control, protocol=pickle.HIGHEST_PROTOCOL)
        decoded = pickle.loads(encoded)
        self.assertIsInstance(decoded, easy)
        self.assertEqual(control, decoded)
