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

class TType:
    STOP = 0
    VOID = 1
    BOOL = 2
    BYTE = 3
    I08 = 3
    DOUBLE = 4
    I16 = 6
    I32 = 8
    I64 = 10
    STRING = 11
    UTF7 = 11
    STRUCT = 12
    MAP = 13
    SET = 14
    LIST = 15
    UTF8 = 16
    UTF16 = 17
    FLOAT = 19

class TMessageType:
    CALL = 1
    REPLY = 2
    EXCEPTION = 3
    ONEWAY = 4


class TProcessorEventHandler:
    """Event handler for thrift processors"""
    #TODO: implement asyncComplete for Twisted

    def getHandlerContext(self, fn_name, server_context):
        """Called at the start of processing a handler method"""
        return None

    def preRead(self, handler_context, fn_name, args):
        """Called before the handler method's argument are read"""
        pass

    def postRead(self, handler_context, fn_name, args):
        """Called after the handler method's argument are read"""
        pass

    def preWrite(self, handler_context, fn_name, result):
        """Called before the handler method's results are written"""
        pass

    def postWrite(self, handler_context, fn_name, result):
        """Called after the handler method's results are written"""
        pass

    def handlerException(self, handler_context, fn_name, exception):
        """Called if (and only if) the handler threw an expected exception."""
        pass

    def handlerError(self, handler_context, fn_name, exception):
        """Called if (and only if) the handler threw an unexpected exception.

        Note that this method is NOT called if the handler threw an
        exception that is declared in the thrift service specification"""
        pass


class TProcessor:

    """Base class for procsessor, which works on two streams."""

    def __init__(self):
        self._event_handler = TProcessorEventHandler()  # null object handler

    def setEventHandler(self, event_handler):
        self._event_handler = event_handler

    def getEventHandler(self):
        return self._event_handler

    def process(self, iprot, oprot, server_context=None):
        pass


class TException(Exception):

    """Base class for all thrift exceptions."""

    # BaseException.message is deprecated in Python v[2.6,3.0)
    if (2, 6, 0) <= sys.version_info < (3, 0):
        def _get_message(self):
            return self._message

        def _set_message(self, message):
            self._message = message

        message = property(_get_message, _set_message)

    def __init__(self, message=None):
        Exception.__init__(self, message)
        self.message = message

class TApplicationException(TException):

    """Application level thrift exceptions."""

    UNKNOWN = 0
    UNKNOWN_METHOD = 1
    INVALID_MESSAGE_TYPE = 2
    WRONG_METHOD_NAME = 3
    BAD_SEQUENCE_ID = 4
    MISSING_RESULT = 5
    INTERNAL_ERROR = 6
    PROTOCOL_ERROR = 7
    INVALID_TRANSFORM = 8
    INVALID_PROTOCOL = 9
    UNSUPPORTED_CLIENT_TYPE = 10

    def __init__(self, type=UNKNOWN, message=None):
        TException.__init__(self, message)
        self.type = type

    def __str__(self):
        if self.message:
            return self.message
        elif self.type == self.UNKNOWN_METHOD:
            return 'Unknown method'
        elif self.type == self.INVALID_MESSAGE_TYPE:
            return 'Invalid message type'
        elif self.type == self.WRONG_METHOD_NAME:
            return 'Wrong method name'
        elif self.type == self.BAD_SEQUENCE_ID:
            return 'Bad sequence ID'
        elif self.type == self.MISSING_RESULT:
            return 'Missing result'
        elif self.type == self.INTERNAL_ERROR:
            return 'Internal error'
        elif self.type == self.PROTOCOL_ERROR:
            return 'Protocol error'
        elif self.type == self.INVALID_TRANSFORM:
            return 'Invalid transform'
        elif self.type == self.INVALID_PROTOCOL:
            return 'Invalid protocol'
        elif self.type == self.UNSUPPORTED_CLIENT_TYPE:
            return 'Unsupported client type'
        else:
            return 'Default (unknown) TApplicationException'

    def read(self, iprot):
        iprot.readStructBegin()
        while True:
            (fname, ftype, fid) = iprot.readFieldBegin()
            if ftype == TType.STOP:
                break
            if fid == 1:
                if ftype == TType.STRING:
                    self.message = iprot.readString()
                else:
                    iprot.skip(ftype)
            elif fid == 2:
                if ftype == TType.I32:
                    self.type = iprot.readI32()
                else:
                    iprot.skip(ftype)
            else:
                iprot.skip(ftype)
            iprot.readFieldEnd()
        iprot.readStructEnd()

    def write(self, oprot):
        oprot.writeStructBegin(b'TApplicationException')
        if self.message is not None:
            oprot.writeFieldBegin(b'message', TType.STRING, 1)
            oprot.writeString(self.message)
            oprot.writeFieldEnd()
        if self.type is not None:
            oprot.writeFieldBegin(b'type', TType.I32, 2)
            oprot.writeI32(self.type)
            oprot.writeFieldEnd()
        oprot.writeFieldStop()
        oprot.writeStructEnd()
