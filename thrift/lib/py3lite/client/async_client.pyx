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

# cython: c_string_type=unicode, c_string_encoding=utf8

import asyncio
import os

cimport cython
cimport folly.iobuf
import folly.executor

from cython.operator cimport dereference as deref
from folly.executor cimport get_executor
from folly.futures cimport bridgeFutureWith, bridgeSemiFutureWith
from folly.iobuf cimport IOBuf
from libc.stdint cimport uint32_t
from libcpp.memory cimport make_unique
from libcpp.utility cimport move as cmove
from thrift.py3lite.client.client_wrapper import ClientWrapper
from thrift.py3lite.client.omni_client cimport cOmniClientResponseWithHeaders
from thrift.py3lite.client.request_channel cimport (
    createThriftChannelTCP,
    createThriftChannelUnix,
    ClientType as cClientType,
    RequestChannel,
)
from thrift.py3lite.client.request_channel import ClientType
from thrift.py3lite.exceptions cimport create_py_exception
from thrift.py3lite.exceptions import ApplicationError, ApplicationErrorType
from thrift.py3lite.serializer cimport Protocol as cProtocol
from thrift.py3lite.serializer import serialize_iobuf, deserialize


@cython.auto_pickle(False)
cdef class AsyncClient:
    def __cinit__(AsyncClient self):
        loop = asyncio.get_event_loop()
        self._executor = get_executor()
        # Keep a reference to the executor for the life of the client
        self._executor_wrapper = folly.executor.loop_to_q[loop]
        self._connect_future = loop.create_future()

    cdef bind_client(self, cRequestChannel_ptr channel):
        self._omni_client = make_unique[cOmniClient](cmove(channel))

    def __enter__(AsyncClient self):
        raise asyncio.InvalidStateError('Use an async context for thrift clients')

    def __exit__(AsyncClient self):
        raise NotImplementedError()

    async def __aenter__(AsyncClient self):
        await asyncio.shield(self._connect_future)
        return self

    async def __aexit__(AsyncClient self, exc_type, exc_value, traceback):
        self._omni_client.reset()
        # To break any future usage of this client
        badfuture = asyncio.get_event_loop().create_future()
        badfuture.set_exception(asyncio.InvalidStateError('Client Out of Context'))
        badfuture.exception()
        self._connect_future = badfuture

    def _send_request(
        AsyncClient self,
        string service_name,
        string function_name,
        args,
        response_cls,
    ):
        protocol = deref(self._omni_client).getChannelProtocolId()
        cdef IOBuf args_iobuf = serialize_iobuf(args, protocol=protocol)

        loop = asyncio.get_event_loop()
        future = loop.create_future()

        if response_cls is None:
            deref(self._omni_client).oneway_send(
                service_name,
                function_name,
                args_iobuf.c_clone(),
            )
            future.set_result(None)
            return future
        else:
            userdata = (future, response_cls, protocol)
            bridgeSemiFutureWith[cOmniClientResponseWithHeaders](
                self._executor,
                deref(self._omni_client).semifuture_send(
                    service_name,
                    function_name,
                    args_iobuf.c_clone(),
                ),
                _async_client_send_request_callback,
                <PyObject *> userdata,
            )
            return asyncio.shield(future)


cdef void _async_client_send_request_callback(
    cFollyTry[cOmniClientResponseWithHeaders]&& result,
    PyObject* userdata,
):
    pyfuture, response_cls, protocol = <object> userdata
    if result.value().buf.hasValue():
        response_iobuf = folly.iobuf.from_unique_ptr(cmove(result.value().buf.value()))
        pyfuture.set_result(
            deserialize(response_cls, response_iobuf, protocol=protocol)
        )
    elif result.value().buf.hasError():
        pyfuture.set_exception(create_py_exception(result.value().buf.error()))
    else:
        pyfuture.set_exception(ApplicationError(
            ApplicationErrorType.MISSING_RESULT,
            "Received no result nor error",
        ))



def get_client(
    clientKlass,
    *,
    host=None,
    port=None,
    path=None,
    double timeout=1,
    cClientType client_type = ClientType.THRIFT_HEADER_CLIENT_TYPE,
    cProtocol protocol = cProtocol.COMPACT,
):
    if not issubclass(clientKlass, ClientWrapper):
        raise TypeError(f"{clientKlass} is not a py3lite client class")

    endpoint = b''
    if client_type == ClientType.THRIFT_HTTP_CLIENT_TYPE:
        if host is None or port is None:
            raise ValueError("Must set host and port when using ClientType.THRIFT_HTTP_CLIENT_TYPE")
        if path is None:
            raise ValueError("use path='/endpoint' when using ClientType.THRIFT_HTTP_CLIENT_TYPE")
        endpoint = os.fsencode(path)
        path = None

    cdef uint32_t _timeout_ms = int(timeout * 1000)
    host = str(host)  # Accept ipaddress objects
    client = clientKlass.Async()

    if host is not None and port is not None:
        if path is not None:
            raise ValueError("Can not set path and host/port at same time")
        bridgeFutureWith[cRequestChannel_ptr](
            (<AsyncClient>client)._executor,
            createThriftChannelTCP(
                host, port, _timeout_ms, client_type, protocol, endpoint
            ),
            requestchannel_callback,
            <PyObject *>client,
        )
    elif path is not None:
        bridgeFutureWith[cRequestChannel_ptr](
            (<AsyncClient>client)._executor,
            createThriftChannelUnix(
                path, _timeout_ms, client_type, protocol
            ),
            requestchannel_callback,
            <PyObject *>client,
        )
    else:
        raise ValueError("Must set path or host/port")

    return client


cdef void requestchannel_callback(
    cFollyTry[cRequestChannel_ptr]&& result,
    PyObject* userData,
):
    cdef AsyncClient client = <object> userData
    future = client._connect_future
    if result.hasException():
        try:
            result.exception().throw_exception()
        except Exception as pyex:
            future.set_exception(pyex)
    else:
        client.bind_client(cmove(result.value()))
        future.set_result(None)
