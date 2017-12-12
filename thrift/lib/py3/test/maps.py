#!/usr/bin/env python3
import unittest

from testing.types import StrIntMap, StrStrIntListMapMap, StrI32ListMap


class MapTests(unittest.TestCase):
    def test_None(self):
        with self.assertRaises(TypeError):
            StrIntMap({None: 5})
        with self.assertRaises(TypeError):
            StrIntMap({'foo': None})
        with self.assertRaises(TypeError):
            StrStrIntListMapMap({'bar': {'foo': [None, None]}})
        with self.assertRaises(TypeError):
            StrStrIntListMapMap({'bar': {'foo': None}})

    def test_empty(self):
        StrIntMap()
        StrIntMap({})
        StrStrIntListMapMap({})
        StrStrIntListMapMap({'foo': {}})
        StrStrIntListMapMap({'foo': {'bar': []}})

    def test_mixed_construction(self):
        s = StrI32ListMap({'bar': [0, 1]})
        x = StrStrIntListMapMap({'foo': s})
        px = dict(x)
        px['baz'] = {'wat': [4]}
        px['foo'] = dict(px['foo'])
        px['foo']['bar'] = px['foo']['bar'] + [5, 7, 8]
        self.assertEquals(s['bar'], [0, 1])
        # Now turn this crazy mixed structure back to Cython
        cx = StrStrIntListMapMap(px)
        px['bar'] = {'lol': 'TypeError'}
        with self.assertRaises(TypeError):
            StrStrIntListMapMap(px)
        self.assertNotIn('bar', cx)
