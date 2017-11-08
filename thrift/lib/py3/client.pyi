from types import TracebackType
from typing import TypeVar, Optional, Type, Dict, Callable, Any

cT = TypeVar('cT', bound='Client')


class Client:
    def set_persistent_header(self, key: str, value: str) -> None: ...
    async def __aenter__(self: cT) -> cT: ...
    async def __aexit__(
        self: cT,
        exc_type: Optional[Type[BaseException]],
        exc_value: Optional[Exception],
        traceback: Optional[TracebackType],
    ) -> Optional[bool]: ...


def get_client(
    clientKlass: Type[cT],
    *,
    host: str = ...,
    port: int,
    timeout: float = ...,
    headers: Dict[str, str] = None
) -> cT: ...


def install_proxy_factory(
    factory: Optional[Callable[[Type[Client]], Callable[[cT], Any]]],
) -> None: ...
