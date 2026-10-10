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

# TypeRefs, generated fixtures, and conversion to the datum representation.

from __future__ import annotations

from apache.thrift.type.any_rep.thrift_types import AnyStruct
from apache.thrift.type.standard.thrift_types import StandardProtocol, TypeName, Void
from apache.thrift.type.type_rep.thrift_types import ProtocolUnion, TypeStruct
from folly.iobuf import IOBuf
from thrift.lib.python.schema.type_system import Primitive, PrimitiveTypeRef

BOOL = PrimitiveTypeRef(Primitive.BOOL)
BYTE = PrimitiveTypeRef(Primitive.BYTE)
I16 = PrimitiveTypeRef(Primitive.I16)
I32 = PrimitiveTypeRef(Primitive.I32)
I64 = PrimitiveTypeRef(Primitive.I64)
FLOAT = PrimitiveTypeRef(Primitive.FLOAT)
DOUBLE = PrimitiveTypeRef(Primitive.DOUBLE)
STRING = PrimitiveTypeRef(Primitive.STRING)
BINARY = PrimitiveTypeRef(Primitive.BINARY)
ANY = PrimitiveTypeRef(Primitive.ANY)


def any_of_i32(value: int) -> AnyStruct:
    """An `AnyStruct` holding a Compact-encoded i32."""
    zigzag = (value << 1) ^ (value >> 31)
    data = bytearray()
    while zigzag > 0x7F:
        data.append((zigzag & 0x7F) | 0x80)
        zigzag >>= 7
    data.append(zigzag)
    return AnyStruct(
        type=TypeStruct(name=TypeName(i32Type=Void.Unused)),
        protocol=ProtocolUnion(standard=StandardProtocol.Compact),
        data=IOBuf(bytes(data)),
    )
