#!/usr/bin/env python3
import unittest

from optional.types import NoDefaults, WithDefaults


class OptionalTests(unittest.TestCase):
    # Note that this test assumes the thrift is compiled with
    # thrift_cpp2_options = "optionals", which changes behavior somewhat
    def test_get_set(self) -> None:
        n = NoDefaults(req_field=1)
        self.assertEquals(n.req_field, 1)
        self.assertEquals(n.unflagged_field, 0)
        self.assertEquals(n.opt_field, None)

        n2 = NoDefaults(req_field=1, unflagged_field=2, opt_field=3)
        self.assertEquals(n2.req_field, 1)
        self.assertEquals(n2.unflagged_field, 2)
        self.assertEquals(n2.opt_field, 3)

        w = WithDefaults(req_field=1)
        self.assertEquals(w.req_field, 1)
        self.assertEquals(w.unflagged_field, 20)
        self.assertEquals(w.opt_field, None)

        w2 = WithDefaults(req_field=1, unflagged_field=2, opt_field=3)
        self.assertEquals(w2.req_field, 1)
        self.assertEquals(w2.unflagged_field, 2)
        self.assertEquals(w2.opt_field, 3)
