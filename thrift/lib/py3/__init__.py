#!/usr/bin/env python3
__all__ = [
    'get_client', 'Client', 'ThriftServer', 'Struct', 'BadEnum', 'Error',
    'ApplicationError', 'TransportError', 'SSLPolicy',
]

try:
    from thrift.py3.client import get_client, Client
except ImportError:
    __all__.remove('Client')
    __all__.remove('get_client')

try:
    from thrift.py3.server import ThriftServer, SSLPolicy
except ImportError:
    __all__.remove('ThriftServer')
    __all__.remove('SSLPolicy')

try:
    from thrift.py3.types import Struct, BadEnum
except ImportError:
    __all__.remove('Struct')
    __all__.remove('BadEnum')

try:
    from thrift.py3.exceptions import Error, ApplicationError, TransportError
except ImportError:
    __all__.remove('Error')
    __all__.remove('ApplicationError')
    __all__.remove('TransportError')
