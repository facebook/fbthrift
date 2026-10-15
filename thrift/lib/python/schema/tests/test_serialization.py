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

from __future__ import annotations

import unittest
from collections.abc import Callable, Iterator

import thrift.lib.python.schema.tests.codec_test.thrift_types as fixtures
from thrift.lib.python.schema import serialization
from thrift.lib.python.schema._datum import check_value
from thrift.lib.python.schema._protocols import default_backend, ProtocolWriter
from thrift.lib.python.schema._ttypes import T_I32, T_I64, T_LIST, T_STRING, T_STRUCT
from thrift.lib.python.schema.dynamic_value import MutableDynamicValue
from thrift.lib.python.schema.serialization import (
    DecodeError,
    deserialize,
    Protocol,
    serialize,
)
from thrift.lib.python.schema.tests.datum_testing import (
    field_ids,
    Generated,
    nested,
    primitives,
    sample_objects,
    to_value,
    type_ref_of,
)
from thrift.lib.python.schema.type_system import TypeRef
from thrift.python import serializer as thrift_python

PROTOCOLS: tuple[Protocol, ...] = (Protocol.BINARY, Protocol.COMPACT)
THRIFT_PYTHON_PROTOCOLS: dict[Protocol, thrift_python.Protocol] = {
    Protocol.BINARY: thrift_python.Protocol.BINARY,
    Protocol.COMPACT: thrift_python.Protocol.COMPACT,
}

POINT: TypeRef = type_ref_of(fixtures.Point)
CHOICE: TypeRef = type_ref_of(fixtures.Choice)
CONTAINERS: TypeRef = type_ref_of(fixtures.Containers)
PRESENCE: TypeRef = type_ref_of(fixtures.Presence)
NESTED: TypeRef = type_ref_of(fixtures.Nested)

# A struct with no fields: just the stop byte, in Binary and Compact.
EMPTY_STRUCT = b"\x00"

# thrift-python writes fields in id order; the codec, in TypeSystem (IDL) order.
NOT_BYTE_EQUAL: frozenset[str] = frozenset({"field_ids"})


def value_of(obj: Generated) -> MutableDynamicValue:
    type_ref = type_ref_of(type(obj))
    return MutableDynamicValue(type_ref, to_value(obj, type_ref))


def generated_bytes(obj: Generated, protocol: Protocol) -> bytes:
    return thrift_python.serialize(obj, THRIFT_PYTHON_PROTOCOLS[protocol])


def generated_object(cls: type[Generated], data: bytes, protocol: Protocol) -> object:
    return thrift_python.deserialize(cls, data, THRIFT_PYTHON_PROTOCOLS[protocol])


def written(protocol: Protocol, write: Callable[[ProtocolWriter], None]) -> bytes:
    """Hand-built input, for wire shapes the codec never writes."""
    writer = default_backend().writers[protocol]()
    write(writer)
    return writer.getvalue()


class SerializationTest(unittest.TestCase):
    def protocols(self) -> Iterator[Protocol]:
        for protocol in PROTOCOLS:
            with self.subTest(protocol=protocol.name):
                yield protocol


class ThriftPythonParityTest(SerializationTest):
    def test_serialize_matches_thrift_python(self) -> None:
        for protocol in self.protocols():
            for name, obj in sample_objects():
                with self.subTest(sample=name):
                    encoded = serialize(value_of(obj), protocol)
                    self.assertEqual(
                        generated_object(type(obj), encoded, protocol), obj
                    )
                    if name not in NOT_BYTE_EQUAL:
                        self.assertEqual(encoded, generated_bytes(obj, protocol))

    def test_deserialize_thrift_python_output(self) -> None:
        for protocol in self.protocols():
            for name, obj in sample_objects():
                with self.subTest(sample=name):
                    decoded = deserialize(
                        type_ref_of(type(obj)), generated_bytes(obj, protocol), protocol
                    )
                    self.assertEqual(decoded, value_of(obj))

    def test_round_trip(self) -> None:
        for protocol in self.protocols():
            for name, obj in sample_objects():
                with self.subTest(sample=name):
                    value = value_of(obj)
                    self.assertEqual(
                        deserialize(
                            value.type_ref, serialize(value, protocol), protocol
                        ),
                        value,
                    )


