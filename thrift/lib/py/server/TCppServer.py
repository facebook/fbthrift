from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import traceback

from thrift.protocol.THeaderProtocol import THeaderProtocol
from thrift.server.TServer import TConnectionContext
from thrift.transport.THeaderTransport import THeaderTransport
from thrift.transport.TTransport import TMemoryBuffer

from _cpp_server_wrapper import CppServerWrapper

class TCppConnectionContext(TConnectionContext):
    def __init__(self, client_principal):
        self.client_principal = client_principal

    def getClientPrincipal(self):
        return self.client_principal

class _Adapter(object):
    def __init__(self, processor):
        self.processor = processor

    # TODO mhorowitz: add read headers here, so they can be added to
    # the constructed header buffer.  Also add endpoint addrs to the
    # context
    def call_processor(self, input, client_type, protocol_type,
                       client_principal):
        try:
            # The input string has already had the header removed, but
            # the python processor will expect it to be there.  In
            # order to reconstitute the message with headers, we use
            # the THeaderProtocol object to write into a memory
            # buffer, then pass that buffer to the python processor.

            write_buf = TMemoryBuffer()
            trans = THeaderTransport(write_buf, client_types=[client_type])
            trans.set_protocol_id(protocol_type)
            trans.write(input)
            trans.flush()

            prot_buf = TMemoryBuffer(write_buf.getvalue())
            prot = THeaderProtocol(prot_buf)

            ctx = TCppConnectionContext(client_principal)

            self.processor.process(prot, prot, ctx)

            # And on the way out, we need to strip off the header,
            # because the C++ code will expect to add it.

            read_buf = TMemoryBuffer(prot_buf.getvalue())
            trans = THeaderTransport(read_buf, client_types=[client_type])
            trans.readFrame(0)

            return trans.cstringio_buf.read()
        except:
            # Don't let exceptions escape back into C++
            traceback.print_exc()

class TCppServer(CppServerWrapper):
    def __init__(self, processor):
        CppServerWrapper.__init__(self)
        self.setAdapter(_Adapter(processor))
