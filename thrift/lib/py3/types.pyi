from enum import Enum
from typing import TypeVar, Type, SupportsInt

eT = TypeVar('eT', bound=Enum)


class NOTSETTYPE(Enum):
    token = 0


NOTSET = NOTSETTYPE.token


class Struct: ...


class Union(Struct): ...


class BadEnum(SupportsInt):
    name: str
    value: int
    enum: Enum

    def __init__(self, the_enum: Type[eT], value: int) -> None: ...
    def __repr__(self) -> str: ...
    def __int__(self) -> int: ...