class ApiTest(SerializationTest):
    def test_compact_is_the_default_protocol(self) -> None:
        value = value_of(primitives())
        data = serialize(value)
        self.assertEqual(data, serialize(value, Protocol.COMPACT))
        self.assertEqual(deserialize(value.type_ref, data), value)

    def test_deserialize_accepts_any_bytes_like_input(self) -> None:
        value = value_of(primitives())
        for protocol in self.protocols():
            data = serialize(value, protocol)
            for buffer in (bytearray(data), memoryview(data)):
                with self.subTest(buffer=type(buffer).__name__):
                    self.assertEqual(
                        deserialize(value.type_ref, buffer, protocol), value
                    )

    def test_serialize_accepts_read_only_and_frozen_handles(self) -> None:
        point = fixtures.Point(x=1, y=2)
        value = value_of(fixtures.Containers(points={point}))
        (element,) = value.as_struct()["points"].as_set()
        self.assertIsInstance(element._datum, tuple)
        for protocol in self.protocols():
            self.assertEqual(
                serialize(value.read_only(), protocol), serialize(value, protocol)
            )
            self.assertEqual(
                serialize(element, protocol), serialize(value_of(point), protocol)
            )

    def test_unsupported_protocols_are_rejected(self) -> None:
        value = value_of(primitives())
        with self.assertRaises(ValueError):
            serialize(value, Protocol.SIMPLE_JSON)
        with self.assertRaises(ValueError):
            deserialize(value.type_ref, b"{}", Protocol.SIMPLE_JSON)

    def test_exports(self) -> None:
        self.assertEqual(
            sorted(serialization.__all__),
            ["DecodeError", "EncodeError", "Protocol", "deserialize", "serialize"],
        )


class DecodedStorageTest(SerializationTest):
    def test_returns_an_owned_mutable_value_of_the_requested_type(self) -> None:
        for protocol in self.protocols():
            data = generated_bytes(nested(), protocol)
            decoded = deserialize(NESTED, data, protocol)
            self.assertIs(type(decoded), MutableDynamicValue)
            self.assertIs(decoded.type_ref, NESTED)
            decoded.as_struct()["choice"].as_union().set_field("number", 42)
            self.assertEqual(deserialize(NESTED, data, protocol), value_of(nested()))

    def test_storage_is_canonical(self) -> None:
        # Owned lists and dicts, frozen map keys and set elements, `[]` for an
        # empty union, and UNSET only in optional fields.
        for protocol in self.protocols():
            for name, obj in sample_objects():
                with self.subTest(sample=name):
                    decoded = deserialize(
                        type_ref_of(type(obj)), generated_bytes(obj, protocol), protocol
                    )
                    check_value(decoded._datum, decoded.type_ref)

    def test_defaults_are_not_shared_between_decoded_values(self) -> None:
        for protocol in self.protocols():
            first = deserialize(PRESENCE, EMPTY_STRUCT, protocol).as_struct()
            first["ints_default"].as_list().append(4)
            first["map_default"].as_map().set("b", 2)
            first["point_default"].as_struct()["x"] = 0
            second = deserialize(PRESENCE, EMPTY_STRUCT, protocol)
            self.assertEqual(second, MutableDynamicValue.default(PRESENCE))


class FieldPresenceTest(SerializationTest):
    def test_terse_fields_are_always_written(self) -> None:
        # Like C++ DynamicValue, every non-optional field is written;
        # thrift-python omits terse fields at their default.
        default = MutableDynamicValue.default(PRESENCE)
        for protocol in self.protocols():
            encoded = serialize(default, protocol)
            self.assertGreater(
                len(encoded), len(generated_bytes(fixtures.Presence(), protocol))
            )
            self.assertEqual(
                generated_object(fixtures.Presence, encoded, protocol),
                fixtures.Presence(),
            )

    def test_unset_optional_fields_are_omitted(self) -> None:
        obj = fixtures.Presence(terse=1, terse_string="x")
        for protocol in self.protocols():
            self.assertEqual(
                serialize(value_of(obj), protocol), generated_bytes(obj, protocol)
            )

    def test_missing_fields_take_their_defaults(self) -> None:
        for protocol in self.protocols():
            decoded = deserialize(PRESENCE, EMPTY_STRUCT, protocol)
            self.assertEqual(decoded, MutableDynamicValue.default(PRESENCE))
            self.assertEqual(decoded, value_of(fixtures.Presence()))
            fields = decoded.as_struct()
            self.assertNotIn("maybe", fields)
            self.assertEqual(fields["plain_default"].as_int(), 7)
            self.assertEqual(
                fields["choice_default"].as_union().field("text").as_str(), "hi"
            )


