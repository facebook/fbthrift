#
# Autogenerated by Thrift
#
# DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
#  @generated
#

import folly.iobuf as _fbthrift_iobuf
import typing as _typing
from thrift.py3.server import RequestContext, ServiceInterface
from abc import abstractmethod, ABCMeta

import module.types as _module_types

_MyServiceInterfaceT = _typing.TypeVar('_MyServiceInterfaceT', bound='MyServiceInterface')


class MyServiceInterface(
    ServiceInterface,
    metaclass=ABCMeta,
):

    @staticmethod
    def pass_context_hasDataById(
        fn: _typing.Callable[
                [_MyServiceInterfaceT, RequestContext, int],
                _typing.Coroutine[_typing.Any, _typing.Any, bool]
        ]
    ) -> _typing.Callable[
        [_MyServiceInterfaceT, int],
        _typing.Coroutine[_typing.Any, _typing.Any, bool]
    ]: ...

    @abstractmethod
    async def hasDataById(
        self,
        id: int
    ) -> bool: ...

    @staticmethod
    def pass_context_getDataById(
        fn: _typing.Callable[
                [_MyServiceInterfaceT, RequestContext, int],
                _typing.Coroutine[_typing.Any, _typing.Any, str]
        ]
    ) -> _typing.Callable[
        [_MyServiceInterfaceT, int],
        _typing.Coroutine[_typing.Any, _typing.Any, str]
    ]: ...

    @abstractmethod
    async def getDataById(
        self,
        id: int
    ) -> str: ...

    @staticmethod
    def pass_context_putDataById(
        fn: _typing.Callable[
                [_MyServiceInterfaceT, RequestContext, int, str],
                _typing.Coroutine[_typing.Any, _typing.Any, None]
        ]
    ) -> _typing.Callable[
        [_MyServiceInterfaceT, int, str],
        _typing.Coroutine[_typing.Any, _typing.Any, None]
    ]: ...

    @abstractmethod
    async def putDataById(
        self,
        id: int,
        data: str
    ) -> None: ...

    @staticmethod
    def pass_context_lobDataById(
        fn: _typing.Callable[
                [_MyServiceInterfaceT, RequestContext, int, str],
                _typing.Coroutine[_typing.Any, _typing.Any, None]
        ]
    ) -> _typing.Callable[
        [_MyServiceInterfaceT, int, str],
        _typing.Coroutine[_typing.Any, _typing.Any, None]
    ]: ...

    @abstractmethod
    async def lobDataById(
        self,
        id: int,
        data: str
    ) -> None: ...
    pass


_MyServiceFastInterfaceT = _typing.TypeVar('_MyServiceFastInterfaceT', bound='MyServiceFastInterface')


class MyServiceFastInterface(
    ServiceInterface,
    metaclass=ABCMeta,
):

    @staticmethod
    def pass_context_hasDataById(
        fn: _typing.Callable[
                [_MyServiceFastInterfaceT, RequestContext, int],
                _typing.Coroutine[_typing.Any, _typing.Any, bool]
        ]
    ) -> _typing.Callable[
        [_MyServiceFastInterfaceT, int],
        _typing.Coroutine[_typing.Any, _typing.Any, bool]
    ]: ...

    @abstractmethod
    async def hasDataById(
        self,
        id: int
    ) -> bool: ...

    @staticmethod
    def pass_context_getDataById(
        fn: _typing.Callable[
                [_MyServiceFastInterfaceT, RequestContext, int],
                _typing.Coroutine[_typing.Any, _typing.Any, str]
        ]
    ) -> _typing.Callable[
        [_MyServiceFastInterfaceT, int],
        _typing.Coroutine[_typing.Any, _typing.Any, str]
    ]: ...

    @abstractmethod
    async def getDataById(
        self,
        id: int
    ) -> str: ...

    @staticmethod
    def pass_context_putDataById(
        fn: _typing.Callable[
                [_MyServiceFastInterfaceT, RequestContext, int, str],
                _typing.Coroutine[_typing.Any, _typing.Any, None]
        ]
    ) -> _typing.Callable[
        [_MyServiceFastInterfaceT, int, str],
        _typing.Coroutine[_typing.Any, _typing.Any, None]
    ]: ...

    @abstractmethod
    async def putDataById(
        self,
        id: int,
        data: str
    ) -> None: ...

    @staticmethod
    def pass_context_lobDataById(
        fn: _typing.Callable[
                [_MyServiceFastInterfaceT, RequestContext, int, str],
                _typing.Coroutine[_typing.Any, _typing.Any, None]
        ]
    ) -> _typing.Callable[
        [_MyServiceFastInterfaceT, int, str],
        _typing.Coroutine[_typing.Any, _typing.Any, None]
    ]: ...

    @abstractmethod
    async def lobDataById(
        self,
        id: int,
        data: str
    ) -> None: ...
    pass


_DbMixedStackArgumentsInterfaceT = _typing.TypeVar('_DbMixedStackArgumentsInterfaceT', bound='DbMixedStackArgumentsInterface')


class DbMixedStackArgumentsInterface(
    ServiceInterface,
    metaclass=ABCMeta,
):

    @staticmethod
    def pass_context_getDataByKey0(
        fn: _typing.Callable[
                [_DbMixedStackArgumentsInterfaceT, RequestContext, str],
                _typing.Coroutine[_typing.Any, _typing.Any, bytes]
        ]
    ) -> _typing.Callable[
        [_DbMixedStackArgumentsInterfaceT, str],
        _typing.Coroutine[_typing.Any, _typing.Any, bytes]
    ]: ...

    @abstractmethod
    async def getDataByKey0(
        self,
        key: str
    ) -> bytes: ...

    @staticmethod
    def pass_context_getDataByKey1(
        fn: _typing.Callable[
                [_DbMixedStackArgumentsInterfaceT, RequestContext, str],
                _typing.Coroutine[_typing.Any, _typing.Any, bytes]
        ]
    ) -> _typing.Callable[
        [_DbMixedStackArgumentsInterfaceT, str],
        _typing.Coroutine[_typing.Any, _typing.Any, bytes]
    ]: ...

    @abstractmethod
    async def getDataByKey1(
        self,
        key: str
    ) -> bytes: ...
    pass


