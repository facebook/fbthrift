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

from base64 import b64encode, b64decode

from .TProtocol import *

JSON_OBJECT_START = b'{'
JSON_OBJECT_END = b'}'
JSON_ARRAY_START = b'['
JSON_ARRAY_END = b']'
JSON_NEW_LINE = b'\n'
JSON_PAIR_SEPARATOR = b':'
JSON_ELEM_SEPARATOR = b','
JSON_BACKSLASH = b'\\'
JSON_STRING_DELIMITER = b'"'
JSON_ZERO_CHAR = b'0'
JSON_TAB = b"  "

JSON_ESCAPE_CHAR = b'u'
JSON_ESCAPE_PREFIX = b"\\u00"

THRIFT_VERSION_1 = 1

THRIFT_NAN = b"NaN"
THRIFT_INFINITY = b"Infinity"
THRIFT_NEGATIVE_INFINITY = b"-Infinity"

JSON_CHAR_TABLE = [ \
#   0   1    2   3   4   5   6   7    8    9    A   B    C    D   E   F
    0,  0,   0,  0,  0,  0,  0,  0,b'b',b't',b'n',  0,b'f',b'r',  0,  0, \
    0,  0,   0,  0,  0,  0,  0,  0,   0,   0,   0,  0,   0,   0,  0,  0, \
    1,  1,b'"',  1,  1,  1,  1,  1,   1,   1,   1,  1,   1,   1,  1,  1, \
]

ESCAPE_CHARS = b"\"\\bfnrt"
ESCAPE_CHAR_VALS = [b'"', b'\\', b'\b', b'\f', b'\n', b'\r', b'\t']


def hexChar(x):
    x &= 0x0f
    return hex(x)[2:]


def hexVal(ch):
    if ch >= '0' and ch <= '9':
        return int(ch) - int('0')
    elif ch >= 'a' and ch <= 'f':
        return int(ch) - int('a') + 10
    raise TProtocolException(TProtocolException.INVALID_DATA,
                             "Unexpected hex value")


class TJSONContext:
    def __init__(self, indentLevel=0):
        self.indentLevel = indentLevel
        return

    def write(self, trans):
        return

    def escapeNum(self):
        return False

    def writeNewLine(self, trans):
        trans.write(JSON_NEW_LINE)
        self.indent(trans)

    def indent(self, trans):
        for i in range(self.indentLevel):
            trans.write(JSON_TAB)


class TJSONPairContext(TJSONContext):
    def __init__(self, indentLevel=0, isMapPair=False):
        self.indentLevel = indentLevel
        self.first = True
        self.colon = True
        self.isMapPair = isMapPair

    def write(self, trans):
        if self.first:
            self.first = False
            self.colon = True
        else:
            if self.colon:
                trans.write(JSON_PAIR_SEPARATOR + b" ")
            else:
                trans.write(JSON_ELEM_SEPARATOR)
                if self.isMapPair:
                    self.writeNewLine(trans)
            self.colon = not self.colon

    def escapeNum(self):
        return self.colon


class TJSONListContext(TJSONContext):
    def __init__(self, indentLevel=0):
        self.indentLevel = indentLevel
        self.first = True

    def write(self, trans):
        if self.first:
            self.first = False
        else:
            trans.write(JSON_ELEM_SEPARATOR)
            self.writeNewLine(trans)


