# Copyright (c) Facebook, Inc. and its affiliates.
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

from thrift.py3.types cimport Struct
from folly.iobuf cimport IOBuf
cimport folly.iobuf as _iobuf
from thrift.py3.common import Protocol
from thrift.py3.exceptions import Error


def serialize(tstruct, protocol=Protocol.COMPACT):
    return b''.join(serialize_iobuf(tstruct, protocol))


def serialize_iobuf(Struct tstruct not None, protocol=Protocol.COMPACT):
    if not isinstance(protocol, Protocol):
        raise TypeError(f"{protocol} must of type Protocol")
    cdef Struct cy_struct = <Struct> tstruct
    return cy_struct._serialize(protocol)


def deserialize_with_length(structKlass, buf not None, protocol=Protocol.COMPACT):
    if not issubclass(structKlass, Struct):
        raise TypeError(f"{structKlass} Must be a py3 thrift struct class")
    if not isinstance(protocol, Protocol):
        raise TypeError(f"{protocol} must of type Protocol")
    cdef Struct cy_struct = <Struct> structKlass.__new__(structKlass)

    cdef IOBuf iobuf = buf if isinstance(buf, IOBuf) else IOBuf(buf)
    try:
        length = cy_struct._deserialize(iobuf._this, protocol)
        return cy_struct, length
    except Exception as e:
        raise Error.__new__(Error, *e.args) from None

def deserialize(structKlass, buf not None, protocol=Protocol.COMPACT):
    return deserialize_with_length(structKlass, buf, protocol)[0]

def serialize_with_header(tstruct, protocol=Protocol.COMPACT, transform=Transform.NONE):
    return b''.join(serialize_with_header_iobuf(tstruct, protocol, transform))

def serialize_with_header_iobuf(Struct tstruct not None, protocol=Protocol.COMPACT, Transform transform=Transform.NONE):
    cdef cTHeader header
    cdef map[string, string] pheaders
    cdef IOBuf buf = <IOBuf>serialize_iobuf(tstruct, protocol)
    header.setProtocolId(protocol)
    if transform is not Transform.NONE:
        header.setTransform(transform)
    return _iobuf.from_unique_ptr(header.addHeader(_iobuf.move(buf._ours), pheaders))


def deserialize_from_header(structKlass, buf not None):
    # Clone because we will take the guts.
    cdef IOBuf iobuf = buf.clone() if isinstance(buf, IOBuf) else IOBuf(buf)
    cdef _iobuf.cIOBufQueue queue = _iobuf.cIOBufQueue(_iobuf.cacheChainLength())
    queue.append(_iobuf.move(iobuf._ours))
    cdef cTHeader header
    cdef map[string, string] pheaders
    cdef size_t needed = 0
    cbuf = header.removeHeader(&queue, needed, pheaders)
    protoid = Protocol(header.getProtocolId())
    return deserialize(structKlass, _iobuf.from_unique_ptr(_iobuf.move(cbuf)), protocol=protoid)
