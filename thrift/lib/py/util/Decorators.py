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

import contextlib
import logging
import os
import resource

from thrift.Thrift import (
    TApplicationException,
    TRequestContext,
)
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


def get_function_name(func):
    return func.__name__.split('_', 1)[-1]


def make_unknown_function_exception(name):
    return TApplicationException(
        TApplicationException.UNKNOWN_METHOD,
        'Unknown function {!r}'.format(name),
    )


def process_main(twisted=False):
    """Decorator for process method."""
    def _decorator(func):
        def nested(self, iprot, oprot, server_ctx=None):
            # self is a TProcessor instance
            name, seqid = self.readMessageBegin(iprot)
            if self.doesKnowFunction(name):
                ret = self.callFunction(name, seqid, iprot, oprot, server_ctx)
                return ret if twisted else True
            self.skipMessageStruct(iprot)
            exc = make_unknown_function_exception(name)
            self.writeException(oprot, name, seqid, exc)
            if twisted:
                from twisted.internet import defer
                return defer.succeed(None)

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


def needs_request_context(processor):
    return hasattr(processor._handler, "setRequestContext")


def set_request_context(processor, iprot):
    if needs_request_context(processor):
        request_context = TRequestContext()
        if isinstance(iprot, THeaderProtocol.THeaderProtocol):
            request_context.setHeaders(iprot.trans.get_headers())
        processor._handler.setRequestContext(request_context)


def reset_request_context(processor):
    if needs_request_context(processor):
        processor._handler.setRequestContext(None)


def process_method(argtype, oneway=False, twisted=False):
    """Decorator for process_xxx methods."""
    def _decorator(func):
        def nested(self, seqid, iprot, oprot, server_ctx):
            _mem_before = _process_method_mem_usage()
            fn_name = get_function_name(func)
            # self is a TProcessor instance
            handler_ctx = self._event_handler.getHandlerContext(
                fn_name,
                server_ctx,
            )
            args = self.readArgs(iprot, handler_ctx, fn_name, argtype)

            if twisted:
                return func(self, args, handler_ctx, seqid, oprot)

            set_request_context(self, iprot)
            result = func(self, args, handler_ctx)
            if not oneway:
                self.writeReply(oprot, handler_ctx, fn_name, seqid, result)
            reset_request_context(self)

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
        self.writeReply(oprot, handler_ctx, fn_name, seqid, result)

    return nested


def write_results_exception_callback(func):
    """Decorator for twisted write_results_exception_xxx methods."""
    def nested(self, error, result, seqid, oprot, handler_ctx):
        fn_name = func.__name__.split('_', 3)[-1]
        _, result = func(self, error, result, handler_ctx)
        self.writeReply(oprot, handler_ctx, fn_name, seqid, result)

    return nested


@contextlib.contextmanager
def protocol_manager(protocol):
    yield protocol.client
    protocol.close()
