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
SimpleJSON scalar encoding and decoding.

The SimpleJSON forms are byte-identical to C++ ``SimpleJSONProtocolWriter``
(``thrift/lib/cpp2/protocol/JSONProtocolCommon*``). Floating-point values use
fmt's ``{}`` layout: the shortest round-trip digits, in fixed notation for
decimal exponents in [-4, 16) and exponent notation (at least two exponent
digits) otherwise. fmt 12 lowers the float32 bound to 7; this follows fmt
9.1.0, the version C++ binaries link by default.
"""

from __future__ import annotations

import base64
import json
import math
import re
import struct
from collections.abc import Callable
from decimal import (
    Context,
    Decimal,
    DivisionByZero,
    InvalidOperation,
    localcontext,
    MAX_EMAX,
    MIN_EMIN,
    Overflow,
    ROUND_CEILING,
    ROUND_FLOOR,
    ROUND_HALF_EVEN,
)

# pyrefly: ignore [missing-import]
from thrift.lib.python.schema._errors import DecodeError

# pyrefly: ignore [missing-import]
from thrift.lib.python.schema._protocol_common import (
    decode_utf8 as decode_utf8,
    INT32_MAX as INT32_MAX,
)

# pyrefly: ignore [missing-import]
from thrift.lib.python.schema._value_checks import (
    check_binary as check_binary,
    check_bool as check_bool,
    check_double as check_double,
    check_float32 as check_float32,
    check_int as check_int,
    check_string as check_string,
    int_in_range,
    is_float32 as is_float32,
)

__all__ = [
    "INT32_MAX",
    "binary_from_json",
    "bool_from_json_key",
    "check_binary",
    "check_bool",
    "check_double",
    "check_float32",
    "check_int",
    "check_string",
    "decode_utf8",
    "floating_from_json",
    "floating_from_json_key",
    "int_from_json",
    "int_in_range",
    "is_float32",
    "json_binary",
    "json_double",
    "json_float32",
    "json_string",
    "special_floating_from_json",
    "utf8_text",
]

_FLOAT32 = struct.Struct("<f")
_FLOAT32_BITS = struct.Struct("<I")
_MAX_FLOAT32_BITS = 0x7F7FFFFF
_MAX_FLOAT32 = 3.4028234663852886e38
# The smallest magnitude that rounds to infinity in float32: halfway between
# the largest float32, (2**24 - 1) * 2**104, and 2**128.
_FLOAT32_OVERFLOW = Decimal((2**25 - 1) * 2**103)

# Decimal arithmetic runs in this context, never the caller's, which may have
# lower precision or trap ``Inexact``. 400 digits are exact for any sum of two
# float32 values.
_DECIMAL = Context(
    prec=400,
    rounding=ROUND_HALF_EVEN,
    Emin=MIN_EMIN,
    Emax=MAX_EMAX,
    capitals=1,
    clamp=0,
    flags=[],
    traps=[InvalidOperation, DivisionByZero, Overflow],
)


def utf8_text(text: str) -> str:
    """``text``, if it is valid UTF-8 (JSON escapes can spell lone
    surrogates)."""
    try:
        text.encode("utf-8")
    except UnicodeEncodeError as e:
        raise DecodeError(f"string is not valid UTF-8 at index {e.start}") from e
    return text


# ---------------------------------------------------------------------------
# SimpleJSON encoding
# ---------------------------------------------------------------------------


def _float32_bits(value: float) -> int:
    return _FLOAT32_BITS.unpack(_FLOAT32.pack(value))[0]


def _float32_from_bits(bits: int) -> float:
    return _FLOAT32.unpack(_FLOAT32_BITS.pack(bits))[0]


def _decimal_digits(value: Decimal) -> tuple[str, int]:
    """The significant digits of a nonzero ``value`` and the decimal exponent
    of its first digit."""
    normalized = value.normalize(_DECIMAL)
    digits = "".join(str(d) for d in normalized.as_tuple().digits)
    return digits, normalized.adjusted()


def _shortest_double(value: float) -> tuple[str, int]:
    # ``repr`` yields the shortest digits that round-trip, like fmt.
    return _decimal_digits(Decimal(repr(value)))


def _shortest_float32(value: float) -> tuple[str, int]:
    """The shortest decimal that rounds to the positive float32 ``value``,
    choosing the closest of equally short candidates (ties to even), as fmt's
    Dragonbox does. Computed exactly, so it is correct even at binade edges."""
    bits = _float32_bits(value)
    with localcontext(_DECIMAL):
        exact = Decimal(value)
        below = Decimal(_float32_from_bits(bits - 1))
        above = (
            Decimal(_float32_from_bits(bits + 1))
            if bits < _MAX_FLOAT32_BITS
            else exact + (exact - below)
        )
        # Round-half-even: a decimal exactly halfway rounds to an even mantissa.
        inclusive = bits % 2 == 0

        def rounds_to_value(candidate: Decimal) -> bool:
            low, high = 2 * candidate - below - exact, above + exact - 2 * candidate
            return (low > 0 and high > 0) or (inclusive and low >= 0 and high >= 0)

        for precision in range(1, 10):
            quantum = Decimal(1).scaleb(exact.adjusted() - precision + 1)
            candidates = [
                candidate
                for candidate in (
                    exact.quantize(quantum, rounding=ROUND_FLOOR),
                    exact.quantize(quantum, rounding=ROUND_CEILING),
                )
                if rounds_to_value(candidate)
            ]
            if candidates:
                best = min(
                    candidates,
                    key=lambda c: (abs(c - exact), c.as_tuple().digits[-1] % 2),
                )
                return _decimal_digits(best)
    raise AssertionError(f"no float32 decimal of 9 digits or fewer for {value!r}")


def _format_like_fmt(digits: str, exponent: int) -> str:
    if -4 <= exponent < 16:
        if exponent < 0:
            return "0." + "0" * (-exponent - 1) + digits
        integer = digits[: exponent + 1].ljust(exponent + 1, "0")
        fraction = digits[exponent + 1 :]
        return f"{integer}.{fraction}" if fraction else integer
    mantissa = digits[0] + (f".{digits[1:]}" if len(digits) > 1 else "")
    return f"{mantissa}e{'-' if exponent < 0 else '+'}{abs(exponent):02d}"


def _json_floating(value: float, shortest: Callable[[float], tuple[str, int]]) -> str:
    if math.isnan(value):
        return '"NaN"'
    if math.isinf(value):
        return '"Infinity"' if value > 0 else '"-Infinity"'
    sign = "-" if math.copysign(1.0, value) < 0 else ""
    if value == 0.0:
        return f"{sign}0"
    return sign + _format_like_fmt(*shortest(abs(value)))


def json_double(value: float) -> str:
    return _json_floating(value, _shortest_double)


def json_float32(value: float) -> str:
    return _json_floating(value, _shortest_float32)


def json_string(value: str) -> str:
    return json.dumps(value, ensure_ascii=False)


def json_binary(value: bytes) -> str:
    return f'"{base64.b64encode(value).decode("ascii").rstrip("=")}"'


# ---------------------------------------------------------------------------
# SimpleJSON decoding
#
# Accepts what the C++ ``SimpleJSONProtocolReader`` accepts, except that it
# rejects malformed base64 (C++ decodes it to garbage) and non-JSON integers
# such as "+1" or "01". Floating-point literals out of range saturate to
# infinity or zero, as fast_float does in C++.
# ---------------------------------------------------------------------------

_JSON_INT = re.compile(r"-?(?:0|[1-9][0-9]*)")
_JSON_NUMBER = re.compile(r"(-?(?:0|[1-9][0-9]*)(?:\.[0-9]+)?)(?:[eE]([-+]?[0-9]+))?")

_SPECIAL_FLOATS: dict[str, float] = {
    "NaN": math.nan,
    "-NaN": -math.nan,
    "Infinity": math.inf,
    "-Infinity": -math.inf,
}


# The longest literal of an i64, "-9223372036854775808". Longer literals are
# rejected before ``int()``, which refuses more than 4300 digits.
_MAX_INT_LITERAL = 20


def int_from_json(literal: str, bits: int, type_name: str) -> int:
    """The integer a JSON integer literal denotes, range-checked."""
    if _JSON_INT.fullmatch(literal) is None:
        raise DecodeError(f"{literal!r} is not an integer literal for '{type_name}'")
    if len(literal) > _MAX_INT_LITERAL:
        raise DecodeError(f"integer literal out of range for type '{type_name}'")
    value = int(literal)
    if not int_in_range(value, bits):
        raise DecodeError(f"value {value} is out of range for type '{type_name}'")
    return value


def bool_from_json_key(text: str) -> bool:
    if text == "true":
        return True
    if text == "false":
        return False
    raise DecodeError(f"{text!r} is not a bool map key")


def _nearest_float32(value: Decimal) -> float:
    """The float32 nearest to ``value`` (ties to even). Narrowing ``float(value)``
    instead could round twice."""
    magnitude = value.copy_abs()
    if magnitude >= _FLOAT32_OVERFLOW:
        result = math.inf
    else:
        # The double nearest to ``magnitude`` narrows to within one float32 of
        # the nearest float32; comparing with the exact midpoints picks it.
        bits = _float32_bits(min(float(magnitude), _MAX_FLOAT32))
        candidates = [
            b for b in (bits - 1, bits, bits + 1) if 0 <= b <= _MAX_FLOAT32_BITS
        ]
        best = candidates[0]
        with localcontext(_DECIMAL):
            for lower, upper in zip(candidates, candidates[1:]):
                midpoint = (
                    Decimal(_float32_from_bits(lower))
                    + Decimal(_float32_from_bits(upper))
                ) / 2
                if magnitude > midpoint or (magnitude == midpoint and upper % 2 == 0):
                    best = upper
        result = _float32_from_bits(best)
    return -result if value.is_signed() else result


def _json_number(literal: str) -> Decimal:
    """The exact value of a JSON number literal, from the literal so that
    "-0" stays negative and float32 rounds once. ``Decimal`` rejects exponents
    beyond about 10**18, so the exponent is clamped to 400 orders of magnitude
    past the significand's digits, where every float and double has already
    overflowed or underflowed."""
    match = _JSON_NUMBER.fullmatch(literal)
    if match is None:
        raise DecodeError(f"{literal!r} is not a number literal")
    significand, exponent = match.groups()
    if exponent is None:
        return Decimal(significand)
    bound = len(significand) + 400
    digits = exponent.lstrip("+-").lstrip("0")
    # Checked before ``int()``, which refuses more than 4300 digits.
    if len(digits) > len(str(bound)):
        clamped = bound
    else:
        clamped = min(int(digits or "0"), bound)
    sign = "-" if exponent.startswith("-") else ""
    return Decimal(f"{significand}e{sign}{clamped}")


def floating_from_json(literal: str, *, float32: bool) -> float:
    exact = _json_number(literal)
    if float32:
        return _nearest_float32(exact)
    # Correctly rounded, and infinite past the largest double.
    return float(exact)


def special_floating_from_json(text: str) -> float:
    """``"NaN"``, ``"-NaN"``, ``"Infinity"``, or ``"-Infinity"``."""
    special = _SPECIAL_FLOATS.get(text)
    if special is None:
        raise DecodeError(f"unrecognized floating-point value {text!r}")
    return special


def floating_from_json_key(text: str, *, float32: bool) -> float:
    special = _SPECIAL_FLOATS.get(text)
    if special is not None:
        return special
    return floating_from_json(text, float32=float32)


def binary_from_json(text: str) -> bytes:
    unpadded = text.rstrip("=")
    try:
        return base64.b64decode(unpadded + "=" * (-len(unpadded) % 4), validate=True)
    except ValueError as e:  # includes binascii.Error
        raise DecodeError(f"invalid base64 binary value {text!r}") from e
