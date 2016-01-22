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

import logging
import os
import resource
import sys
from thrift.Thrift import TMessageType, TApplicationException, TType, \
        TRequestContext
from thrift.protocol import THeaderProtocol


log = logging.getLogger(__name__)


def get_memory_usage():
    #this parses the resident set size from /proc/self/stat, which
    #is the same approach the C++ FacebookBase takes
    with open('/proc/self/stat') as stat:
        stat_string = stat.read()
    #rss is field number 23 in /proc/pid/stat, see man proc
    #for the full list
    rss_pages = int(stat_string.split()[23])
    #/proc/pid/stat excludes 3 administrative pages from the count
    rss_pages += 3
    return rss_pages * resource.getpagesize()


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


try:
    # You can use the following for last resort memory leak debugging if Heapy
    # and other clean room methods fail. Setting this to multiple megabytes
    # will expose unexpectedly heavy requests.

    # Note that in case of multithreading use, there's sometimes going to be
    # multiple requests reporting a single bump in memory usage. Look at the
    # slowest request first (it will present the biggest bump).
    MEMORY_WARNING_THRESHOLD = int(
        os.environ.get('THRIFT_PER_REQUEST_MEMORY_WARNING', 0)
    )
except (TypeError, ValueError):
    MEMORY_WARNING_THRESHOLD = 0

_process_method_mem_usage = get_memory_usage if MEMORY_WARNING_THRESHOLD else lambda: 0


def process_method(argtype, oneway=False, twisted=False):
    """Decorator for process_xxx methods."""
    def _decorator(func):
        fn_name = func.__name__.split('_', 1)[-1]
        def nested(self, seqid, iprot, oprot, server_ctx):
            _mem_before = _process_method_mem_usage()

            handler_ctx = self._event_handler.getHandlerContext(fn_name,
                    server_ctx)
            args = argtype()
            reply_type = TMessageType.REPLY
            self._event_handler.preRead(handler_ctx, fn_name, args)
            args.read(iprot)
            iprot.readMessageEnd()
            self._event_handler.postRead(handler_ctx, fn_name, args)

            if hasattr(self._handler, "setRequestContext"):
                request_context = TRequestContext()
                if (isinstance(iprot, THeaderProtocol.THeaderProtocol)):
                    request_context.setHeaders(iprot.trans.get_headers())
                self._handler.setRequestContext(request_context)

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
            if hasattr(self._handler, "setRequestContext"):
                self._handler.setRequestContext(None)

            _mem_after = _process_method_mem_usage()
            if _mem_after - _mem_before > MEMORY_WARNING_THRESHOLD:
                log.error(
                    'Memory usage rose from %d to %d while processing `%s` '
                    'with args `%s`',
                    _mem_before,
                    _mem_after,
                    fn_name,
                    str(args),
                )
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
