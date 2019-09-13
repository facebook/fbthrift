from typing import Iterator, TypeVar

_T = TypeVar("_T")
sT = TypeVar("sT", bound=SemiStream)
rT = TypeVar("rT")
rsT = TypeVar("rsT", bound=ResponseAndSemiStream)

class SemiStream:
    """
    Base class for all SemiStream object
    """
    async def __aiter__(self: sT) -> Iterator[_T]: ...
    async def __anext__(self: sT) -> _T: ...

class ResponseAndSemiStream:
    def get_response(self: rsT) -> rT: ...
    def get_stream(self: rST, buffer_size: int) -> sT: ...
