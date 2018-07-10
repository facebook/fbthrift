from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import unittest

from fuzz import TestService
from thrift.util.fuzzer import Service


class TestServiceWrapper(unittest.TestCase):

    def testServiceLoadMethods(self):
        service = Service(None, None, TestService)
        service.load_methods()
        service_methods = service.get_methods()

        self.assertEqual(len(service_methods), 3)
        self.assertIsNotNone(service_methods["lookup"])
        self.assertIsNotNone(service_methods["nested"])
        self.assertIsNotNone(service_methods["listStruct"])

        method_args = service_methods["lookup"]["args_class"].thrift_spec

        # args_class has an additional None value in the beginning before the args
        self.assertEqual(len(method_args), 3)

        self.assertEqual(method_args[1][2], "root")
        self.assertEqual(method_args[2][2], "key")

        self.assertEqual(len(service_methods["lookup"]["thrift_exceptions"]), 2)

    def testServiceFilterMethods(self):
        service = Service(None, None, TestService)
        service.load_methods()
        service_methods = service.get_methods(["lookup", "nested"])

        self.assertEqual(len(service_methods), 2)
        self.assertIsNotNone(service_methods["lookup"])
        self.assertIsNotNone(service_methods["nested"])

    def testServiceExcludeIfaces(self):
        service = Service(None, None, TestService)
        service.load_methods(exclude_ifaces=[TestService.Iface])
        service_methods = service.get_methods()

        self.assertEqual(len(service_methods), 0)
