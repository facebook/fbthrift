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

# Wire-level tests of the protocol readers and writers. Expected bytes are
# derived by hand from the C++ writers in thrift/lib/cpp2/protocol/.

from __future__ import annotations

import functools
import math
import struct

from thrift.lib.python.schema._errors import EncodeError
from thrift.lib.python.schema._protocols import Backend, Protocol, ProtocolWriter
from thrift.lib.python.schema._ttypes import (
    T_BOOL,
    T_BYTE,
    T_DOUBLE,
    T_FLOAT,
    T_I16,
    T_I32,
    T_I64,
    T_STRING,
    T_STRUCT,
)
from thrift.lib.python.schema.tests.protocol_testing import ProtocolTestCase, WriteCall


def _f32(value: float) -> float:
    return struct.unpack("<f", struct.pack("<f", value))[0]


def _struct_of(*fields: tuple[str, int, WriteCall]) -> WriteCall:
    def write(writer: ProtocolWriter) -> None:
        writer.write_struct_begin("S")
        for field_id, (name, ttype, value) in enumerate(fields, start=1):
            writer.write_field_begin(name, ttype, field_id)
            value(writer)
            writer.write_field_end()
        writer.write_field_stop()
        writer.write_struct_end()

    return write


def _map_of(key_ttype: int, *entries: tuple[WriteCall, WriteCall]) -> WriteCall:
    def write(writer: ProtocolWriter) -> None:
        writer.write_map_begin(key_ttype, T_I32, len(entries))
        for key, value in entries:
            key(writer)
            value(writer)
        writer.write_map_end()

    return write


def _list_of(*elements: WriteCall) -> WriteCall:
    def write(writer: ProtocolWriter) -> None:
        writer.write_list_begin(T_I32, len(elements))
        for element in elements:
            element(writer)
        writer.write_list_end()

    return write


def _one(writer: ProtocolWriter) -> None:
    writer.write_i32(1)


def _empty_set(writer: ProtocolWriter) -> None:
    writer.write_set_begin(T_I32, 0)
    writer.write_set_end()


def _raw_struct(writer: ProtocolWriter) -> None:
    writer.write_raw(b"{}")


