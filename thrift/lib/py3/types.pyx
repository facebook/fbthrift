from enum import Enum

__all__ = ['Struct', 'BadEnum', 'NOTSET']

class NOTSETTYPE(Enum):
    token = 0

NOTSET = NOTSETTYPE.token


cdef class Struct:
    """
    Base class for all thrift structs
    """
    cdef bytes _serialize(self, proto):
        return b''

    cdef uint32_t _deserialize(self, const IOBuf* buf, proto):
        return 0


cdef class BadEnum:
    """
    This represents a BadEnum value from thrift.
    So an out of date thrift definition or a default value that is not
    in the enum
    """

    def __init__(self, the_enum, value):
        self.the_enum = the_enum
        self.value = value
        self.name = '#INVALID#'

    def __repr__(self):
        return f'<{self.the_enum.__name__}.{self.name}: {self.value}>'


cdef translate_cpp_enum_to_python(object EnumClass, int value):
    try:
        return EnumClass(value)
    except ValueError:
        return BadEnum(EnumClass, value)
