#!/usr/bin/env python3
import unittest

from thrift.py3 import serialize, deserialize
from testing.types import Kind, Perm, File


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

    def test_flag_enum(self) -> None:
        with self.assertRaises(TypeError):
            # flags are not ints
            File(name='/etc/motd', permissions=4)  # type: ignore
        x = File(name='/bin/sh', permissions=Perm.read | Perm.execute)
        self.assertIsInstance(x.permissions, Perm)
        self.assertEqual(x.permissions, Perm.read | Perm.execute)
        self.assertNotIsInstance(2, Perm, "Flags are not ints")
        x = File(name='')
        self.assertFalse(x.permissions)
        self.assertIsInstance(x.permissions, Perm)

    def test_flag_enum_serialization_roundtrip(self) -> None:
        x = File(
            name='/dev/null',
            type=Kind.CHAR,
            permissions=Perm.read | Perm.write
        )

        y = deserialize(File, serialize(x))
        self.assertEqual(x, y)
        self.assertEqual(x.permissions, Perm.read | Perm.write)
        self.assertIsInstance(x.permissions, Perm)
