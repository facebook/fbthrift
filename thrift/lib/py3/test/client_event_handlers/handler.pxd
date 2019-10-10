
cdef extern from "thrift/lib/py3/test/client_event_handlers/handler.h" namespace "thrift::py3::test":
    cdef cppclass cTestClientEventHandler "thrift::py3::test::TestClientEventHandler":
        bint preWriteCalled()
