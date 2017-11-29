from enum import Enum
from typing import TypeVar, Type

eT = TypeVar('eT', bound=Enum)


class NOTSETTYPE(Enum):
    token = 0


NOTSET = NOTSETTYPE.token


class Struct: ...


class Union(Struct): ...


class BadEnum:
    name: str

    def __init__(self, the_enum: Type[eT], value: int) -> None: ...

    def __repr__(self) -> str: ...
