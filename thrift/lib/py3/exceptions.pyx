cdef class Error(Exception):
    """base class for all thrift exceptions (TException)"""
    pass

cdef class ApplicationError(Error):
    """All Application Level Errors (TApplicationException)"""
    pass

cdef class TransportError(Error):
    """All Transport Level Errors (TTransportException)"""
    pass
