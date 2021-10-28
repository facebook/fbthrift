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

from folly cimport cFollyExceptionWrapper
from libc.stdint cimport int16_t
from libcpp.memory cimport unique_ptr
from libcpp.string cimport string


cdef extern from "thrift/lib/cpp/Thrift.h" namespace "apache::thrift":
    cdef cppclass cTException "apache::thrift::TException":
        const char* what() nogil


cdef extern from "thrift/lib/cpp/TApplicationException.h" \
        namespace "apache::thrift":

    cpdef enum ApplicationErrorType "apache::thrift::TApplicationException::TApplicationExceptionType":
        UNKNOWN "apache::thrift::TApplicationException::UNKNOWN"
        UNKNOWN_METHOD "apache::thrift::TApplicationException::UNKNOWN_METHOD"
        INVALID_MESSAGE_TYPE "apache::thrift::TApplicationException::INVALID_MESSAGE_TYPE"
        WRONG_METHOD_NAME "apache::thrift::TApplicationException::WRONG_METHOD_NAME"
        BAD_SEQUENCE_ID "apache::thrift::TApplicationException::BAD_SEQUENCE_ID"
        MISSING_RESULT "apache::thrift::TApplicationException::MISSING_RESULT"
        INTERNAL_ERROR "apache::thrift::TApplicationException::INTERNAL_ERROR"
        PROTOCOL_ERROR "apache::thrift::TApplicationException::PROTOCOL_ERROR"
        INVALID_TRANSFORM "apache::thrift::TApplicationException::INVALID_TRANSFORM"
        INVALID_PROTOCOL "apache::thrift::TApplicationException::INVALID_PROTOCOL"
        UNSUPPORTED_CLIENT_TYPE "apache::thrift::TApplicationException::UNSUPPORTED_CLIENT_TYPE"
        LOADSHEDDING "apache::thrift::TApplicationException::LOADSHEDDING"
        TIMEOUT "apache::thrift::TApplicationException::TIMEOUT"
        INJECTED_FAILURE "apache::thrift::TApplicationException::INJECTED_FAILURE"

    cdef cppclass cTApplicationException "apache::thrift::TApplicationException"(cTException):
        cTApplicationException(ApplicationErrorType type, const string& message) nogil except +
        ApplicationErrorType getType() nogil


cdef extern from "thrift/lib/py3lite/exceptions.h" namespace "::thrift::py3lite::exception":
    cdef unique_ptr[T] try_make_unique_exception[T](const cFollyExceptionWrapper& ex)


cdef class Error(Exception):
    """base class for all Thrift exceptions"""
    pass


cdef class ApplicationError(Error):
    pass

cdef ApplicationError create_ApplicationError(unique_ptr[cTApplicationException] ex)


cdef object create_py_exception(const cFollyExceptionWrapper& ex)


# Base class for all generated exceptions defined in Thrift IDL
cdef class GeneratedError(Error):
    cdef object _fbthrift_data
    cdef _fbthrift_get_field_value(self, int16_t index)
