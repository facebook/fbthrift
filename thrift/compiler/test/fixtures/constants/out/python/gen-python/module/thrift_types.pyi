#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

from __future__ import annotations

import typing as _typing

import enum

import folly.iobuf as _fbthrift_iobuf
import module.thrift_abstract_types as _fbthrift_python_abstract_types
import thrift.python.types as _fbthrift_python_types
import thrift.python.exceptions as _fbthrift_python_exceptions

from module.thrift_enums import (
    EmptyEnum as _fbthrift_EmptyEnum,
    City as _fbthrift_City,
    Company as _fbthrift_Company,
)
EmptyEnum = _fbthrift_EmptyEnum
City = _fbthrift_City
Company = _fbthrift_Company


class Internship(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.Internship):
    weeks: _typing.Final[int] = ...
    title: _typing.Final[str] = ...
    employer: _typing.Final[_typing.Optional[_fbthrift_Company]] = ...
    compensation: _typing.Final[_typing.Optional[float]] = ...
    school: _typing.Final[_typing.Optional[str]] = ...
    def __init__(
        self, *,
        weeks: _typing.Optional[int]=...,
        title: _typing.Optional[str]=...,
        employer: _typing.Optional[Company]=...,
        compensation: _typing.Optional[float]=...,
        school: _typing.Optional[str]=...
    ) -> None: ...

    def __call__(
        self, *,
        weeks: _typing.Optional[int]=...,
        title: _typing.Optional[str]=...,
        employer: _typing.Optional[Company]=...,
        compensation: _typing.Optional[float]=...,
        school: _typing.Optional[str]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, str, _fbthrift_Company, float, str]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "module.thrift_mutable_types.Internship": ...  # type: ignore
    def _to_py3(self) -> "module.types.Internship": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.Internship": ...  # type: ignore
_fbthrift_Internship = Internship

