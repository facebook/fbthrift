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

import asyncio

from libcpp.memory cimport make_unique
from thrift.py3.server cimport ServiceInterface, ThriftServer


cdef class Handler(ServiceInterface):
    def __cinit__(self):
        self._cpp_obj = cStreamTestService.createInstance()


cdef class TestServer:
    cdef ThriftServer server
    cdef object serve_task

    def __init__(self, ip=None):
        self.server = ThriftServer(Handler(), ip=ip)

    async def __aenter__(self):
        self.serve_task = asyncio.get_event_loop().create_task(self.server.serve())
        return await self.server.get_address()

    async def __aexit__(self, *exc_info):
        self.server.stop()
        await self.serve_task
