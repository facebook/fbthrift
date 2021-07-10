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

from libcpp.string cimport string
from libcpp.vector cimport vector

cdef extern from "thrift/compiler/ast/diagnostic.h" namespace "apache::thrift::compiler":
    cdef cppclass diagnostic_level:
        pass

cdef extern from "thrift/compiler/ast/diagnostic.h" namespace "apache::thrift::compiler":
    cdef cppclass diagnostic:
        diagnostic_level level()
        string file()
        int lineno()
        string token()
        string message()
        string str()

    cdef cppclass diagnostic_results:
        vector[diagnostic] diagnostics()


cdef extern from "thrift/compiler/compiler.h" namespace "apache::thrift::compiler":
    cdef cppclass compile_retcode:
        pass

    cdef struct compile_result:
        compile_retcode retcode
        diagnostic_results detail

    cdef compile_result compile(vector[string]) except +
