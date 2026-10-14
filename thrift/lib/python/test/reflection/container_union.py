#!/usr/bin/env python3
# Copyright (c) Meta Platforms, Inc. and affiliates.
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

from python_test.reflection.container_union.thrift_types import Choice, Containers, Item
from thrift.python.reflection.inspect_impl import inspect
from thrift.python.reflection.types_reflection import (
    ListSpec,
    MapSpec,
    StructSpec,
    ThriftType,
)


class ContainerUnionReflectionTest(unittest.TestCase):
    """Container reflection reports union elements as UNION, like the
    generated reflection of a union-typed field."""

    def setUp(self) -> None:
        self.value = Containers(
            choice_list=[Choice(number=1)],
            choice_map={"a": Choice(text="b")},
            nested_choices={1: [Choice(number=2)]},
            item_list=[Item(id=3)],
        )

    def test_list_of_unions(self) -> None:
        spec = inspect(self.value.choice_list)
        assert isinstance(spec, ListSpec)
        self.assertIs(spec.value, Choice)
        self.assertEqual(spec.thrift_type, ThriftType.UNION)

    def test_map_of_unions(self) -> None:
        spec = inspect(self.value.choice_map)
        assert isinstance(spec, MapSpec)
        self.assertEqual(spec.key_thrift_type, ThriftType.STRING)
        self.assertIs(spec.value, Choice)
        self.assertEqual(spec.value_thrift_type, ThriftType.UNION)

    def test_nested_list_of_unions(self) -> None:
        spec = inspect(self.value.nested_choices[1])
        assert isinstance(spec, ListSpec)
        self.assertEqual(spec.thrift_type, ThriftType.UNION)

    def test_list_of_structs_is_still_struct(self) -> None:
        spec = inspect(self.value.item_list)
        assert isinstance(spec, ListSpec)
        self.assertIs(spec.value, Item)
        self.assertEqual(spec.thrift_type, ThriftType.STRUCT)

    def test_matches_field_reflection(self) -> None:
        # The generated reflection of the struct's fields and the runtime
        # reflection of their container values agree on the element kind.
        struct_spec = inspect(Containers)
        assert isinstance(struct_spec, StructSpec)
        field = struct_spec.get_field("choice_list")
        assert field is not None
        field_type_spec = inspect(field.type)
        assert isinstance(field_type_spec, ListSpec)
        self.assertEqual(
            field_type_spec.thrift_type, inspect(self.value.choice_list).thrift_type
        )
