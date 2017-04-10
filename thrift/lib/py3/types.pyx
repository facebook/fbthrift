__all__ = ['Struct', 'BadEnum', 'NOTSET']

NOTSET = object()


cdef class Struct:
    """
    Base class for all thrift structs
    """
    pass


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
