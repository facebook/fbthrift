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

"""Python value checks for Thrift primitive types."""

from __future__ import annotations

import math
import struct

from thrift.lib.python.schema._errors import EncodeError

_FLOAT32 = struct.Struct("<f")


def _mismatch(value: object, type_name: str) -> EncodeError:
    return EncodeError(
        f"value {value!r} of Python type '{type(value).__name__}' does not fit "
        f"type '{type_name}'"
    )


def check_bool(value: object, type_name: str = "bool") -> bool:
    if not isinstance(value, bool):
        raise _mismatch(value, type_name)
    return value


def int_in_range(value: int, bits: int) -> bool:
    bound = 1 << (bits - 1)
    return -bound <= value < bound


def check_int(value: object, bits: int, type_name: str) -> int:
    if isinstance(value, bool) or not isinstance(value, int):
        raise _mismatch(value, type_name)
    if not int_in_range(value, bits):
        raise EncodeError(f"value {value} is out of range for type '{type_name}'")
    return value


def is_float32(value: float) -> bool:
    """Whether ``value`` is exactly representable in IEEE-754 single
    precision (NaN and infinities are)."""
    if math.isnan(value) or math.isinf(value):
        return True
    try:
        return _FLOAT32.unpack(_FLOAT32.pack(value))[0] == value
    except OverflowError:
        return False


def check_float32(value: object, type_name: str = "float") -> float:
    if not isinstance(value, float):
        raise _mismatch(value, type_name)
    if not is_float32(value):
        raise EncodeError(
            f"value {value!r} is not exactly representable in type '{type_name}' "
            "(a float32)"
        )
    return value


def check_double(value: object, type_name: str = "double") -> float:
    if not isinstance(value, float):
        raise _mismatch(value, type_name)
    return value


def check_string(value: object, type_name: str = "string") -> str:
    if not isinstance(value, str):
        raise _mismatch(value, type_name)
    try:
        value.encode("utf-8")
    except UnicodeEncodeError as e:
        raise EncodeError(f"string is not valid UTF-8 at index {e.start}") from e
    return value


def check_binary(value: object, type_name: str = "binary") -> bytes:
    if not isinstance(value, bytes):
        raise _mismatch(value, type_name)
    return value
