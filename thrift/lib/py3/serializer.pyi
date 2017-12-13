#!/usr/bin/env python3
from enum import Enum
from typing import TypeVar, Type

from thrift.py3.types import Struct

sT = TypeVar('sT', bound=Struct)


class Protocol(Enum):
    COMPACT = ...
    BINARY = ...
    JSON = ...
    value: int


def serialize(tstruct: sT, protocol: Protocol = ...) -> bytes: ...


def deserialize(
    structKlass: Type[sT],
    buf: bytes,
    protocol: Protocol = ...
) -> sT: ...
