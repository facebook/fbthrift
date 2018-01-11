#!/usr/bin/env python3

import ipaddress
from enum import Enum
from typing import Callable, NamedTuple, Union, Optional, TypeVar, Mapping, ClassVar

mT = TypeVar('mT', bound=Callable)


class SocketAddress(NamedTuple):
    ip: Optional[Union[ipaddress.IPv4Address, ipaddress.IPv6Address]]
    port: Optional[int]
    path: Optional[bytes]


def pass_context(func: mT) -> mT: ...


class SSLPolicy(Enum):
    DISABLED: int
    PERMITTED: int
    REQUIRED: int
    value: int


class ServiceInterface:
    annotations: ClassVar[Mapping[str, str]] = ...


hT = TypeVar('hT', bound=ServiceInterface)


class ThriftServer:
    def __init__(self, handler: hT, port: int) -> None: ...
    async def serve(self) -> None: ...
    def set_ssl_policy(self, policy: SSLPolicy) -> None: ...
    def stop(self) -> None: ...
    async def get_address(self) -> SocketAddress: ...


class ConnectionContext:
    peer_address: SocketAddress
    is_tls: bool
    peer_common_name: str
    peer_certificate: bytes


class RequestContext:
    connection_context: ConnectionContext
