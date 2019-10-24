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

from folly.executor cimport get_executor

import asyncio


cdef class SemiStream:
    """
    Base class for all SemiStream object
    """
    def __cinit__(SemiStream self):
        self._executor = get_executor()

    def __aiter__(self):
        return self

    def __anext__(self):
        loop = asyncio.get_event_loop()
        future = loop.create_future()
        future.set_exception(RuntimeError("Not implemented"))
        return future

cdef class ResponseAndSemiStream:
    def get_response(self):
        pass

    def get_stream(self, int buffer_size):
        pass


