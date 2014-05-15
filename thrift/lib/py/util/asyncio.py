# @lint-avoid-python-3-compatibility-imports

import sys
import asyncio
from thrift.Thrift import TMessageType, TApplicationException

def process_main(func):
    """Decorator for process method."""
    def nested(self, iprot, oprot, server_ctx=None):
        (name, type, seqid) = iprot.readMessageBegin()
        name = name.decode()
        if name not in self._processMap:
            iprot.skip(TType.STRUCT)
            iprot.readMessageEnd()
            x = TApplicationException(TApplicationException.UNKNOWN_METHOD,
                    'Unknown function %s' % (name))
            oprot.writeMessageBegin(name, TMessageType.EXCEPTION, seqid)
            x.write(oprot)
            oprot.writeMessageEnd()
            oprot.trans.flush()
        else:
            yield from self._processMap[name](self, seqid, iprot, oprot,
                    server_ctx)

    return nested

def run_in_loop(func):
    func._run_in_loop = True
    return func

def is_run_in_loop(func):
    return getattr(func, "_run_in_loop", False)

def process_method(oneway=False):
    """Decorator for process_xxx methods for asyncio."""
    def _decorator(func):
        def nested(self, seqid, iprot, oprot, server_ctx):
            fn_name = func.__name__.split('_', 1)[-1]
            handler_ctx = self._event_handler.getHandlerContext(fn_name,
                    server_ctx)
            args = getattr(sys.modules[func.__module__], fn_name + "_args")()
            reply_type = TMessageType.REPLY
            self._event_handler.preRead(handler_ctx, fn_name, args)
            args.read(iprot)
            iprot.readMessageEnd()
            self._event_handler.postRead(handler_ctx, fn_name, args)

            result = yield from func(self, args, handler_ctx)
            if not oneway:
                if isinstance(result, TApplicationException):
                    reply_type = TMessageType.EXCEPTION

                self._event_handler.preWrite(handler_ctx, fn_name, result)
                oprot.writeMessageBegin(fn_name, reply_type, seqid)
                result.write(oprot)
                oprot.writeMessageEnd()
                oprot.trans.flush()
                self._event_handler.postWrite(handler_ctx, fn_name, result)

        return nested

    return _decorator
