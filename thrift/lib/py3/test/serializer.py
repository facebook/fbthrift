#!/usr/bin/env python3
import unittest
import pickle
from thrift.py3 import serialize, deserialize, Protocol, Struct, Error

from testing.types import easy, hard, Integers


class SerializerTests(unittest.TestCase):

    def test_bad_deserialize(self) -> None:
        with self.assertRaises(Error):
            deserialize(easy, b'', protocol=Protocol.JSON)
        with self.assertRaises(Error):
            deserialize(easy, b'\x05AAAAAAAA')
        with self.assertRaises(Error):
            deserialize(easy, b'\x02\xDE\xAD\xBE\xEF', protocol=Protocol.BINARY)

    def thrift_serialization_round_robin(self, control: Struct) -> None:
        control = easy(val=5, val_list=[1, 2, 3, 4])

        for proto in Protocol:
            encoded = serialize(control, protocol=proto)
            self.assertIsInstance(encoded, bytes)
            decoded = deserialize(type(control), encoded, protocol=proto)
            self.assertIsInstance(decoded, type(control))
            self.assertEqual(control, decoded)

    def pickle_round_robin(self, control: Struct) -> None:
        control = easy(val=0, val_list=[5, 6, 7])
        encoded = pickle.dumps(control, protocol=pickle.HIGHEST_PROTOCOL)
        decoded = pickle.loads(encoded)
        self.assertIsInstance(decoded, type(control))
        self.assertEqual(control, decoded)

    def test_serialize_easy_struct(self) -> None:
        control = easy(val=5, val_list=[1, 2, 3, 4])
        self.thrift_serialization_round_robin(control)

    def test_pickle_easy_struct(self) -> None:
        control = easy(val=0, val_list=[5, 6, 7])
        self.pickle_round_robin(control)

    def test_serialize_hard_struct(self) -> None:
        control = hard(
            val=0, val_list=[1, 2, 3, 4], name='foo', an_int=Integers(tiny=1)
        )
        self.thrift_serialization_round_robin(control)

    def test_pickle_hard_struct(self) -> None:
        control = hard(
            val=0, val_list=[1, 2, 3, 4], name='foo', an_int=Integers(tiny=1)
        )
        self.pickle_round_robin(control)

    def test_serialize_Integers_union(self) -> None:
        control = Integers(medium=1337)
        self.thrift_serialization_round_robin(control)

    def test_pickle_Integers_union(self) -> None:
        control = Integers(large=2 ** 32)
        self.pickle_round_robin(control)
