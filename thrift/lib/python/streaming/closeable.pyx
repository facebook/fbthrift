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

import asyncio
import sys

from thrift.python.streaming.python_user_exception import PythonUserException
from thrift.python.serializer import serialize_iobuf 



cdef class CloseableGenerator:
    """
    Transforms an async iterator (e.g., `async for` into a closeable async generator)
    by wrapping each __anext__ call in a Task to make it cancellable
    """
    def __cinit__(
        self,
        stream_generator, # async iterator
        Protocol protocol,
        return_struct_class,
        tuple user_exceptions, # tuple[UserExceptionMeta, ...]
    ):
        self._stream_generator = stream_generator
        self._protocol = protocol
        self._anext_task = None
        self._closed = False
        self._return_struct_class = return_struct_class
        self._user_exceptions = tuple(
            ((<UserExceptionMeta>ex).ex_class for ex in user_exceptions)
        )
        self._user_exception_meta = user_exceptions


    def __aiter__(self) -> _typing.AsyncIterator[_fbthrift_iobuf.IOBuf]:
        return self

    cdef object transform_to_python_user_exception(self, ex):
        cdef UserExceptionMeta user_ex
        for user_ex in self._user_exception_meta:
            if not isinstance(ex, user_ex.ex_class):
                continue
            return_struct = self._return_struct_class(**{user_ex.ex_field : ex})
            buf = serialize_iobuf(return_struct, self._protocol)
            return PythonUserException(user_ex.ex_name, str(ex), buf)

        raise RuntimeError(
            f"Exception type {type(ex)} not found in {self._user_exception_meta}"
        )

    async def __anext__(self) -> _fbthrift_iobuf.IOBuf:
        if self._closed:
            raise StopAsyncIteration
        # if coroutine, ensure_future transforms it into Task and schedules it
        # if already Future or Task, it's a no-op
        self._anext_task = asyncio.ensure_future(self._stream_generator.__anext__())
        try:
            item = await self._anext_task
        except StopAsyncIteration:
            self._closed = True
            raise
        except self._user_exceptions as ex:
            raise self.transform_to_python_user_exception(ex)
        finally:
            self._anext_task = None
        return serialize_iobuf(self._return_struct_class(success=item), self._protocol)

    async def asend(self, value: None) -> _fbthrift_iobuf.IOBuf:
        assert value is None, "asend() only supports None"
        return await self.__anext__()

    async def athrow(self, typ: _typing.Any, val: _typing.Any = None, tb: _typing.Any = None) -> _fbthrift_iobuf.IOBuf:
        # Present for async generator protocol completeness. Not called by the
        # thrift runtime (cancelAsyncGenerator uses aclose(), not athrow()).
        await self.aclose()
        raise StopAsyncIteration

    async def aclose(self) -> None:
        if self._closed:
            return
        self._closed = True
        task = self._anext_task
        if task is not None and not task.done():
            task.cancel()
            try:
                await task
            except (asyncio.CancelledError, StopAsyncIteration):
                pass
            except Exception as e:
                print(
                    f"CloseableGenerator.aclose: unexpected exception on task.cancel():\n{e}",
                    file=sys.stderr,
                )
        await self._stream_generator.aclose()


cdef class UserExceptionMeta:
    def __cinit__(self, object ex_class, str ex_field, str ex_name):
        self.ex_class = ex_class
        self.ex_field = ex_field
        self.ex_name = ex_name
