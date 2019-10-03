#!/usr/bin/env python3
from typing import List

__all__ = []  # type: List[str]

try:
    from thrift.py3.client import get_client, Client
    __all__.extend(['Client', 'get_client'])
except ModuleNotFoundError:
    pass

try:
    from thrift.py3.server import (  # noqa: 401
        ThriftServer, SSLPolicy, pass_context, RequestContext, get_context
    )
    __all__.extend(
        ['ThriftServer', 'get_context', 'pass_context', 'SSLPolicy', 'RequestContext']
    )
except ModuleNotFoundError:
    pass

try:
    from thrift.py3.types import Struct, BadEnum, Union, Enum, Flag
    __all__.extend(['Struct', 'BadEnum', 'Union', 'Enum', 'Flag'])
except ModuleNotFoundError:
    pass

try:
    from thrift.py3.exceptions import Error, ApplicationError, TransportError, ProtocolError
    __all__.extend(['Error', 'ApplicationError', 'TransportError', 'ProtocolError'])
except ModuleNotFoundError:
    pass

try:
    from thrift.py3.serializer import Protocol, serialize, deserialize
    __all__.extend(['Protocol', 'serialize', 'deserialize'])
except ModuleNotFoundError:
    pass

try:
    from thrift.py3.common import Priority, RpcOptions
    __all__.extend(['Priority', 'RpcOptions'])
except ModuleNotFoundError:
    pass
