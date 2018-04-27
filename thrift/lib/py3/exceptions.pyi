from enum import Enum, Flag

class TransportErrorType(Enum):
    UNKNOWN: int
    NOT_OPEN: int
    ALREADY_OPEN: int
    TIMED_OUT: int
    END_OF_FILE: int
    INTERRUPTED: int
    BAD_ARGS: int
    CORRUPTED_DATA: int
    INTERNAL_ERROR: int
    NOT_SUPPORTED: int
    INVALID_STATE: int
    INVALID_FRAME_SIZE: int
    SSL_ERROR: int
    COULD_NOT_BIND: int
    SASL_HANDSHAKE_TIMEOUT: int
    NETWORK_ERROR: int
    value: int

class TransportOptions(Flag):
    CHANNEL_IS_VALID: int
    value: int

class ApplicationErrorType(Enum):
    UNKNOWN: int
    UNKNOWN_METHOD: int
    INVALID_MESSAGE_TYPE: int
    WRONG_METHOD_NAME: int
    BAD_SEQUENCE_ID: int
    MISSING_RESULT: int
    INTERNAL_ERROR: int
    PROTOCOL_ERROR: int
    INVALID_TRANSFORM: int
    INVALID_PROTOCOL: int
    UNSUPPORTED_CLIENT_TYPE: int
    LOADSHEDDING: int
    TIMEOUT: int
    INJECTED_FAILURE: int
    value: int

class ProtocolErrorType(Enum):
    UNKNOWN: int
    INVALID_DATA: int
    NEGATIVE_SIZE: int
    BAD_VERSION: int
    NOT_IMPLEMENTED: int
    MISSING_REQUIRED_FIELD: int
    value: int


class Error(Exception): ...


class ApplicationError(Error):
    def __int__(self, type: ApplicationErrorType, message: str) -> None: ...
    type: ApplicationErrorType
    message: str


class LibraryError(Error): ...


class ProtocolError(LibraryError):
    type: ProtocolErrorType
    message: str


class TransportError(LibraryError):
    type: TransportErrorType
    options: TransportOptions
    message: str
    errno: int
