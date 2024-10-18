#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations


# EXPERIMENTAL - DO NOT USE !!!
# See `experimental_generate_mutable_types` documentation in thrift compiler

#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import typing as _typing

import folly.iobuf as _fbthrift_iobuf
import thrift.python.types as _fbthrift_python_types
import thrift.python.mutable_types as _fbthrift_python_mutable_types
import thrift.python.mutable_exceptions as _fbthrift_python_mutable_exceptions
import thrift.python.mutable_containers as _fbthrift_python_mutable_containers


class _fbthrift_compatible_with_FooStreamEx:
    pass


class FooStreamEx(_fbthrift_python_mutable_exceptions.MutableGeneratedError, _fbthrift_compatible_with_FooStreamEx):
    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    def _to_python(self) -> "module.thrift_types.FooStreamEx": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "module.types.FooStreamEx": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.FooStreamEx": ...  # type: ignore


class _fbthrift_compatible_with_FooEx:
    pass


class FooEx(_fbthrift_python_mutable_exceptions.MutableGeneratedError, _fbthrift_compatible_with_FooEx):
    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    def _to_python(self) -> "module.thrift_types.FooEx": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "module.types.FooEx": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.FooEx": ...  # type: ignore


class _fbthrift_compatible_with_FooEx2:
    pass


class FooEx2(_fbthrift_python_mutable_exceptions.MutableGeneratedError, _fbthrift_compatible_with_FooEx2):
    def __init__(
        self,
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[None]]]: ...
    def _to_python(self) -> "module.thrift_types.FooEx2": ...  # type: ignore
    def _to_mutable_python(self) -> _typing.Self: ...
    def _to_py3(self) -> "module.types.FooEx2": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.FooEx2": ...  # type: ignore


class _fbthrift_PubSubStreamingService_returnstream_args(_fbthrift_python_types.Struct):
    i32_from: _typing.Final[int] = ...
    i32_to: _typing.Final[int] = ...

    def __init__(
        self, *,
        i32_from: _typing.Optional[int]=...,
        i32_to: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int, int]]]: ...


class _fbthrift_PubSubStreamingService_returnstream_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...


class _fbthrift_PubSubStreamingService_returnstream_result_stream(_fbthrift_python_types._fbthrift_ResponseStreamResult[int]):

    def __init__(
        self, *, success: _typing.Optional[int] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
        ]]]: ...


class _fbthrift_PubSubStreamingService_streamthrows_args(_fbthrift_python_types.Struct):
    foo: _typing.Final[int] = ...

    def __init__(
        self, *,
        foo: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_PubSubStreamingService_streamthrows_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...


class _fbthrift_PubSubStreamingService_streamthrows_result_stream(_fbthrift_python_types._fbthrift_ResponseStreamResult[int]):
    e: _typing.Final[FooStreamEx]

    def __init__(
        self, *, success: _typing.Optional[int] = ..., e: _typing.Optional[FooStreamEx]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
            FooStreamEx,
        ]]]: ...


class _fbthrift_PubSubStreamingService_servicethrows_args(_fbthrift_python_types.Struct):
    foo: _typing.Final[int] = ...

    def __init__(
        self, *,
        foo: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_PubSubStreamingService_servicethrows_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]
    e: _typing.Final[FooEx]

    def __init__(
        self, *, success: _typing.Optional[None] = ..., e: _typing.Optional[FooEx]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
            FooEx,
        ]]]: ...


class _fbthrift_PubSubStreamingService_servicethrows_result_stream(_fbthrift_python_types._fbthrift_ResponseStreamResult[int]):

    def __init__(
        self, *, success: _typing.Optional[int] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
        ]]]: ...


class _fbthrift_PubSubStreamingService_servicethrows2_args(_fbthrift_python_types.Struct):
    foo: _typing.Final[int] = ...

    def __init__(
        self, *,
        foo: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_PubSubStreamingService_servicethrows2_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]
    e1: _typing.Final[FooEx]
    e2: _typing.Final[FooEx2]

    def __init__(
        self, *, success: _typing.Optional[None] = ..., e1: _typing.Optional[FooEx]=..., e2: _typing.Optional[FooEx2]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
            FooEx,
            FooEx2,
        ]]]: ...


class _fbthrift_PubSubStreamingService_servicethrows2_result_stream(_fbthrift_python_types._fbthrift_ResponseStreamResult[int]):

    def __init__(
        self, *, success: _typing.Optional[int] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
        ]]]: ...