class TSimpleJSONProtocol(TProtocolBase):
    """
    JSON protocol implementation for Thrift. This protocol is write-only, and
    produces a simple output format that conforms to the JSON standard.
    """

    def __init__(self, trans):
        TProtocolBase.__init__(self, trans)
        self.contexts = [TJSONContext()]  # Used as stack for contexts.
        self.context = TJSONContext()

    def pushContext(self, newContext):
        self.contexts.append(self.context)
        self.context = newContext

    def popContext(self):
        if len(self.contexts) > 0:
            self.context = self.contexts.pop()

    # Writing functions.

    def writeJSONEscapeChar(self, ch):
        self.trans.write(JSON_ESCAPE_PREFIX)
        self.trans.write(hexChar(ch >> 4))
        self.trans.write(hexChar(ch))

    def writeJSONChar(self, ch):
        if ord(ch) >= 0x30:
            if ch == JSON_BACKSLASH:  # Only special character >= 0x30 is '\'.
                self.trans.write(JSON_BACKSLASH)
                self.trans.write(JSON_BACKSLASH)
            else:
                self.trans.write(ch)
        else:
            outCh = JSON_CHAR_TABLE[ord(ch)]
            if outCh == 1:
                self.trans.write(ch)
            elif outCh > 1:
                self.trans.write(JSON_BACKSLASH)
                self.trans.write(outCh)
            else:
                self.writeJSONEscapeChar(ord(ch))

    def writeJSONString(self, outStr):
        self.context.write(self.trans)
        self.trans.write(JSON_STRING_DELIMITER)
        for ch in outStr:
            self.writeJSONChar(ch)
        self.trans.write(JSON_STRING_DELIMITER)

    def writeJSONBase64(self, outStr):
        self.context.write(self.trans)
        self.trans.write(JSON_STRING_DELIMITER)
        b64Str = b64encode(outStr)
        self.trans.write(b64Str)
        self.trans.write(JSON_STRING_DELIMITER)

    def writeJSONInteger(self, num):
        self.context.write(self.trans)
        escapeNum = self.context.escapeNum()
        numStr = str(num)
        if escapeNum:
            self.trans.write(JSON_STRING_DELIMITER)
        self.trans.write(numStr)
        if escapeNum:
            self.trans.write(JSON_STRING_DELIMITER)

    def writeJSONBool(self, boolVal):
        self.context.write(self.trans)
        if self.context.escapeNum():
            self.trans.write(JSON_STRING_DELIMITER)
        if boolVal:
            self.trans.write(b"true")
        else:
            self.trans.write(b"false")
        if self.context.escapeNum():
            self.trans.write(JSON_STRING_DELIMITER)

    def writeJSONDouble(self, num):
        self.context.write(self.trans)
        numStr = str(num)
        special = False

        if numStr == "nan":
            numStr = THRIFT_NAN
            special = True
        elif numStr == "inf":
            numStr = THRIFT_INFINITY
            special = True
        elif numStr == "-inf":
            numStr = THRIFT_NEGATIVE_INFINITY
            special = True

        escapeNum = special or self.context.escapeNum()
        if escapeNum:
            self.trans.write(JSON_STRING_DELIMITER)
        self.trans.write(numStr)
        if escapeNum:
            self.trans.write(JSON_STRING_DELIMITER)

    def writeJSONObjectStart(self):
        self.context.write(self.trans)
        self.trans.write(JSON_OBJECT_START)
        self.pushContext(TJSONPairContext(len(self.contexts)))

    def writeJSONObjectEnd(self):
        self.popContext()
        self.context.writeNewLine(self.trans)
        self.trans.write(JSON_OBJECT_END)

    def writeJSONArrayStart(self):
        self.context.write(self.trans)
        self.trans.write(JSON_ARRAY_START)
        self.pushContext(TJSONListContext(len(self.contexts)))

    def writeJSONArrayEnd(self):
        self.popContext()
        self.context.writeNewLine(self.trans)
        self.trans.write(JSON_ARRAY_END)

    def writeJSONMapStart(self):
        self.context.write(self.trans)
        self.trans.write(JSON_OBJECT_START)
        self.pushContext(TJSONListContext(len(self.contexts)))

    def writeJSONMapEnd(self):
        self.popContext()
        self.context.writeNewLine(self.trans)
        self.trans.write(JSON_OBJECT_END)

    def writeMessageBegin(self, name, messageType, seqId):
        self.writeJSONArrayStart()
        self.pushContext(TJSONListContext(len(self.contexts)))
        self.writeJSONInteger(THRIFT_VERSION_1)
        self.writeJSONString(name)
        self.writeJSONInteger(messageType)
        self.writeJSONInteger(seqId)

    def writeMessageEnd(self):
        self.popContext()
        self.writeJSONArrayEnd()

    def writeStructBegin(self, name):
        self.writeJSONObjectStart()

    def writeStructEnd(self):
        self.writeJSONObjectEnd()

    def writeFieldBegin(self, name, fieldType, fieldId):
        self.context.write(self.trans)
        self.popContext()
        self.pushContext(TJSONPairContext(len(self.contexts)))
        self.context.writeNewLine(self.trans)
        self.writeJSONString(name)

    def writeFieldEnd(self):
        return

    def writeFieldStop(self):
        return

    def writeMapBegin(self, keyType, valType, size):
        self.writeJSONMapStart()
        self.context.writeNewLine(self.trans)
        self.pushContext(TJSONPairContext(len(self.contexts) - 1, True))

    def writeMapEnd(self):
        self.popContext()
        self.writeJSONMapEnd()

    def writeListBegin(self, elemType, size):
        self.writeJSONArrayStart()
        self.context.writeNewLine(self.trans)

    def writeListEnd(self):
        self.writeJSONArrayEnd()

    def writeSetBegin(self, elemType, size):
        self.writeJSONArrayStart()
        self.context.writeNewLine(self.trans)

    def writeSetEnd(self):
        self.writeJSONArrayEnd()

    def writeBool(self, val):
        self.writeJSONBool(val)

    def writeByte(self, byte):
        self.writeJSONInteger(byte)

    def writeI16(self, i16):
        self.writeJSONInteger(i16)

    def writeI32(self, i32):
        self.writeJSONInteger(i32)

    def writeI64(self, i64):
        self.writeJSONInteger(i64)

    def writeDouble(self, d):
        self.writeJSONDouble(d)

    def writeFloat(self, f):
        self.writeJSONDouble(f)

    def writeString(self, outStr):
        self.writeJSONString(outStr)

    def writeBinary(self, outStr):
        self.writeJSONBase64(outStr)


class TSimpleJSONProtocolFactory:
    def getProtocol(self, trans):
        prot = TSimpleJSONProtocol(trans)
        return prot
