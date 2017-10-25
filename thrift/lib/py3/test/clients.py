#!/usr/bin/env python3
import unittest

from testing.clients import TestingService


class ClientTests(unittest.TestCase):
    def test_client_keyword_arguments(self):
        # Create a broken client
        client = TestingService()
        # This should not raise an exception
        client.complex_action(first='foo', second='bar', third=9, fourth='baz')
        client.complex_action('foo', 'bar', 9, 'baz')