class SimpleJsonWriterTest(ProtocolTestCase):
    """Byte-exact SimpleJSON, as `SimpleJSONProtocolWriter` writes it."""

    def written(self, backend: Backend, call: WriteCall) -> bytes:
        writer = backend.writers[Protocol.SIMPLE_JSON]()
        call(writer)
        return writer.getvalue()

    def assert_writes(self, cases: list[tuple[WriteCall, str]]) -> None:
        for backend in self.backends():
            for call, expected in cases:
                with self.subTest(expected=expected):
                    self.assertEqual(
                        self.written(backend, call), expected.encode("utf-8")
                    )

    def test_structs_use_field_names_and_no_whitespace(self) -> None:
        self.assert_writes(
            [
                (_struct_of(), "{}"),
                (
                    _struct_of(
                        ("a", T_I32, _one),
                        ("b", T_STRING, lambda w: w.write_string("x")),
                        ('q"uote', T_BOOL, lambda w: w.write_bool(False)),
                    ),
                    '{"a":1,"b":"x","q\\"uote":false}',
                ),
                (
                    _struct_of(("inner", T_STRUCT, _struct_of(("x", T_I32, _one)))),
                    '{"inner":{"x":1}}',
                ),
            ]
        )

    def test_lists_sets_and_maps(self) -> None:
        self.assert_writes(
            [
                (_list_of(), "[]"),
                (_list_of(_one, _one), "[1,1]"),
                (_list_of(_list_of(_one), _list_of()), "[[1],[]]"),
                (_map_of(T_STRING), "{}"),
                (
                    _map_of(
                        T_STRING,
                        (lambda w: w.write_string("k"), _list_of(_one)),
                        (lambda w: w.write_string("j"), _struct_of(("x", T_I32, _one))),
                    ),
                    '{"k":[1],"j":{"x":1}}',
                ),
                (_list_of(_map_of(T_I32, (_one, _one))), '[{"1":1}]'),
            ]
        )

    def test_map_keys_are_quoted_by_key_type(self) -> None:
        self.assert_writes(
            [
                (_map_of(T_I32, (_one, _one)), '{"1":1}'),
                (_map_of(T_BYTE, (lambda w: w.write_byte(-1), _one)), '{"-1":1}'),
                (_map_of(T_I16, (lambda w: w.write_i16(7), _one)), '{"7":1}'),
                (
                    _map_of(T_I64, (lambda w: w.write_i64(2**40), _one)),
                    '{"1099511627776":1}',
                ),
                (_map_of(T_BOOL, (lambda w: w.write_bool(True), _one)), '{"true":1}'),
                (_map_of(T_DOUBLE, (lambda w: w.write_double(1.5), _one)), '{"1.5":1}'),
                (_map_of(T_DOUBLE, (lambda w: w.write_double(-0.0), _one)), '{"-0":1}'),
                (
                    _map_of(T_DOUBLE, (lambda w: w.write_double(math.nan), _one)),
                    '{"NaN":1}',
                ),
                (
                    _map_of(T_FLOAT, (lambda w: w.write_float(_f32(0.1)), _one)),
                    '{"0.1":1}',
                ),
                (_map_of(T_STRING, (lambda w: w.write_string("s"), _one)), '{"s":1}'),
                (
                    _map_of(T_STRING, (lambda w: w.write_binary(b"\x00\x01"), _one)),
                    '{"AAE":1}',
                ),
            ]
        )

    def test_struct_and_container_map_keys_are_rejected(self) -> None:
        # C++ writes these unquoted, which is not JSON.
        keys: list[WriteCall] = [
            _struct_of(),
            _list_of(),
            _map_of(T_I32),
            _empty_set,
            _raw_struct,
        ]
        for backend in self.backends():
            for key in keys:
                with self.subTest(key=key):
                    with self.assertRaises(EncodeError):
                        self.written(backend, _map_of(T_STRUCT, (key, _one)))

    def test_scalars(self) -> None:
        self.assert_writes(
            [
                (lambda w: w.write_bool(True), "true"),
                (lambda w: w.write_bool(False), "false"),
                (lambda w: w.write_byte(-128), "-128"),
                (lambda w: w.write_i16(32767), "32767"),
                (lambda w: w.write_i32(-(2**31)), "-2147483648"),
                (lambda w: w.write_i64(2**63 - 1), "9223372036854775807"),
                (lambda w: w.write_binary(b""), '""'),
                (lambda w: w.write_binary(b"a"), '"YQ"'),
                (lambda w: w.write_binary(b"ab"), '"YWI"'),
                (lambda w: w.write_binary(b"abc"), '"YWJj"'),
                (lambda w: w.write_binary(b"\xfb\xff"), '"+/8"'),
                (lambda w: w.write_raw(b'{"x":1}'), '{"x":1}'),
            ]
        )

    def test_strings_escape_quotes_backslashes_and_controls(self) -> None:
        self.assert_writes(
            [
                (lambda w: w.write_string(""), '""'),
                (lambda w: w.write_string('a"b\\c'), '"a\\"b\\\\c"'),
                (
                    lambda w: w.write_string("\b\t\n\f\r\x00\x01\x1f\x7f"),
                    '"\\b\\t\\n\\f\\r\\u0000\\u0001\\u001f\x7f"',
                ),
                (lambda w: w.write_string("é ☃ 😀 /"), '"é ☃ 😀 /"'),
            ]
        )

    def test_doubles_follow_fmt(self) -> None:
        cases = [
            (1.0, "1"),
            (0.1, "0.1"),
            (1e16, "1e+16"),
            (1e15, "1000000000000000"),
            (1e-5, "1e-05"),
            (1e-4, "0.0001"),
            (-0.0, "-0"),
            (0.0, "0"),
            (123.456, "123.456"),
            (1.5e300, "1.5e+300"),
            (5e-324, "5e-324"),
            (1.7976931348623157e308, "1.7976931348623157e+308"),
            (1234567890123456.8, "1234567890123456.8"),
            (0.30000000000000004, "0.30000000000000004"),
            (1e22, "1e+22"),
            (2.2250738585072014e-308, "2.2250738585072014e-308"),
            (123456789012345680.0, "1.2345678901234568e+17"),
            (math.inf, '"Infinity"'),
            (-math.inf, '"-Infinity"'),
            (math.nan, '"NaN"'),
            (-math.nan, '"NaN"'),
        ]
        self.assert_writes(
            [
                (functools.partial(_write_double, value=value), text)
                for value, text in cases
            ]
        )

    def test_floats_follow_fmt_9(self) -> None:
        cases = [
            (0.1, "0.1"),
            (1.0, "1"),
            (3.4028234663852886e38, "3.4028235e+38"),
            (1.401298464324817e-45, "1e-45"),
            (1.1754943508222875e-38, "1.1754944e-38"),
            (1234567.0, "1234567"),
            (0.3, "0.3"),
            (-2.5, "-2.5"),
            (1e-5, "1e-05"),
            (-0.0, "-0"),
            (3.1415927, "3.1415927"),
            (9.999999e-5, "9.999999e-05"),
            (8388609.0, "8388609"),
            (5e-39, "5e-39"),
            # fmt 12 switches float32 to exponent notation from 1e7 up.
            (16777216.0, "16777216"),
            (123456789.0, "123456790"),
            (1e15, "1000000000000000"),
            (1e16, "1e+16"),
            (math.inf, '"Infinity"'),
            (math.nan, '"NaN"'),
        ]
        self.assert_writes(
            [
                (functools.partial(_write_float, value=_f32(value)), text)
                for value, text in cases
            ]
        )


def _write_double(writer: ProtocolWriter, value: float) -> None:
    writer.write_double(value)


def _write_float(writer: ProtocolWriter, value: float) -> None:
    writer.write_float(value)