class _fbthrift_PubSubStreamingService_boththrows_args(_fbthrift_python_types.Struct):
    foo: _typing.Final[int] = ...

    def __init__(
        self, *,
        foo: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_PubSubStreamingService_boththrows_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]
    e: _typing.Final[FooEx]

    def __init__(
        self, *, success: _typing.Optional[None] = ..., e: _typing.Optional[FooEx]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
            FooEx,
        ]]]: ...


class _fbthrift_PubSubStreamingService_boththrows_result_stream(_fbthrift_python_types._fbthrift_ResponseStreamResult[int]):
    e: _typing.Final[FooStreamEx]

    def __init__(
        self, *, success: _typing.Optional[int] = ..., e: _typing.Optional[FooStreamEx]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
            FooStreamEx,
        ]]]: ...


class _fbthrift_PubSubStreamingService_responseandstreamstreamthrows_args(_fbthrift_python_types.Struct):
    foo: _typing.Final[int] = ...

    def __init__(
        self, *,
        foo: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_PubSubStreamingService_responseandstreamstreamthrows_result(_fbthrift_python_types.Struct):
    success: _typing.Final[int]

    def __init__(
        self, *, success: _typing.Optional[int] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
        ]]]: ...


class _fbthrift_PubSubStreamingService_responseandstreamstreamthrows_result_stream(_fbthrift_python_types._fbthrift_ResponseStreamResult[int]):
    e: _typing.Final[FooStreamEx]

    def __init__(
        self, *, success: _typing.Optional[int] = ..., e: _typing.Optional[FooStreamEx]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
            FooStreamEx,
        ]]]: ...


class _fbthrift_PubSubStreamingService_responseandstreamservicethrows_args(_fbthrift_python_types.Struct):
    foo: _typing.Final[int] = ...

    def __init__(
        self, *,
        foo: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_PubSubStreamingService_responseandstreamservicethrows_result(_fbthrift_python_types.Struct):
    success: _typing.Final[int]
    e: _typing.Final[FooEx]

    def __init__(
        self, *, success: _typing.Optional[int] = ..., e: _typing.Optional[FooEx]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
            FooEx,
        ]]]: ...


class _fbthrift_PubSubStreamingService_responseandstreamservicethrows_result_stream(_fbthrift_python_types._fbthrift_ResponseStreamResult[int]):

    def __init__(
        self, *, success: _typing.Optional[int] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
        ]]]: ...


class _fbthrift_PubSubStreamingService_responseandstreamboththrows_args(_fbthrift_python_types.Struct):
    foo: _typing.Final[int] = ...

    def __init__(
        self, *,
        foo: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int]]]: ...


class _fbthrift_PubSubStreamingService_responseandstreamboththrows_result(_fbthrift_python_types.Struct):
    success: _typing.Final[int]
    e: _typing.Final[FooEx]

    def __init__(
        self, *, success: _typing.Optional[int] = ..., e: _typing.Optional[FooEx]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
            FooEx,
        ]]]: ...


class _fbthrift_PubSubStreamingService_responseandstreamboththrows_result_stream(_fbthrift_python_types._fbthrift_ResponseStreamResult[int]):
    e: _typing.Final[FooStreamEx]

    def __init__(
        self, *, success: _typing.Optional[int] = ..., e: _typing.Optional[FooStreamEx]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
            FooStreamEx,
        ]]]: ...


class _fbthrift_PubSubStreamingService_returnstreamFast_args(_fbthrift_python_types.Struct):
    i32_from: _typing.Final[int] = ...
    i32_to: _typing.Final[int] = ...

    def __init__(
        self, *,
        i32_from: _typing.Optional[int]=...,
        i32_to: _typing.Optional[int]=...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[None, int, int]]]: ...


class _fbthrift_PubSubStreamingService_returnstreamFast_result(_fbthrift_python_types.Struct):
    success: _typing.Final[None]

    def __init__(
        self, *, success: _typing.Optional[None] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            None,
        ]]]: ...


class _fbthrift_PubSubStreamingService_returnstreamFast_result_stream(_fbthrift_python_types._fbthrift_ResponseStreamResult[int]):

    def __init__(
        self, *, success: _typing.Optional[int] = ...
    ) -> None: ...

    def __iter__(self) -> _typing.Iterator[_typing.Tuple[
        str,
        _typing.Union[
            int,
        ]]]: ...
