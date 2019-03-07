#!/usr/bin/env python3
import unittest
from collections import OrderedDict

from thrift.test.sample_structs.ttypes import DummyStruct, Struct
from thrift.util import struct_to_dict


class TestStructToDict(unittest.TestCase):
    def test_struct_to_dict(self) -> None:
        dummy_struct = DummyStruct(1)
        test_struct = Struct(
            1,
            [1, 2],
            {1, 3},
            {1: 2, 3: 4},
            {1: [1, 2]},
            {1: {1, 2}},
            [dummy_struct],
            [[1, 2]],
            [{1, 2}],
        )
        self.assertEqual(
            struct_to_dict(test_struct),
            OrderedDict(
                [
                    ("a", 1),
                    ("b", [1, 2]),
                    ("c", {1, 3}),
                    ("d", {1: 2, 3: 4}),
                    ("e", {1: [1, 2]}),
                    ("f", {1: {1, 2}}),
                    ("g", [OrderedDict([("a", 1)])]),
                    ("h", [[1, 2]]),
                    ("i", [{1, 2}]),
                ]
            ),
        )
