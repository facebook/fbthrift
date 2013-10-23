from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from fb303.ContextFacebookBase import ContextFacebookBase
from thrift.Thrift import TProcessorEventHandler
from thrift.server.TCppServer import TCppServer

from test.reverse import ReverseService, ttypes

class ReverseEventHandler(TProcessorEventHandler):
    def getHandlerContext(self, fn_name, server_context):
        return server_context

class ReverseHandler(ContextFacebookBase,
                     ReverseService.ContextIface):
    def __init__(self):
        ContextFacebookBase.__init__(self, "reverse")

    def reverse(self, handler_ctx, req):
        print("client_principal is {}".format(handler_ctx.getClientPrincipal()))
        return ttypes.Response(str="".join(reversed(req.str)))

processor = ReverseService.ContextProcessor(ReverseHandler())
processor.setEventHandler(ReverseEventHandler())

server = TCppServer(processor)
server.setPort(9999)
server.serve()
