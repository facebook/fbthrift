# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

# cython: c_string_type=unicode, c_string_encoding=ascii

from collections import namedtuple
from cython.operator cimport dereference as deref, preincrement as inc
from enum import Enum
from libc.stdint cimport uint32_t

class CompileRetcode(Enum):
    SUCCESS = 0
    FAILURE = 1

class DiagnosticLevel(Enum):
    FAILURE = 0
    YY_ERROR = 1
    WARNING = 2
    VERBOSE = 3
    DBG = 4

DiagnosticMessage = namedtuple(
    'DiagnosticMessage',
    ['level', 'filename', 'lineno', 'last_token', 'message']
)

def thrift_compile(vector[string] argv):
    result = compile(argv)

    py_messages = []
    it = result.diagnostics.const_begin()
    end = result.diagnostics.const_end()
    while it != end:
        py_messages.append(
            DiagnosticMessage(
                DiagnosticLevel(<uint32_t>(deref(it).level)),
                deref(it).filename,
                deref(it).lineno,
                deref(it).last_token,
                deref(it).message,
            )
        )
        inc(it)
    return CompileRetcode(<uint32_t>result.retcode), py_messages
