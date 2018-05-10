#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from typing import Any, AnyStr, TypeVar  # noqa: F401

from thrift.transport import TTransport
from thrift.protocol import THeaderProtocol


def serialize(protocol_factory, thr):
    # type: (Any, Any) -> AnyStr
    """Convenience method for serializing objects using the given
    protocol factory and a TMemoryBuffer."""
    transport = TTransport.TMemoryBuffer()
    protocol = protocol_factory.getProtocol(transport)
    thr.write(protocol)
    if isinstance(protocol, THeaderProtocol.THeaderProtocol):
        protocol.trans.flush()
    return transport.getvalue()


T = TypeVar("T")  # noqa: F401


def deserialize(protocol_factory, data, thr_out):
    # type: (Any, AnyStr, T) -> T
    """Convenience method for deserializing objects using the given
    protocol factory and a TMemoryBuffer.  returns its thr_out
    argument."""
    transport = TTransport.TMemoryBuffer(data)
    try:
        protocol = protocol_factory.getProtocol(transport, thr_out.thrift_spec)  # noqa: T484
    except TypeError:
        protocol = protocol_factory.getProtocol(transport)
    if isinstance(protocol, THeaderProtocol.THeaderProtocol):
        # this reads the THeader headers to detect what the underlying
        # protocol is, as well as looking at transforms, etc.
        protocol.trans.readFrame(0)
        protocol.reset_protocol()
    thr_out.read(protocol)  # noqa: T484
    return thr_out
