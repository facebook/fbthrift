#!/usr/bin/env python3
import unittest

from testing.types import SetI32Lists, SetSetI32Lists


class SetTests(unittest.TestCase):
    def test_None(self):
        with self.assertRaises(TypeError):
            SetI32Lists({None})
        with self.assertRaises(TypeError):
            SetSetI32Lists({{None}})

    def test_empty(self):
        SetI32Lists(set())
        SetI32Lists({()})
        SetSetI32Lists(set())
        SetSetI32Lists({SetI32Lists()})
        SetSetI32Lists({SetI32Lists({()})})

    def test_mixed_construction(self):
        x = SetI32Lists({(0, 1, 2), (3, 4, 5)})
        z = SetSetI32Lists({x})
        pz = set(z)
        pz.add(x)
        nx = {(9, 10, 11)}
        pz.add(SetI32Lists(nx))
        cz = SetSetI32Lists(pz)
        self.assertIn(nx, cz)
        pz.add(5)
        with self.assertRaises(TypeError):
            SetSetI32Lists(pz)
