#!/usr/bin/env python3
from enum import Enum
from typing import TypeVar, Type, Union

from thrift.py3.types import Struct
from folly.iobuf import IOBuf

sT = TypeVar('sT', bound=Struct)


class Protocol(Enum):
    COMPACT: Protocol = ...
    BINARY: Protocol = ...
    JSON: Protocol = ...
    COMPACT_JSON: Protocol = ...
    value: int


def serialize(tstruct: sT, protocol: Protocol = ...) -> bytes: ...
def serialize_iobuf(tstruct: sT, protocol: Protocol = ...) -> IOBuf: ...


def deserialize(
    structKlass: Type[sT],
    buf: Union[bytes, bytearray, IOBuf, memoryview],
    protocol: Protocol = ...
) -> sT: ...
