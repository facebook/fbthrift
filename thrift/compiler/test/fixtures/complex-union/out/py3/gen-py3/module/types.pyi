#
# Autogenerated by Thrift for thrift/compiler/test/fixtures/complex-union/src/module.thrift
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
import module.thrift_types



_ComplexUnionValueType = _typing.Union[None, int, str, _typing.Sequence[int], _typing.Sequence[str], _typing.Mapping[int, str], str]

class ComplexUnion(thrift.py3.types.Union, _typing.Hashable):
    class __fbthrift_IsSet:
        intValue: bool
        stringValue: bool
        intListValue: bool
        stringListValue: bool
        typedefValue: bool
        pass

    intValue: _typing.Final[int] = ...
    stringValue: _typing.Final[str] = ...
    intListValue: _typing.Final[_typing.Sequence[int]] = ...
    stringListValue: _typing.Final[_typing.Sequence[str]] = ...
    typedefValue: _typing.Final[_typing.Mapping[int, str]] = ...
    stringRef: _typing.Final[_typing.Optional[str]] = ...

    def __init__(
        self, *,
        intValue: _typing.Optional[int]=None,
        stringValue: _typing.Optional[str]=None,
        intListValue: _typing.Optional[_typing.Sequence[int]]=None,
        stringListValue: _typing.Optional[_typing.Sequence[str]]=None,
        typedefValue: _typing.Optional[_typing.Mapping[int, str]]=None,
        stringRef: _typing.Optional[str]=None
    ) -> None: ...

    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'ComplexUnion') -> bool: ...
    def __gt__(self, other: 'ComplexUnion') -> bool: ...
    def __le__(self, other: 'ComplexUnion') -> bool: ...
    def __ge__(self, other: 'ComplexUnion') -> bool: ...

    class Type(_python_std_enum.Enum):
        EMPTY: ComplexUnion.Type = ...
        intValue: ComplexUnion.Type = ...
        stringValue: ComplexUnion.Type = ...
        intListValue: ComplexUnion.Type = ...
        stringListValue: ComplexUnion.Type = ...
        typedefValue: ComplexUnion.Type = ...
        stringRef: ComplexUnion.Type = ...

    @staticmethod
    def fromValue(value: _ComplexUnionValueType) -> ComplexUnion: ...
    type: _typing.Final[ComplexUnion.Type]
    value: _typing.Final[_ComplexUnionValueType]
    def get_type(self) -> ComplexUnion.Type: ...

    def _to_python(self) -> module.thrift_types.ComplexUnion: ...
    def _to_py3(self) -> ComplexUnion: ...
    def _to_py_deprecated(self) -> "module.ttypes.ComplexUnion": ...   # type: ignore

_ListUnionValueType = _typing.Union[None, _typing.Sequence[int], _typing.Sequence[str]]

class ListUnion(thrift.py3.types.Union, _typing.Hashable):
    class __fbthrift_IsSet:
        intListValue: bool
        stringListValue: bool
        pass

    intListValue: _typing.Final[_typing.Sequence[int]] = ...
    stringListValue: _typing.Final[_typing.Sequence[str]] = ...

    def __init__(
        self, *,
        intListValue: _typing.Optional[_typing.Sequence[int]]=None,
        stringListValue: _typing.Optional[_typing.Sequence[str]]=None
    ) -> None: ...

    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'ListUnion') -> bool: ...
    def __gt__(self, other: 'ListUnion') -> bool: ...
    def __le__(self, other: 'ListUnion') -> bool: ...
    def __ge__(self, other: 'ListUnion') -> bool: ...

    class Type(_python_std_enum.Enum):
        EMPTY: ListUnion.Type = ...
        intListValue: ListUnion.Type = ...
        stringListValue: ListUnion.Type = ...

    @staticmethod
    def fromValue(value: _ListUnionValueType) -> ListUnion: ...
    type: _typing.Final[ListUnion.Type]
    value: _typing.Final[_ListUnionValueType]
    def get_type(self) -> ListUnion.Type: ...

    def _to_python(self) -> module.thrift_types.ListUnion: ...
    def _to_py3(self) -> ListUnion: ...
    def _to_py_deprecated(self) -> "module.ttypes.ListUnion": ...   # type: ignore

_DataUnionValueType = _typing.Union[None, bytes, str]

