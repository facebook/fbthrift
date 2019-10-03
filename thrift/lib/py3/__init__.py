#!/usr/bin/env python3
# See __init__.pyi for easier to digest types for typecheckers

__all__ = []

try:
    from thrift.py3.client import get_client, Client  # noqa: 401

    __all__.extend(["Client", "get_client"])
except ModuleNotFoundError:
    pass

try:
    from thrift.py3.server import (  # noqa: 401
        ThriftServer,
        SSLPolicy,
        get_context,
        pass_context,
        RequestContext,
    )

    __all__.extend(
        ["ThriftServer", "get_context", "pass_context", "SSLPolicy", "RequestContext"]
    )
except ModuleNotFoundError:
    pass

try:
    from thrift.py3.types import Struct, BadEnum, Union, Enum, Flag  # noqa: 401

    __all__.extend(["Struct", "BadEnum", "Union", "Enum", "Flag"])
except ModuleNotFoundError:
    pass

try:
    from thrift.py3.exceptions import (  # noqa: 401
        Error,
        ApplicationError,
        TransportError,
        ProtocolError,
    )

    __all__.extend(["Error", "ApplicationError", "TransportError", "ProtocolError"])
except ModuleNotFoundError:
    pass

try:
    from thrift.py3.serializer import Protocol, serialize, deserialize  # noqa: 401

    __all__.extend(["Protocol", "serialize", "deserialize"])
except ModuleNotFoundError:
    pass

try:
    from thrift.py3.common import Priority, RpcOptions  # noqa: 401

    __all__.extend(["Priority", "RpcOptions"])
except ModuleNotFoundError:
    pass
