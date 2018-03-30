#!/usr/bin/env python3
import unittest

from enum import Enum
from thrift.py3 import serialize, deserialize, Protocol, BadEnum
from testing.types import Color, ColorGroups, Kind, Perm, File
from typing import cast, Type


class EnumTests(unittest.TestCase):

    def test_normal_enum(self) -> None:
        with self.assertRaises(TypeError):
            # Enums are not ints
            File(name='/etc/motd', type=8)  # type: ignore
        x = File(name='/etc', type=Kind.DIR)
        self.assertIsInstance(x.type, Kind)
        self.assertEqual(x.type, Kind.DIR)
        self.assertNotEqual(x.type, Kind.SOCK)
        self.assertNotEqual(x.type, 4, "Enums are not Ints")
        self.assertNotIsInstance(4, Kind, "Enums are not Ints")
        self.assertIn(x.type, Kind)
        self.assertEqual(int(x.type), 4)

    def test_enum_value_rename(self) -> None:
        """The value name is None but we auto rename it to None_"""
        x = deserialize(File, b'{"name":"blah", "type":0}', Protocol.JSON)
        self.assertEqual(x.type, Kind.None_)

    def test_flag_enum(self) -> None:
        with self.assertRaises(TypeError):
            # flags are not ints
            File(name='/etc/motd', permissions=4)  # type: ignore
        x = File(name='/bin/sh', permissions=Perm.read | Perm.execute)
        self.assertIsInstance(x.permissions, Perm)
        self.assertEqual(x.permissions, Perm.read | Perm.execute)
        self.assertNotIsInstance(2, Perm, "Flags are not ints")
        self.assertEqual(int(x.permissions), 5)
        x = File(name='')
        self.assertFalse(x.permissions)
        self.assertIsInstance(x.permissions, Perm)

    def test_flag_enum_serialization_roundtrip(self) -> None:
        x = File(name='/dev/null', type=Kind.CHAR, permissions=Perm.read | Perm.write)

        y = deserialize(File, serialize(x))
        self.assertEqual(x, y)
        self.assertEqual(x.permissions, Perm.read | Perm.write)
        self.assertIsInstance(x.permissions, Perm)

    def test_bad_enum_in_struct(self) -> None:
        x = deserialize(File, b'{"name": "something", "type": 64}', Protocol.JSON)
        self.assertBadEnum(cast(BadEnum, x.type), Kind, 64)

    def test_bad_enum_in_list_index(self) -> None:
        x = deserialize(ColorGroups, b'{"color_list": [1, 5, 0]}', Protocol.JSON)
        self.assertEqual(len(x.color_list), 3)
        self.assertEqual(x.color_list[0], Color.blue)
        self.assertBadEnum(cast(BadEnum, x.color_list[1]), Color, 5)
        self.assertEqual(x.color_list[2], Color.red)

    def test_bad_enum_in_list_iter(self) -> None:
        x = deserialize(ColorGroups, b'{"color_list": [1, 5, 0]}', Protocol.JSON)
        for idx, v in enumerate(x.color_list):
            if idx == 0:
                self.assertEqual(v, Color.blue)
            elif idx == 1:
                self.assertBadEnum(cast(BadEnum, v), Color, 5)
            else:
                self.assertEqual(v, Color.red)

    def test_bad_enum_in_list_reverse(self) -> None:
        x = deserialize(ColorGroups, b'{"color_list": [1, 5, 0]}', Protocol.JSON)
        for idx, v in enumerate(reversed(x.color_list)):
            if idx == 0:
                self.assertEqual(v, Color.red)
            elif idx == 1:
                self.assertBadEnum(cast(BadEnum, v), Color, 5)
            else:
                self.assertEqual(v, Color.blue)

    def test_bad_enum_in_set_iter(self) -> None:
        x = deserialize(ColorGroups, b'{"color_set": [1, 5, 0]}', Protocol.JSON)
        for v in x.color_set:
            if v not in (Color.blue, Color.red):
                self.assertBadEnum(cast(BadEnum, v), Color, 5)

    def test_bad_enum_in_map_lookup(self) -> None:
        json = b'{"color_map": {"1": 2, "0": 5, "6": 1, "7": 8}}'
        x = deserialize(ColorGroups, json, Protocol.JSON)
        val = x.color_map[Color.red]
        self.assertBadEnum(cast(BadEnum, val), Color, 5)

    def test_bad_enum_in_map_iter(self) -> None:
        json = b'{"color_map": {"1": 2, "0": 5, "6": 1, "7": 8}}'
        x = deserialize(ColorGroups, json, Protocol.JSON)
        s = set()
        for k in x.color_map:
            s.add(k)
        self.assertEqual(len(s), 4)
        s.discard(Color.red)
        s.discard(Color.blue)
        lst = sorted(s, key=lambda e: cast(BadEnum, e).value)
        self.assertBadEnum(cast(BadEnum, lst[0]), Color, 6)
        self.assertBadEnum(cast(BadEnum, lst[1]), Color, 7)

    def test_bad_enum_in_map_values(self) -> None:
        json = b'{"color_map": {"1": 2, "0": 5, "6": 1, "7": 8}}'
        x = deserialize(ColorGroups, json, Protocol.JSON)
        s = set()
        for k in x.color_map.values():
            s.add(k)
        self.assertEqual(len(s), 4)
        s.discard(Color.green)
        s.discard(Color.blue)
        lst = sorted(s, key=lambda e: cast(BadEnum, e).value)
        self.assertBadEnum(cast(BadEnum, lst[0]), Color, 5)
        self.assertBadEnum(cast(BadEnum, lst[1]), Color, 8)

    def test_bad_enum_in_map_items(self) -> None:
        json = b'{"color_map": {"1": 2, "0": 5, "6": 1, "7": 8}}'
        x = deserialize(ColorGroups, json, Protocol.JSON)
        for k, v in x.color_map.items():
            if k == Color.blue:
                self.assertEqual(v, Color.green)
            elif k == Color.red:
                self.assertBadEnum(cast(BadEnum, v), Color, 5)
            else:
                ck = cast(BadEnum, k)
                if ck.value == 6:
                    self.assertEqual(v, Color.blue)
                else:
                    self.assertBadEnum(cast(BadEnum, v), Color, 8)

    def assertBadEnum(self, e: BadEnum, cls: Type[Enum], val: int) -> None:
        self.assertIsInstance(e, BadEnum)
        self.assertEqual(e.value, val)
        self.assertEqual(e.enum, cls)
        self.assertEqual(int(e), val)