class DataUnion(thrift.py3.types.Union, _typing.Hashable):
    class __fbthrift_IsSet:
        binaryData: bool
        stringData: bool
        pass

    binaryData: _typing.Final[bytes] = ...
    stringData: _typing.Final[str] = ...

    def __init__(
        self, *,
        binaryData: _typing.Optional[bytes]=None,
        stringData: _typing.Optional[str]=None
    ) -> None: ...

    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'DataUnion') -> bool: ...
    def __gt__(self, other: 'DataUnion') -> bool: ...
    def __le__(self, other: 'DataUnion') -> bool: ...
    def __ge__(self, other: 'DataUnion') -> bool: ...

    class Type(_python_std_enum.Enum):
        EMPTY: DataUnion.Type = ...
        binaryData: DataUnion.Type = ...
        stringData: DataUnion.Type = ...

    @staticmethod
    def fromValue(value: _DataUnionValueType) -> DataUnion: ...
    type: _typing.Final[DataUnion.Type]
    value: _typing.Final[_DataUnionValueType]
    def get_type(self) -> DataUnion.Type: ...

    def _to_python(self) -> module.thrift_types.DataUnion: ...
    def _to_py3(self) -> DataUnion: ...
    def _to_py_deprecated(self) -> "module.ttypes.DataUnion": ...   # type: ignore

class Val(thrift.py3.types.Struct, _typing.Hashable):
    class __fbthrift_IsSet:
        strVal: bool
        intVal: bool
        typedefValue: bool
        pass

    strVal: _typing.Final[str] = ...
    intVal: _typing.Final[int] = ...
    typedefValue: _typing.Final[_typing.Mapping[int, str]] = ...

    def __init__(
        self, *,
        strVal: _typing.Optional[str]=None,
        intVal: _typing.Optional[int]=None,
        typedefValue: _typing.Optional[_typing.Mapping[int, str]]=None
    ) -> None: ...

    def __call__(
        self, *,
        strVal: _typing.Union[str, None]=None,
        intVal: _typing.Union[int, None]=None,
        typedefValue: _typing.Union[_typing.Mapping[int, str], None]=None
    ) -> Val: ...

    def __reduce__(self) -> _typing.Tuple[_typing.Callable, _typing.Tuple[_typing.Type['Val'], bytes]]: ...
    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'Val') -> bool: ...
    def __gt__(self, other: 'Val') -> bool: ...
    def __le__(self, other: 'Val') -> bool: ...
    def __ge__(self, other: 'Val') -> bool: ...

    def _to_python(self) -> module.thrift_types.Val: ...
    def _to_py3(self) -> Val: ...
    def _to_py_deprecated(self) -> "module.ttypes.Val": ...   # type: ignore

_ValUnionValueType = _typing.Union[None, Val, Val]

class ValUnion(thrift.py3.types.Union, _typing.Hashable):
    class __fbthrift_IsSet:
        v1: bool
        v2: bool
        pass

    v1: _typing.Final[Val] = ...
    v2: _typing.Final[Val] = ...

    def __init__(
        self, *,
        v1: _typing.Optional[Val]=None,
        v2: _typing.Optional[Val]=None
    ) -> None: ...

    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'ValUnion') -> bool: ...
    def __gt__(self, other: 'ValUnion') -> bool: ...
    def __le__(self, other: 'ValUnion') -> bool: ...
    def __ge__(self, other: 'ValUnion') -> bool: ...

    class Type(_python_std_enum.Enum):
        EMPTY: ValUnion.Type = ...
        v1: ValUnion.Type = ...
        v2: ValUnion.Type = ...

    @staticmethod
    def fromValue(value: _ValUnionValueType) -> ValUnion: ...
    type: _typing.Final[ValUnion.Type]
    value: _typing.Final[_ValUnionValueType]
    def get_type(self) -> ValUnion.Type: ...

    def _to_python(self) -> module.thrift_types.ValUnion: ...
    def _to_py3(self) -> ValUnion: ...
    def _to_py_deprecated(self) -> "module.ttypes.ValUnion": ...   # type: ignore

_VirtualComplexUnionValueType = _typing.Union[None, str, str]

class VirtualComplexUnion(thrift.py3.types.Union, _typing.Hashable):
    class __fbthrift_IsSet:
        thingOne: bool
        thingTwo: bool
        pass

    thingOne: _typing.Final[str] = ...
    thingTwo: _typing.Final[str] = ...

    def __init__(
        self, *,
        thingOne: _typing.Optional[str]=None,
        thingTwo: _typing.Optional[str]=None
    ) -> None: ...

    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'VirtualComplexUnion') -> bool: ...
    def __gt__(self, other: 'VirtualComplexUnion') -> bool: ...
    def __le__(self, other: 'VirtualComplexUnion') -> bool: ...
    def __ge__(self, other: 'VirtualComplexUnion') -> bool: ...

    class Type(_python_std_enum.Enum):
        EMPTY: VirtualComplexUnion.Type = ...
        thingOne: VirtualComplexUnion.Type = ...
        thingTwo: VirtualComplexUnion.Type = ...

    @staticmethod
    def fromValue(value: _VirtualComplexUnionValueType) -> VirtualComplexUnion: ...
    type: _typing.Final[VirtualComplexUnion.Type]
    value: _typing.Final[_VirtualComplexUnionValueType]
    def get_type(self) -> VirtualComplexUnion.Type: ...

    def _to_python(self) -> module.thrift_types.VirtualComplexUnion: ...
    def _to_py3(self) -> VirtualComplexUnion: ...
    def _to_py_deprecated(self) -> "module.ttypes.VirtualComplexUnion": ...   # type: ignore

