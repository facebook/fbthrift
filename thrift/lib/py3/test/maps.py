#!/usr/bin/env python3
import unittest

from testing.types import StrIntMap, StrStrIntListMapMap, StrI32ListMap
from typing import List, Dict


class MapTests(unittest.TestCase):

    def test_None(self) -> None:
        with self.assertRaises(TypeError):
            StrIntMap({None: 5})  # type: ignore
        with self.assertRaises(TypeError):
            StrIntMap({'foo': None})  # type: ignore
        with self.assertRaises(TypeError):
            StrStrIntListMapMap({'bar': {'foo': [None, None]}})  # type: ignore
        with self.assertRaises(TypeError):
            StrStrIntListMapMap({'bar': {'foo': None}})  # type: ignore

    def test_empty(self) -> None:
        StrIntMap()
        StrIntMap({})
        StrStrIntListMapMap({})
        StrStrIntListMapMap({'foo': {}})
        StrStrIntListMapMap({'foo': {'bar': []}})

    def test_mixed_construction(self) -> None:
        s = StrI32ListMap({'bar': [0, 1]})
        x = StrStrIntListMapMap({'foo': s})
        px: Dict[str, Dict[str, List[int]]] = {}
        px['foo'] = x['foo']  # type: ignore
        px['baz'] = {'wat': [4]}
        px['foo'] = dict(px['foo'])
        px['foo']['bar'] = px['foo']['bar'] + [5, 7, 8]
        self.assertEquals(s['bar'], [0, 1])
        # Now turn this crazy mixed structure back to Cython
        cx = StrStrIntListMapMap(px)
        px['bar'] = {'lol': 'TypeError'}  # type: ignore
        with self.assertRaises(TypeError):
            StrStrIntListMapMap(px)
        self.assertNotIn('bar', cx)

    def test_hashability(self) -> None:
        hash(StrI32ListMap())
        x = StrStrIntListMapMap({'foo': StrI32ListMap()})
        hash(x['foo'])
