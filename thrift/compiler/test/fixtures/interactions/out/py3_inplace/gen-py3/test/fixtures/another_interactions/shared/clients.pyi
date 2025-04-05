#
# Autogenerated by Thrift for shared.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

import folly.iobuf as _fbthrift_iobuf
import thrift.py3.types
import thrift.py3.client
import thrift.py3.common
import typing as _typing
from types import TracebackType

import test.fixtures.another_interactions.shared.types as _test_fixtures_another_interactions_shared_types


_InteractLocallyT = _typing.TypeVar('_InteractLocallyT', bound='InteractLocally')


class InteractLocally(thrift.py3.client.Client):

    def createSharedInteraction(self) -> InteractLocally_SharedInteraction: ...
    def async_createSharedInteraction(self) -> InteractLocally_SharedInteraction: ...

_InteractLocally_SharedInteraction = _typing.TypeVar('_InteractLocally_SharedInteraction', bound='InteractLocally_SharedInteraction')


class InteractLocally_SharedInteraction(thrift.py3.client.Client):

    async def init(
        self,
        *,
        rpc_options: _typing.Optional[thrift.py3.common.RpcOptions]=None
    ) -> int: ...

    async def do_something(
        self,
        *,
        rpc_options: _typing.Optional[thrift.py3.common.RpcOptions]=None
    ) -> _test_fixtures_another_interactions_shared_types.DoSomethingResult: ...

    async def tear_down(
        self,
        *,
        rpc_options: _typing.Optional[thrift.py3.common.RpcOptions]=None
    ) -> None: ...

