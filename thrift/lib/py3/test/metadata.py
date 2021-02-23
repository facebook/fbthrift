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
from testing.clients import TestingService
from testing.services import TestingServiceInterface
from testing.types import hard, HardError, Perm
from thrift.py3.metadata import gen_metadata


class MetadataTests(unittest.TestCase):
    def test_metadata_enums(self) -> None:
        meta = gen_metadata(testing.metadata)
        enumName = "testing.Perm"
        self.assertIsNotNone(meta)
        permEnum = meta.enums[enumName]
        self.assertEqual(permEnum.name, enumName)
        self.assertEqual(permEnum.elements[1], "execute")
        self.assertEqual(permEnum.elements[4], "read")
        self.assertEqual(len(permEnum.elements), 3)
        self.assertEqual(permEnum, gen_metadata(Perm))
        self.assertEqual(permEnum, gen_metadata(Perm(1)))

    def test_metadata_structs(self) -> None:
        meta = gen_metadata(testing.metadata)
        structName = "testing.hard"
        self.assertIsNotNone(meta)
        hardStruct = meta.structs[structName]

        self.assertFalse(hardStruct.is_union)
        self.assertEqual(hardStruct.name, structName)
        self.assertEqual(len(hardStruct.fields), 5)

        self.assertEqual(hardStruct, gen_metadata(hard))
        self.assertEqual(
            hardStruct, gen_metadata(hard(val=1, val_list=[1, 2], name="name"))
        )

        field = hardStruct.fields[2]
        self.assertEqual(field.name, "name")
        self.assertEqual(field.is_optional, False)
        self.assertEqual(field.type.value, ThriftPrimitiveType.THRIFT_STRING_TYPE)

        self.assertEqual(meta.structs["testing.EmptyUnion"].is_union, True)

    def test_metadata_exceptions(self) -> None:
        meta = gen_metadata(testing.metadata)
        errorName = "testing.HardError"
        self.assertIsNotNone(meta)
        hardError = meta.exceptions[errorName]
        self.assertEqual(hardError.name, errorName)
        self.assertEqual(len(hardError.fields), 2)

        self.assertEqual(hardError, gen_metadata(HardError))
        self.assertEqual(hardError, gen_metadata(HardError(code=1)))

        field = hardError.fields[1]
        self.assertEqual(field.name, "code")
        self.assertEqual(field.is_optional, False)
        self.assertEqual(field.type.value, ThriftPrimitiveType.THRIFT_I32_TYPE)

    def test_metadata_services(self) -> None:
        meta = gen_metadata(testing.metadata)
        serviceName = "testing.TestingService"
        self.assertIsNotNone(meta)
        testingService = meta.services[serviceName]
        self.assertEqual(testingService, gen_metadata(TestingService))
        self.assertEqual(testingService, gen_metadata(TestingServiceInterface))
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
