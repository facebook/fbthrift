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

from thrift.lib.python.schema._errors import DecodeError
from thrift.lib.python.schema._protocols import (
    available_backends,
    Backend,
    Protocol,
    ProtocolReader,
    ProtocolWriter,
)
from thrift.lib.python.schema._ttypes import (
    PROTOCOL_MAX_DEPTH,
    T_LIST,
    T_STOP,
    T_STRUCT,
    T_VOID,
)

WriteCall = Callable[[ProtocolWriter], None]
ReadCall = Callable[[ProtocolReader], object]


class ProtocolTestCase(unittest.TestCase):
    """Run protocol tests once per available backend."""

    def backends(self) -> Iterator[Backend]:
        for backend in available_backends():
            with self.subTest(backend=backend.name):
                yield backend


class WireTestCase(ProtocolTestCase):
    PROTOCOL: Protocol

    def writer(self, backend: Backend) -> ProtocolWriter:
        return backend.writers[self.PROTOCOL]()

    def reader(self, backend: Backend, data: bytes) -> ProtocolReader:
        return backend.readers[self.PROTOCOL](data)

    def written(self, backend: Backend, *calls: WriteCall) -> bytes:
        writer = self.writer(backend)
        for call in calls:
            call(writer)
        return writer.getvalue()

    def assert_writes(self, cases: list[tuple[WriteCall, bytes]]) -> None:
        for backend in self.backends():
            for call, expected in cases:
                with self.subTest(expected=expected):
                    self.assertEqual(self.written(backend, call), expected)

    def assert_reads(self, cases: list[tuple[bytes, ReadCall, object]]) -> None:
        """Each read consumes exactly its bytes, and fails on every proper
        prefix of them."""
        for backend in self.backends():
            for data, call, expected in cases:
                with self.subTest(data=data):
                    reader = self.reader(backend, data)
                    self.assertEqual(call(reader), expected)
                    self.assertEqual(reader.remaining(), 0)
                    for end in range(len(data)):
                        with self.assertRaises(DecodeError, msg=data[:end]):
                            call(self.reader(backend, data[:end]))

    def assert_read_fails(self, cases: list[tuple[bytes, ReadCall]]) -> None:
        for backend in self.backends():
            for data, call in cases:
                with self.subTest(data=data):
                    with self.assertRaises(DecodeError):
                        call(self.reader(backend, data))

    def assert_skips(self, cases: list[tuple[int, bytes]]) -> None:
        """``skip`` consumes exactly the value, fails on every truncation of
        it, and ``read_raw`` returns it."""
        for backend in self.backends():
            for ttype, data in cases:
                with self.subTest(ttype=ttype, data=data):
                    reader = self.reader(backend, data + b"tail")
                    reader.skip(ttype)
                    self.assertEqual(reader.remaining(), 4)
                    self.assertEqual(self.reader(backend, data).read_raw(ttype), data)
                    for end in range(len(data)):
                        with self.assertRaises(DecodeError, msg=data[:end]):
                            self.reader(backend, data[:end]).skip(ttype)

    def test_skip_rejects_types_without_values(self) -> None:
        for backend in self.backends():
            for ttype in (T_STOP, T_VOID, 5, 7, 20, 255):
                with self.subTest(ttype=ttype):
                    with self.assertRaises(DecodeError):
                        self.reader(backend, b"\x00" * 16).skip(ttype)

    def test_reader_nesting_limit(self) -> None:
        for backend in self.backends():
            reader = self.reader(backend, b"")
            for _ in range(PROTOCOL_MAX_DEPTH):
                reader.read_struct_begin()
            with self.assertRaises(DecodeError):
                reader.read_struct_begin()
            reader.read_struct_end()
            reader.read_struct_begin()  # the level is free again

    def nested_structs(self, levels: int) -> bytes:
        raise NotImplementedError

    def nested_lists(self, levels: int) -> bytes:
        raise NotImplementedError

    def test_skip_depth_limit_matches_cpp(self) -> None:
        # C++ `skip` fails when its depth reaches FLAGS_thrift_protocol_max_depth.
        # A struct field is skipped one level deeper than its struct, and a
        # container element two levels deeper (`skip` -> `skip_n` -> `skip`).
        cases = [
            (T_STRUCT, self.nested_structs, PROTOCOL_MAX_DEPTH),
            (T_LIST, self.nested_lists, PROTOCOL_MAX_DEPTH // 2),
        ]
        for backend in self.backends():
            for ttype, make, deepest in cases:
                with self.subTest(ttype=ttype):
                    reader = self.reader(backend, make(deepest))
                    reader.skip(ttype)
                    self.assertEqual(reader.remaining(), 0)
                    with self.assertRaises(DecodeError):
                        self.reader(backend, make(deepest + 1)).skip(ttype)
