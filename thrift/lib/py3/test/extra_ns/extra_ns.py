#!/usr/bin/env python3
import unittest

from b.types import TypedefB
from c.types import StructC


class ExtraNamespaceTests(unittest.TestCase):

    def test_ns_via_typedef(self) -> None:
        sc = StructC(ue=TypedefB(val=33))
        self.assertEqual(33, sc.ue.val)
