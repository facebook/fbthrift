#!/usr/bin/env python3
import asyncio
import unittest
import pickle

from typing import Mapping, Sequence, AbstractSet, Union, Any
from thrift.py3 import serialize, deserialize, Protocol, Struct, Error
from thrift.py3.serializer import serialize_iobuf

from testing.types import easy, hard, Integers, I32List, StrStrMap, SetI32, Digits


class SerializerTests(unittest.TestCase):
    def test_sanity(self) -> None:
        with self.assertRaises(TypeError):
            serialize(1, Protocol.COMPACT)  # type: ignore

        with self.assertRaises(TypeError):
            serialize(easy(), None)  # type: ignore

        with self.assertRaises(TypeError):
            deserialize(Protocol, b'')  # type: ignore

        with self.assertRaises(TypeError):
            deserialize(easy, Protocol)  # type: ignore

    def test_from_thread_pool(self) -> None:
        control = easy(val=5, val_list=[1, 2, 3, 4])
        loop = asyncio.get_event_loop()
        coro = loop.run_in_executor(None, serialize, control)
        encoded = loop.run_until_complete(coro)
        coro = loop.run_in_executor(None, deserialize, type(control), encoded)
        decoded = loop.run_until_complete(coro)
        self.assertEqual(control, decoded)

    def test_serialize_iobuf(self) -> None:
        control = easy(val=5, val_list=[1, 2, 3, 4, 5])
        iobuf = serialize_iobuf(control)
        decoded = deserialize(type(control), iobuf)
        self.assertEqual(control, decoded)

    def test_bad_deserialize(self) -> None:
        with self.assertRaises(Error):
            deserialize(easy, b"", protocol=Protocol.JSON)
        with self.assertRaises(Error):
            deserialize(easy, b"\x05AAAAAAAA")
        with self.assertRaises(Error):
            deserialize(easy, b"\x02\xDE\xAD\xBE\xEF", protocol=Protocol.BINARY)

    def thrift_serialization_round_robin(
        self, control: Struct, fixtures: Mapping[Protocol, bytes]
    ) -> None:
        for proto in Protocol:
            encoded = serialize(control, protocol=proto)
            self.assertIsInstance(encoded, bytes)
            decoded = deserialize(type(control), encoded, protocol=proto)
            self.assertIsInstance(decoded, type(control))
            self.assertEqual(control, decoded)
            self.assertEqual((proto, encoded), (proto, fixtures.get(proto)))

    def pickle_round_robin(
        self, control: Union[Struct, Mapping[Any, Any], Sequence[Any], AbstractSet[Any]]
    ) -> None:
        encoded = pickle.dumps(control, protocol=pickle.HIGHEST_PROTOCOL)
        decoded = pickle.loads(encoded)
        self.assertIsInstance(decoded, type(control))
        self.assertEqual(control, decoded)

    def test_serialize_easy_struct(self) -> None:
        control = easy(val=5, val_list=[1, 2, 3, 4])
        fixtures: Mapping[Protocol, bytes] = {
            Protocol.COMPACT: b"\x15\n\x19E\x02\x04\x06\x08,\x00\x00",
            Protocol.BINARY: b"\x08\x00\x01\x00\x00\x00\x05\x0f\x00\x02\x08\x00\x00\x00"
            b"\x04\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00"
            b"\x00\x00\x04\x0c\x00\x04\x00\x00",
            Protocol.JSON: b'{"val":5,"val_list":[1,2,3,4],"an_int":{}}',
            Protocol.COMPACT_JSON: b'{"1":{"i32":5},"2":{"lst":["i32",4,1,2,3,4]},"4"'
            b':{"rec":{}}}',
        }
        self.thrift_serialization_round_robin(control, fixtures)

    def test_pickle_easy_struct(self) -> None:
        control = easy(val=0, val_list=[5, 6, 7])
        self.pickle_round_robin(control)

    def test_serialize_hard_struct(self) -> None:
        control = hard(
            val=0, val_list=[1, 2, 3, 4], name="foo", an_int=Integers(tiny=1)
        )
        fixtures: Mapping[Protocol, bytes] = {
            Protocol.COMPACT: b"\x15\x00\x19E\x02\x04\x06\x08\x18\x03foo\x1c\x13\x01"
            b"\x00\x18\x0csome default\x00",
            Protocol.BINARY: b"\x08\x00\x01\x00\x00\x00\x00\x0f\x00\x02\x08\x00\x00\x00"
            b"\x04\x00\x00\x00\x01\x00\x00\x00\x02\x00\x00\x00\x03\x00"
            b"\x00\x00\x04\x0b\x00\x03\x00\x00\x00\x03foo\x0c\x00\x04"
            b"\x03\x00\x01\x01\x00\x0b\x00\x05\x00\x00\x00\x0csome def"
            b"ault\x00",
            Protocol.JSON: b'{"val":0,"val_list":[1,2,3,4],"name":"foo","an_int":{"tiny'
            b'":1},"other":"some default"}',
            Protocol.COMPACT_JSON: b'{"1":{"i32":0},"2":{"lst":["i32",4,1,2,3,4]},"3":'
            b'{"str":"foo"},"4":{"rec":{"1":{"i8":1}}},"5":{"str":"some default"}}',
        }
        self.thrift_serialization_round_robin(control, fixtures)

    def test_pickle_hard_struct(self) -> None:
        control = hard(
            val=0, val_list=[1, 2, 3, 4], name="foo", an_int=Integers(tiny=1)
        )
        self.pickle_round_robin(control)

    def test_serialize_Integers_union(self) -> None:
        control = Integers(medium=1337)
        fixtures: Mapping[Protocol, bytes] = {
            Protocol.COMPACT: b"5\xf2\x14\x00",
            Protocol.BINARY: b"\x08\x00\x03\x00\x00\x059\x00",
            Protocol.JSON: b'{"medium":1337}',
            Protocol.COMPACT_JSON: b'{"3":{"i32":1337}}',
        }

        self.thrift_serialization_round_robin(control, fixtures)

    def test_pickle_Integers_union(self) -> None:
        control = Integers(large=2 ** 32)
        self.pickle_round_robin(control)

    def test_pickle_sequence(self) -> None:
        control = I32List([1, 2, 3, 4])
        self.pickle_round_robin(control)

        digits = Digits(data=[Integers(tiny=1), Integers(tiny=2), Integers(large=0)])
        assert digits.data
        self.pickle_round_robin(digits.data)

    def test_pickle_set(self) -> None:
        control = SetI32({1, 2, 3, 4})
        self.pickle_round_robin(control)

    def test_pickle_mapping(self) -> None:
        control = StrStrMap({"test": "test", "foo": "bar"})
        self.pickle_round_robin(control)
