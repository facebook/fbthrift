from collections.abc import Mapping, Sequence, Set
from enum import Enum
from typing import Any, NamedTuple, Optional, Tuple, Type, Union, overload

from thrift.py3.client import Client
from thrift.py3.common import InterfaceSpec, MethodSpec
from thrift.py3.exceptions import Error
from thrift.py3.server import ServiceInterface
from thrift.py3.types import ListSpec, MapSpec, SetSpec, Struct, StructSpec
@overload
def inspect(cls: Union[Struct, Type[Struct], Error, Type[Error]]) -> StructSpec: ...
@overload
def inspect(
    cls: Union[ServiceInterface, Type[ServiceInterface], Client, Type[Client]]
) -> InterfaceSpec: ...
@overload
def inspect(cls: Union[Sequence[Any], Type[Sequence[Any]]]) -> ListSpec: ...
@overload
def inspect(cls: Union[Set[Any], Type[Set[Any]]]) -> SetSpec: ...
@overload
def inspect(cls: Union[Mapping[Any, Any], Type[Mapping[Any, Any]]]) -> MapSpec: ...
def inspectable(cls: Any) -> bool: ...
