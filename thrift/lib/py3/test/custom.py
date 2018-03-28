#!/usr/bin/env python3
import unittest

from testing.types import customized


# Tests for customized alternate implementations of the data structures:


class CustomTests(unittest.TestCase):

    def test_list_template(self) -> None:
        c = customized(list_template=[1, 2, 3])
        res = []
        for v in c.list_template:
            res.append(v)
        self.assertEquals(res, [1, 2, 3])

    def test_set_template(self) -> None:
        c = customized(set_template={1, 2, 3})
        res = set()
        for v in c.set_template:
            res.add(v)
        self.assertEquals(res, {1, 2, 3})

    def test_map_template(self) -> None:
        c = customized(map_template={1: 2, 3: 6, 5: 10})
        res = {}
        for k, v in c.map_template.items():
            res[k] = v
        self.assertEquals(res, {1: 2, 3: 6, 5: 10})

    def test_list_type(self) -> None:
        c = customized(list_type=[1, 2, 3])
        res = []
        for v in c.list_type:
            res.append(v)
        self.assertEquals(res, [1, 2, 3])

    def test_set_type(self) -> None:
        c = customized(set_type={1, 2, 3})
        res = set()
        for v in c.set_type:
            res.add(v)
        self.assertEquals(res, {1, 2, 3})

    def test_map_type(self) -> None:
        c = customized(map_type={1: 2, 3: 6, 5: 10})
        res = {}
        for k, v in c.map_type.items():
            res[k] = v
        self.assertEquals(res, {1: 2, 3: 6, 5: 10})

    def test_string_type(self) -> None:
        c = customized(string_type="hello")
        self.assertEquals(c.string_type, "hello")
