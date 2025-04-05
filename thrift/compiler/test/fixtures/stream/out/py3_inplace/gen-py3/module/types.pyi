#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/stream/src/module.thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

import enum as _python_std_enum
import folly.iobuf as _fbthrift_iobuf
import thrift.py3.types
import thrift.python.types
import thrift.py3.exceptions
import typing as _typing

import sys
import itertools
import thrift.py3.stream
import module.thrift_types


class FooStreamEx(thrift.py3.exceptions.GeneratedError, _typing.Hashable):
    class __fbthrift_IsSet:
        pass


    def __init__(
        self, 
    ) -> None: ...

    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'FooStreamEx') -> bool: ...
    def __gt__(self, other: 'FooStreamEx') -> bool: ...
    def __le__(self, other: 'FooStreamEx') -> bool: ...
    def __ge__(self, other: 'FooStreamEx') -> bool: ...

    def _to_python(self) -> module.thrift_types.FooStreamEx: ...
    def _to_py3(self) -> FooStreamEx: ...
    def _to_py_deprecated(self) -> "module.ttypes.FooStreamEx": ...   # type: ignore

class FooEx(thrift.py3.exceptions.GeneratedError, _typing.Hashable):
    class __fbthrift_IsSet:
        pass


    def __init__(
        self, 
    ) -> None: ...

    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'FooEx') -> bool: ...
    def __gt__(self, other: 'FooEx') -> bool: ...
    def __le__(self, other: 'FooEx') -> bool: ...
    def __ge__(self, other: 'FooEx') -> bool: ...

    def _to_python(self) -> module.thrift_types.FooEx: ...
    def _to_py3(self) -> FooEx: ...
    def _to_py_deprecated(self) -> "module.ttypes.FooEx": ...   # type: ignore

class FooEx2(thrift.py3.exceptions.GeneratedError, _typing.Hashable):
    class __fbthrift_IsSet:
        pass


    def __init__(
        self, 
    ) -> None: ...

    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'FooEx2') -> bool: ...
    def __gt__(self, other: 'FooEx2') -> bool: ...
    def __le__(self, other: 'FooEx2') -> bool: ...
    def __ge__(self, other: 'FooEx2') -> bool: ...

    def _to_python(self) -> module.thrift_types.FooEx2: ...
    def _to_py3(self) -> FooEx2: ...
    def _to_py_deprecated(self) -> "module.ttypes.FooEx2": ...   # type: ignore


class ClientBufferedStream__i32(thrift.py3.stream.ClientBufferedStream[int]):
    def __aiter__(self) -> _typing.AsyncIterator[int]: ...
    async def __anext__(self) -> int: ...

class ServerStream__i32(thrift.py3.stream.ServerStream[int]):
    pass

class ServerPublisher_cint32_t(thrift.py3.stream.ServerPublisher):
    def complete(self) -> None: ...
    def send(self, item: int) -> None: ...

class ResponseAndClientBufferedStream__i32_i32(thrift.py3.stream.ResponseAndClientBufferedStream[int, int]):
    def __iter__(self) -> _typing.Tuple[
        int,
        ClientBufferedStream__i32,
    ]: ...

class ResponseAndServerStream__i32_i32(thrift.py3.stream.ResponseAndServerStream[int, int]):
    pass