class ReadValidationTest(SerializationTest):
    def test_unknown_fields_are_skipped(self) -> None:
        # FieldIds shares fields 1 and 2 (both i32) with Point.
        for protocol in self.protocols():
            decoded = deserialize(
                POINT, generated_bytes(field_ids(), protocol), protocol
            )
            self.assertEqual(decoded, value_of(fixtures.Point(x=1, y=2)))

    def test_unknown_enum_values_are_kept(self) -> None:
        fields = to_value(primitives(), type_ref_of(fixtures.Primitives))
        assert isinstance(fields, list)
        fields[9] = 99
        value = MutableDynamicValue(type_ref_of(fixtures.Primitives), fields)
        for protocol in self.protocols():
            self.assertEqual(
                deserialize(value.type_ref, serialize(value, protocol), protocol),
                value,
            )

    def test_field_type_mismatch_is_rejected(self) -> None:
        # Primitives field 1 is a bool, Point field 1 an i32.
        for protocol in self.protocols():
            with self.assertRaisesRegex(DecodeError, "'x'"):
                deserialize(POINT, generated_bytes(primitives(), protocol), protocol)

    def test_element_types_are_checked_only_in_non_empty_containers(self) -> None:
        def ints(size: int) -> Callable[[ProtocolWriter], None]:
            def write(writer: ProtocolWriter) -> None:
                writer.write_struct_begin("Containers")
                writer.write_field_begin("ints", T_LIST, 1)
                writer.write_list_begin(T_I64, size)
                for _ in range(size):
                    writer.write_i64(5)
                writer.write_list_end()
                writer.write_field_end()
                writer.write_field_stop()
                writer.write_struct_end()

            return write

        for protocol in self.protocols():
            with self.assertRaisesRegex(DecodeError, "list element"):
                deserialize(CONTAINERS, written(protocol, ints(1)), protocol)
            decoded = deserialize(CONTAINERS, written(protocol, ints(0)), protocol)
            self.assertEqual(decoded, MutableDynamicValue.default(CONTAINERS))

    def test_unions(self) -> None:
        def union(
            *fields: tuple[int, int, int | str],
        ) -> Callable[[ProtocolWriter], None]:
            def write(writer: ProtocolWriter) -> None:
                writer.write_struct_begin("Choice")
                for field_id, ttype, payload in fields:
                    writer.write_field_begin("", ttype, field_id)
                    if isinstance(payload, int):
                        writer.write_i32(payload)
                    else:
                        writer.write_string(payload)
                    writer.write_field_end()
                writer.write_field_stop()
                writer.write_struct_end()

            return write

        for protocol in self.protocols():
            decoded = deserialize(
                CHOICE, written(protocol, union((99, T_I32, 1))), protocol
            )
            self.assertIsNone(decoded.as_union().active)
            with self.assertRaisesRegex(DecodeError, "more than one field"):
                deserialize(
                    CHOICE,
                    written(protocol, union((1, T_I32, 1), (2, T_STRING, "x"))),
                    protocol,
                )
            with self.assertRaisesRegex(DecodeError, "more than one field"):
                deserialize(
                    CHOICE,
                    written(protocol, union((99, T_I32, 1), (1, T_I32, 1))),
                    protocol,
                )
            with self.assertRaisesRegex(DecodeError, "'number'"):
                deserialize(
                    CHOICE, written(protocol, union((1, T_STRING, "x"))), protocol
                )


class MalformedInputTest(SerializationTest):
    def test_truncated_input(self) -> None:
        for protocol in self.protocols():
            data = generated_bytes(nested(), protocol)
            for end in range(len(data)):
                with self.subTest(end=end), self.assertRaises(DecodeError):
                    deserialize(NESTED, data[:end], protocol)

    def test_trailing_bytes(self) -> None:
        for protocol in self.protocols():
            data = generated_bytes(fixtures.Point(x=1), protocol)
            with self.assertRaisesRegex(DecodeError, "1 bytes left"):
                deserialize(POINT, data + b"\x00", protocol)

    def test_sizes_beyond_the_input(self) -> None:
        def huge_list(writer: ProtocolWriter) -> None:
            writer.write_struct_begin("Containers")
            writer.write_field_begin("ints", T_LIST, 1)
            writer.write_list_begin(T_I32, 2**31 - 1)

        for protocol in self.protocols():
            with self.assertRaisesRegex(DecodeError, "truncated"):
                deserialize(CONTAINERS, written(protocol, huge_list), protocol)

    def test_deep_nesting(self) -> None:
        def nested_structs(depth: int) -> Callable[[ProtocolWriter], None]:
            def write(writer: ProtocolWriter) -> None:
                for _ in range(depth):
                    writer.write_struct_begin("Nested")
                    writer.write_field_begin("next", T_STRUCT, 3)
                writer.write_struct_begin("Nested")
                writer.write_field_stop()
                writer.write_struct_end()
                for _ in range(depth):
                    writer.write_field_end()
                    writer.write_field_stop()
                    writer.write_struct_end()

            return write

        for protocol in self.protocols():
            shallow = deserialize(
                NESTED, written(protocol, nested_structs(20)), protocol
            )
            self.assertIn("next", shallow.as_struct())
            with self.assertRaisesRegex(DecodeError, "nested too deeply"):
                deserialize(NESTED, written(protocol, nested_structs(5000)), protocol)
