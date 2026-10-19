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


"""
SimpleJSON: the writer, byte-identical to C++ ``SimpleJSONProtocolWriter``
(``thrift/lib/cpp2/protocol/SimpleJSONProtocol-inl.h``), and the parser that
turns input into a tree for the walk to read.

A struct is ``{"name":value,...}``, a list or set ``[...]``, and a map
``{key:value,...}``, with no whitespace. Map keys of integer, enum, bool, and
floating-point types are quoted; struct and container keys, which C++ writes
as invalid JSON, raise ``EncodeError``.
"""

from __future__ import annotations

import functools
import json
from typing import ClassVar, NoReturn

# pyrefly: ignore [missing-import]
from thrift.lib.python.schema._errors import DecodeError, EncodeError

# pyrefly: ignore [missing-import]
from thrift.lib.python.schema._protocol_common import decode_utf8

# pyrefly: ignore [missing-import]
from thrift.lib.python.schema._simple_json_scalars import (
    json_binary,
    json_double,
    json_float32,
    json_string,
)

# pyrefly: ignore [missing-import]
from thrift.lib.python.schema._ttypes import Protocol


class _Context:
    """An open object or array. ``meta`` counts the items written in it;
    in an object, even items are keys."""

    __slots__ = ("is_object", "meta")
    is_object: bool
    meta: int

    def __init__(self, is_object: bool) -> None:
        self.is_object = is_object
        self.meta = 0


class SimpleJsonWriter:
    __slots__ = ("_out", "_contexts")
    protocol: ClassVar[Protocol] = Protocol.SIMPLE_JSON

    _out: list[str]
    _contexts: list[_Context]

    def __init__(self) -> None:
        self._out = []
        self._contexts = []

    def getvalue(self) -> bytes:
        return "".join(self._out).encode("utf-8")

    def _write_context(self) -> bool:
        """Write the separator before an item; ``True`` if it is a map key."""
        if not self._contexts:
            return False
        context = self._contexts[-1]
        meta = context.meta
        context.meta += 1
        if context.is_object:
            if meta % 2 == 0:
                if meta:
                    self._out.append(",")
                return True
            self._out.append(":")
        elif meta:
            self._out.append(",")
        return False

    def _begin(self, is_object: bool) -> None:
        if self._write_context():
            raise EncodeError("SimpleJSON cannot write a struct or container map key")
        self._contexts.append(_Context(is_object))
        self._out.append("{" if is_object else "[")

    def _end(self) -> None:
        self._out.append("}" if self._contexts.pop().is_object else "]")

    def _write_number(self, text: str) -> None:
        # Numbers are quoted in key position; "NaN" and the infinities
        # already are.
        if self._write_context() and not text.startswith('"'):
            text = f'"{text}"'
        self._out.append(text)

    def write_struct_begin(self, name: str) -> None:
        self._begin(True)

    def write_struct_end(self) -> None:
        self._end()

    def write_field_begin(self, name: str, ttype: int, field_id: int) -> None:
        self._write_context()
        self._out.append(json_string(name))

    def write_field_end(self) -> None:
        pass

    def write_field_stop(self) -> None:
        pass

    def write_map_begin(self, key_ttype: int, value_ttype: int, size: int) -> None:
        self._begin(True)

    def write_map_end(self) -> None:
        self._end()

    def write_list_begin(self, element_ttype: int, size: int) -> None:
        self._begin(False)

    def write_list_end(self) -> None:
        self._end()

    def write_set_begin(self, element_ttype: int, size: int) -> None:
        self._begin(False)

    def write_set_end(self) -> None:
        self._end()

    def write_bool(self, value: bool) -> None:
        self._write_number("true" if value else "false")

    def write_byte(self, value: int) -> None:
        self._write_number(str(value))

    def write_i16(self, value: int) -> None:
        self._write_number(str(value))

    def write_i32(self, value: int) -> None:
        self._write_number(str(value))

    def write_i64(self, value: int) -> None:
        self._write_number(str(value))

    def write_float(self, value: float) -> None:
        self._write_number(json_float32(value))

    def write_double(self, value: float) -> None:
        self._write_number(json_double(value))

    def write_string(self, value: str) -> None:
        self._write_context()
        self._out.append(json_string(value))

    def write_binary(self, value: bytes) -> None:
        self._write_context()
        self._out.append(json_binary(value))

    def write_raw(self, data: bytes) -> None:
        if self._write_context():
            raise EncodeError("SimpleJSON cannot write a struct map key")
        self._out.append(decode_utf8(data))


# ---------------------------------------------------------------------------
# Reading: the ``json`` module (C-accelerated) builds a tree, which the walk
# then reads against a ``TypeRef``.
# ---------------------------------------------------------------------------


class JsonObject(list[tuple[str, object]]):
    """A JSON object, as its ``(name, value)`` pairs in input order, so that
    repeated names stay visible."""

    __slots__ = ()


class JsonNumber:
    """A JSON number, kept as its literal so that ``-0`` and float32 rounding
    are exact."""

    __slots__ = ("literal",)
    literal: str

    def __init__(self, literal: str) -> None:
        self.literal = literal

    def __repr__(self) -> str:
        return f"JsonNumber({self.literal})"


def _reject_constant(name: str) -> NoReturn:
    raise ValueError(f"bare {name} is not valid; it must be quoted")


@functools.cache
def _decoder() -> json.JSONDecoder:
    # C++ accepts raw control characters inside strings, hence strict=False.
    return json.JSONDecoder(
        object_pairs_hook=JsonObject,
        parse_float=JsonNumber,
        parse_int=JsonNumber,
        parse_constant=_reject_constant,
        strict=False,
    )


def parse_json(data: bytes) -> object:
    """The tree of a JSON document: ``JsonObject``, ``list``, ``str``,
    ``JsonNumber``, ``bool``, and ``None``. Raises ``DecodeError`` if ``data``
    is not one JSON value (or ``RecursionError`` if it nests too deeply)."""
    try:
        return _decoder().decode(decode_utf8(data))
    except ValueError as e:  # includes json.JSONDecodeError
        raise DecodeError(f"invalid JSON: {e}") from e


def json_kind(node: object) -> str:
    """The JSON kind of a tree node, for error messages."""
    match node:
        case JsonObject():
            return "object"
        case list():
            return "array"
        case str():
            return "string"
        case JsonNumber():
            return "number"
        case bool():
            return "bool"
        case None:
            return "null"
        case _:
            raise AssertionError(f"unexpected JSON node {node!r}")


def dump_json(node: object) -> str:
    """A tree as JSON text again, with numbers as they were written."""
    match node:
        case JsonObject():
            items = ",".join(f"{json.dumps(k)}:{dump_json(v)}" for k, v in node)
            return f"{{{items}}}"
        case list():
            return f"[{','.join(dump_json(e) for e in node)}]"
        case JsonNumber():
            return node.literal
        case _:
            return json.dumps(node)
