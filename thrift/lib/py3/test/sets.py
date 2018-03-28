#!/usr/bin/env python3
import unittest

from testing.types import SetI32Lists, SetSetI32Lists
from typing import AbstractSet, Sequence


class SetTests(unittest.TestCase):

    def test_None(self) -> None:
        with self.assertRaises(TypeError):
            SetI32Lists({None})  # type: ignore
        with self.assertRaises(TypeError):
            SetSetI32Lists({{None}})  # type: ignore

    def test_empty(self) -> None:
        SetI32Lists(set())
        SetI32Lists({()})
        SetSetI32Lists(set())
        SetSetI32Lists({SetI32Lists()})
        SetSetI32Lists({SetI32Lists({()})})

    def test_mixed_construction(self) -> None:
        x = SetI32Lists({(0, 1, 2), (3, 4, 5)})
        z = SetSetI32Lists({x})
        pz = set(z)
        pz.add(x)
        nx: AbstractSet[Sequence[int]] = {(9, 10, 11)}
        pz.add(SetI32Lists(nx))
        cz = SetSetI32Lists(pz)
        self.assertIn(nx, cz)
        pz.add(5)  # type: ignore
        with self.assertRaises(TypeError):
            SetSetI32Lists(pz)

    def test_hashability(self) -> None:
        hash(SetI32Lists())
        z = SetSetI32Lists({SetI32Lists({(1, 2, 3)})})
        hash(z)
        for sub_set in z:
            hash(sub_set)