class Range(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.Range):
    min: _typing.Final[int] = ...
    max: _typing.Final[int] = ...
    def __init__(
        self, *,
        min: _typing.Optional[int]=...,
        max: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        min: _typing.Optional[int]=...,
        max: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "module.thrift_mutable_types.Range": ...  # type: ignore
    def _to_py3(self) -> "module.types.Range": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.Range": ...  # type: ignore
_fbthrift_Range = Range

class struct1(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.struct1):
    a: _typing.Final[int] = ...
    b: _typing.Final[str] = ...
    def __init__(
        self, *,
        a: _typing.Optional[int]=...,
        b: _typing.Optional[str]=...
    ) -> None: ...

    def __call__(
        self, *,
        a: _typing.Optional[int]=...,
        b: _typing.Optional[str]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, str]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "module.thrift_mutable_types.struct1": ...  # type: ignore
    def _to_py3(self) -> "module.types.struct1": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.struct1": ...  # type: ignore
_fbthrift_struct1 = struct1

class struct2(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.struct2):
    a: _typing.Final[int] = ...
    b: _typing.Final[str] = ...
    c: _typing.Final[_fbthrift_struct1] = ...
    d: _typing.Final[_typing.Sequence[int]] = ...
    def __init__(
        self, *,
        a: _typing.Optional[int]=...,
        b: _typing.Optional[str]=...,
        c: _typing.Optional[struct1]=...,
        d: _typing.Optional[_typing.Sequence[int]]=...
    ) -> None: ...

    def __call__(
        self, *,
        a: _typing.Optional[int]=...,
        b: _typing.Optional[str]=...,
        c: _typing.Optional[struct1]=...,
        d: _typing.Optional[_typing.Sequence[int]]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, str, _fbthrift_struct1, _typing.Sequence[int]]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "module.thrift_mutable_types.struct2": ...  # type: ignore
    def _to_py3(self) -> "module.types.struct2": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.struct2": ...  # type: ignore
_fbthrift_struct2 = struct2

class struct3(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.struct3):
    a: _typing.Final[str] = ...
    b: _typing.Final[int] = ...
    c: _typing.Final[_fbthrift_struct2] = ...
    def __init__(
        self, *,
        a: _typing.Optional[str]=...,
        b: _typing.Optional[int]=...,
        c: _typing.Optional[struct2]=...
    ) -> None: ...

    def __call__(
        self, *,
        a: _typing.Optional[str]=...,
        b: _typing.Optional[int]=...,
        c: _typing.Optional[struct2]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[str, int, _fbthrift_struct2]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "module.thrift_mutable_types.struct3": ...  # type: ignore
    def _to_py3(self) -> "module.types.struct3": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.struct3": ...  # type: ignore
_fbthrift_struct3 = struct3

class struct4(_fbthrift_python_types.Struct, _fbthrift_python_abstract_types.struct4):
    a: _typing.Final[int] = ...
    b: _typing.Final[_typing.Optional[float]] = ...
    c: _typing.Final[_typing.Optional[int]] = ...
    def __init__(
        self, *,
        a: _typing.Optional[int]=...,
        b: _typing.Optional[float]=...,
        c: _typing.Optional[int]=...
    ) -> None: ...

    def __call__(
        self, *,
        a: _typing.Optional[int]=...,
        b: _typing.Optional[float]=...,
        c: _typing.Optional[int]=...
    ) -> _typing.Self: ...
    def __iter__(self) -> _typing.Iterator[_typing.Tuple[str, _typing.Union[int, float, int]]]: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "module.thrift_mutable_types.struct4": ...  # type: ignore
    def _to_py3(self) -> "module.types.struct4": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.struct4": ...  # type: ignore
_fbthrift_struct4 = struct4

class union1(_fbthrift_python_types.Union, _fbthrift_python_abstract_types.union1):
    i: _typing.Final[int] = ...
    d: _typing.Final[float] = ...
    def __init__(
        self, *,
        i: _typing.Optional[int]=...,
        d: _typing.Optional[float]=...
    ) -> None: ...


    class Type(enum.Enum):
        EMPTY: union1.Type = ...
        i: union1.Type = ...
        d: union1.Type = ...

    class FbThriftUnionFieldEnum(enum.Enum):
        EMPTY: union1.FbThriftUnionFieldEnum = ...
        i: union1.FbThriftUnionFieldEnum = ...
        d: union1.FbThriftUnionFieldEnum = ...

    fbthrift_current_value: _typing.Final[_typing.Union[None, int, float]]
    fbthrift_current_field: _typing.Final[FbThriftUnionFieldEnum]
    @classmethod
    def fromValue(cls, value: _typing.Union[None, int, float]) -> union1: ...
    value: _typing.Final[_typing.Union[None, int, float]]
    type: _typing.Final[Type]
    def get_type(self) -> Type: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "module.thrift_mutable_types.union1": ...  # type: ignore
    def _to_py3(self) -> "module.types.union1": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.union1": ...  # type: ignore
_fbthrift_union1 = union1

class union2(_fbthrift_python_types.Union, _fbthrift_python_abstract_types.union2):
    i: _typing.Final[int] = ...
    d: _typing.Final[float] = ...
    s: _typing.Final[_fbthrift_struct1] = ...
    u: _typing.Final[_fbthrift_union1] = ...
    def __init__(
        self, *,
        i: _typing.Optional[int]=...,
        d: _typing.Optional[float]=...,
        s: _typing.Optional[struct1]=...,
        u: _typing.Optional[union1]=...
    ) -> None: ...


    class Type(enum.Enum):
        EMPTY: union2.Type = ...
        i: union2.Type = ...
        d: union2.Type = ...
        s: union2.Type = ...
        u: union2.Type = ...

    class FbThriftUnionFieldEnum(enum.Enum):
        EMPTY: union2.FbThriftUnionFieldEnum = ...
        i: union2.FbThriftUnionFieldEnum = ...
        d: union2.FbThriftUnionFieldEnum = ...
        s: union2.FbThriftUnionFieldEnum = ...
        u: union2.FbThriftUnionFieldEnum = ...

    fbthrift_current_value: _typing.Final[_typing.Union[None, int, float, _fbthrift_struct1, _fbthrift_union1]]
    fbthrift_current_field: _typing.Final[FbThriftUnionFieldEnum]
    @classmethod
    def fromValue(cls, value: _typing.Union[None, int, float, _fbthrift_struct1, _fbthrift_union1]) -> union2: ...
    value: _typing.Final[_typing.Union[None, int, float, _fbthrift_struct1, _fbthrift_union1]]
    type: _typing.Final[Type]
    def get_type(self) -> Type: ...
    def _to_python(self) -> _typing.Self: ...
    def _to_mutable_python(self) -> "module.thrift_mutable_types.union2": ...  # type: ignore
    def _to_py3(self) -> "module.types.union2": ...  # type: ignore
    def _to_py_deprecated(self) -> "module.ttypes.union2": ...  # type: ignore
_fbthrift_union2 = union2


myInt: int = ...

name: str = ...

multi_line_string: str = ...

states: _typing.Sequence[_typing.Mapping[str, int]] = ...

x: float = ...

y: float = ...

z: float = ...

zeroDoubleValue: float = ...

longDoubleValue: float = ...

bin: bytes = ...

my_company: _fbthrift_Company = ...

foo: str = ...

bar: int = ...

mymap: _typing.Mapping[str, str] = ...

my_apps: _typing.Sequence[_fbthrift_Company] = ...

instagram: _fbthrift_Internship = ...

partial_const: _fbthrift_Internship = ...

kRanges: _typing.Sequence[_fbthrift_Range] = ...

internList: _typing.Sequence[_fbthrift_Internship] = ...

pod_0: _fbthrift_struct1 = ...

pod_s_0: _fbthrift_struct1 = ...

pod_1: _fbthrift_struct1 = ...

pod_s_1: _fbthrift_struct1 = ...

pod_2: _fbthrift_struct2 = ...

pod_trailing_commas: _fbthrift_struct2 = ...

pod_s_2: _fbthrift_struct2 = ...

pod_3: _fbthrift_struct3 = ...

pod_s_3: _fbthrift_struct3 = ...

pod_4: _fbthrift_struct4 = ...

u_1_1: _fbthrift_union1 = ...

u_1_2: _fbthrift_union1 = ...

u_1_3: _fbthrift_union1 = ...

u_2_1: _fbthrift_union2 = ...

u_2_2: _fbthrift_union2 = ...

u_2_3: _fbthrift_union2 = ...

u_2_4: _fbthrift_union2 = ...

u_2_5: _fbthrift_union2 = ...

u_2_6: _fbthrift_union2 = ...

apostrophe: str = ...

tripleApostrophe: str = ...

quotationMark: str = ...

backslash: str = ...

escaped_a: str = ...

char2ascii: _typing.Mapping[str, int] = ...

escaped_strings: _typing.Sequence[str] = ...

unicode_list: _typing.Sequence[str] = ...

false_c: bool = ...

true_c: bool = ...

zero_byte: int = ...

zero16: int = ...

zero32: int = ...

zero64: int = ...

zero_dot_zero: float = ...

empty_string: str = ...

empty_int_list: _typing.Sequence[int] = ...

empty_string_list: _typing.Sequence[str] = ...

empty_int_set: _typing.Set[int] = ...

empty_string_set: _typing.Set[str] = ...

empty_int_int_map: _typing.Mapping[int, int] = ...

empty_int_string_map: _typing.Mapping[int, str] = ...

empty_string_int_map: _typing.Mapping[str, int] = ...

empty_string_string_map: _typing.Mapping[str, str] = ...

unicode_map: _typing.Mapping[str, str] = ...

maxIntDec: int = ...

maxIntOct: int = ...

maxIntHex: int = ...

maxIntBin: int = ...

maxDub: float = ...

minDub: float = ...

minSDub: float = ...

maxPIntDec: int = ...

maxPIntOct: int = ...

maxPIntHex: int = ...

maxPIntBin: int = ...

maxPDub: float = ...

minPDub: float = ...

minPSDub: float = ...

minIntDec: int = ...

minIntOct: int = ...

minIntHex: int = ...

minIntBin: int = ...

maxNDub: float = ...

minNDub: float = ...

minNSDub: float = ...

I2B: _typing.Mapping[int, bool] = ...

I2B_REF: _typing.Mapping[int, bool] = ...

MyCompany = _fbthrift_Company
MyStringIdentifier = str
MyIntIdentifier = int
MyMapIdentifier = _typing.Dict[str, str]
