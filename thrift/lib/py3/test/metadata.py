#!/usr/bin/env python3
# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import unittest

import testing.metadata
from apache.thrift.metadata.types import ThriftPrimitiveType


class MetadataTests(unittest.TestCase):
    def test_metadata_enums(self) -> None:
        meta = testing.metadata.getThriftModuleMetadata()
        enumName = "testing.Perm"
        self.assertIsNotNone(meta)
        self.assertEqual(meta.enums[enumName].name, enumName)
        self.assertEqual(meta.enums[enumName].elements[1], "execute")
        self.assertEqual(meta.enums[enumName].elements[4], "read")
        self.assertEqual(len(meta.enums[enumName].elements), 3)

    def test_metadata_structs(self) -> None:
        meta = testing.metadata.getThriftModuleMetadata()
        structName = "testing.hard"
        self.assertIsNotNone(meta)
        hardStruct = meta.structs[structName]
        self.assertFalse(hardStruct.is_union)
        self.assertEqual(hardStruct.name, structName)
        self.assertEqual(len(hardStruct.fields), 5)

        field = hardStruct.fields[2]
        self.assertEqual(field.name, "name")
        self.assertEqual(field.is_optional, False)
        self.assertEqual(field.type.value, ThriftPrimitiveType.THRIFT_STRING_TYPE)

        self.assertEqual(meta.structs["testing.EmptyUnion"].is_union, True)

    def test_metadata_exceptions(self) -> None:
        meta = testing.metadata.getThriftModuleMetadata()
        errorName = "testing.HardError"
        self.assertIsNotNone(meta)
        hardError = meta.exceptions[errorName]
        self.assertEqual(hardError.name, errorName)
        self.assertEqual(len(hardError.fields), 2)

        field = hardError.fields[1]
        self.assertEqual(field.name, "code")
        self.assertEqual(field.is_optional, False)
        self.assertEqual(field.type.value, ThriftPrimitiveType.THRIFT_I32_TYPE)

    def test_metadata_services(self) -> None:
        meta = testing.metadata.getThriftModuleMetadata()
        serviceName = "testing.TestingService"
        self.assertIsNotNone(meta)
        testingService = meta.services[serviceName]
        self.assertEqual(testingService.name, serviceName)
        self.assertEqual(testingService.parent, None)
        self.assertEqual(len(testingService.functions), 11)

        func = testingService.functions[4]
        self.assertEqual(func.name, "complex_action")
        self.assertEqual(func.return_type.value, ThriftPrimitiveType.THRIFT_I32_TYPE)
        self.assertEqual(func.exceptions, [])
        self.assertFalse(func.is_oneway)
        self.assertEqual(len(func.arguments), 4)

        arg = func.arguments[2]
        self.assertEqual(arg.name, "third")
        self.assertFalse(arg.is_optional)
        self.assertEqual(arg.type.value, ThriftPrimitiveType.THRIFT_I64_TYPE)