class NonCopyableStruct(thrift.py3.types.Struct, _typing.Hashable):
    class __fbthrift_IsSet:
        num: bool
        pass

    num: _typing.Final[int] = ...

    def __init__(
        self, *,
        num: _typing.Optional[int]=None
    ) -> None: ...

    def __call__(
        self, *,
        num: _typing.Union[int, None]=None
    ) -> NonCopyableStruct: ...

    def __reduce__(self) -> _typing.Tuple[_typing.Callable, _typing.Tuple[_typing.Type['NonCopyableStruct'], bytes]]: ...
    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'NonCopyableStruct') -> bool: ...
    def __gt__(self, other: 'NonCopyableStruct') -> bool: ...
    def __le__(self, other: 'NonCopyableStruct') -> bool: ...
    def __ge__(self, other: 'NonCopyableStruct') -> bool: ...

    def _to_python(self) -> module.thrift_types.NonCopyableStruct: ...
    def _to_py3(self) -> NonCopyableStruct: ...
    def _to_py_deprecated(self) -> "module.ttypes.NonCopyableStruct": ...   # type: ignore

_NonCopyableUnionValueType = _typing.Union[None, NonCopyableStruct]

class NonCopyableUnion(thrift.py3.types.Union, _typing.Hashable):
    class __fbthrift_IsSet:
        s: bool
        pass

    s: _typing.Final[NonCopyableStruct] = ...

    def __init__(
        self, *,
        s: _typing.Optional[NonCopyableStruct]=None
    ) -> None: ...

    def __hash__(self) -> int: ...
    def __str__(self) -> str: ...
    def __repr__(self) -> str: ...
    def __lt__(self, other: 'NonCopyableUnion') -> bool: ...
    def __gt__(self, other: 'NonCopyableUnion') -> bool: ...
    def __le__(self, other: 'NonCopyableUnion') -> bool: ...
    def __ge__(self, other: 'NonCopyableUnion') -> bool: ...

    class Type(_python_std_enum.Enum):
        EMPTY: NonCopyableUnion.Type = ...
        s: NonCopyableUnion.Type = ...

    @staticmethod
    def fromValue(value: _NonCopyableUnionValueType) -> NonCopyableUnion: ...
    type: _typing.Final[NonCopyableUnion.Type]
    value: _typing.Final[_NonCopyableUnionValueType]
    def get_type(self) -> NonCopyableUnion.Type: ...

    def _to_python(self) -> module.thrift_types.NonCopyableUnion: ...
    def _to_py3(self) -> NonCopyableUnion: ...
    def _to_py_deprecated(self) -> "module.ttypes.NonCopyableUnion": ...   # type: ignore

_List__i64T = _typing.TypeVar('_List__i64T', bound=_typing.Sequence[int])


class List__i64(_typing.Sequence[int], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Sequence[int]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Sequence[int]: ...
    @_typing.overload
    def __getitem__(self, i: int) -> int: ...
    @_typing.overload
    def __getitem__(self, s: slice) -> _typing.Sequence[int]: ...
    def __add__(self, other: _typing.Sequence[int]) -> 'List__i64': ...
    def __radd__(self, other: _typing.Sequence[int]) -> 'List__i64': ...
    def __reversed__(self) -> _typing.Iterator[int]: ...
    #pyre-ignore[14]: no idea what pyre is on about
    def __iter__(self) -> _typing.Iterator[int]: ...


_List__stringT = _typing.TypeVar('_List__stringT', bound=_typing.Sequence[str])


class List__string(_typing.Sequence[str], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Sequence[str]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Sequence[str]: ...
    @_typing.overload
    def __getitem__(self, i: int) -> str: ...
    @_typing.overload
    def __getitem__(self, s: slice) -> _typing.Sequence[str]: ...
    def __add__(self, other: _typing.Sequence[str]) -> 'List__string': ...
    def __radd__(self, other: _typing.Sequence[str]) -> 'List__string': ...
    def __reversed__(self) -> _typing.Iterator[str]: ...
    #pyre-ignore[14]: no idea what pyre is on about
    def __iter__(self) -> _typing.Iterator[str]: ...


class Map__i16_string(_typing.Mapping[int, str], _typing.Hashable):
    def __init__(self, items: _typing.Optional[_typing.Mapping[int, str]]=None) -> None: ...
    def __len__(self) -> int: ...
    def __hash__(self) -> int: ...
    def __copy__(self) -> _typing.Mapping[int, str]: ...
    def __getitem__(self, key: int) -> str: ...
    def __iter__(self) -> _typing.Iterator[int]: ...


containerTypedef = Map__i16_string
