#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import sys
from thrift.Thrift import TMessageType, TApplicationException, TType

def process_main(twisted=False):
    """Decorator for process method."""
    def _decorator(func):
        def nested(self, iprot, oprot, server_ctx=None):
            (name, type, seqid) = iprot.readMessageBegin()
            if sys.version_info[0] >= 3:
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
                if twisted is True:
                    from twisted.internet import defer
                    return defer.succeed(None)
            else:
                ret = self._processMap[name](self, seqid, iprot, oprot,
                        server_ctx)
                if twisted is True:
                    return ret
                else:
                    return True

        return nested

    return _decorator

def process_method(argtype, oneway=False, twisted=False):
    """Decorator for process_xxx methods."""
    def _decorator(func):
        def nested(self, seqid, iprot, oprot, server_ctx):
            fn_name = func.__name__.split('_', 1)[-1]
            handler_ctx = self._event_handler.getHandlerContext(fn_name,
                    server_ctx)
            args = argtype()
            reply_type = TMessageType.REPLY
            self._event_handler.preRead(handler_ctx, fn_name, args)
            args.read(iprot)
            iprot.readMessageEnd()
            self._event_handler.postRead(handler_ctx, fn_name, args)

            if twisted is True:
                return func(self, args, handler_ctx, seqid, oprot)
            elif oneway is True:
                func(self, args, handler_ctx)
            else:
                result = func(self, args, handler_ctx)
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

def write_results_success_callback(func):
    """Decorator for twisted write_results_success_xxx methods.
       No need to call func so it can be empty.
    """
    def nested(self, success, result, seqid, oprot, handler_ctx):
        fn_name = func.__name__.split('_', 3)[-1]
        result.success = success
        self._event_handler.preWrite(handler_ctx, fn_name, result)
        oprot.writeMessageBegin(fn_name, TMessageType.REPLY, seqid)
        result.write(oprot)
        oprot.writeMessageEnd()
        oprot.trans.flush()
        self._event_handler.postWrite(handler_ctx, fn_name, result)

    return nested

def write_results_exception_callback(func):
    """Decorator for twisted write_results_exception_xxx methods."""
    def nested(self, error, result, seqid, oprot, handler_ctx):
        fn_name = func.__name__.split('_', 3)[-1]

        # Call the decorated function
        reply_type, result = func(self, error, result, handler_ctx)

        self._event_handler.preWrite(handler_ctx, fn_name, result)
        oprot.writeMessageBegin(fn_name, reply_type, seqid)
        result.write(oprot)
        oprot.writeMessageEnd()
        oprot.trans.flush()
        self._event_handler.postWrite(handler_ctx, fn_name, result)

    return nested
