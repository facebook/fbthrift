# Copyright (c) Meta Platforms, Inc. and affiliates.
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

from libcpp cimport bool as cbool
from thrift.python.protocol cimport Protocol

cdef class CloseableGenerator:
    cdef object _stream_generator
    cdef Protocol _protocol
    # object state
    cdef object _anext_task
    cdef cbool _closed
    # properties of the schema
    cdef object _return_struct_class
    cdef tuple _user_exceptions
    cdef tuple _user_exception_meta

    cdef object transform_to_python_user_exception(self, ex)

cdef class UserExceptionMeta:
    cdef object ex_class
    cdef str ex_field
    cdef str ex_name
