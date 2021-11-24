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

cimport folly.iobuf

from cython.operator cimport dereference as deref
from folly.iobuf cimport IOBuf
from libcpp.memory cimport make_unique
from libcpp.string cimport string
from libcpp.utility cimport move as cmove
from thrift.py3lite.client.request_channel cimport RequestChannel
from thrift.py3lite.exceptions cimport create_py_exception
from thrift.py3lite.exceptions import ApplicationError, ApplicationErrorType
from thrift.py3lite.serializer import serialize_iobuf, deserialize


cdef class SyncClient:
    def __init__(SyncClient self, RequestChannel channel):
        self._omni_client = make_unique[cOmniClient](cmove(channel._cpp_obj))

    def __enter__(SyncClient self):
        return self

    def __exit__(SyncClient self, exec_type, exc_value, traceback):
        # TODO: close channel
        pass

    def _send_request(
        SyncClient self,
        string service_name,
        string function_name,
        args,
        response_cls,
    ):
        protocol = deref(self._omni_client).getChannelProtocolId()
        cdef IOBuf args_iobuf = serialize_iobuf(args, protocol=protocol)
        if response_cls is None:
            deref(self._omni_client).oneway_send(
                service_name,
                function_name,
                args_iobuf.c_clone(),
                self._persistent_headers,
            )
        else:
            resp = deref(self._omni_client).sync_send(
                service_name,
                function_name,
                args_iobuf.c_clone(),
                self._persistent_headers,
            )
            if resp.buf.hasValue():
                response_iobuf = folly.iobuf.from_unique_ptr(cmove(resp.buf.value()))
                return deserialize(response_cls, response_iobuf, protocol=protocol)
            elif resp.buf.hasError():
                raise create_py_exception(resp.buf.error())
            else:
                raise ApplicationError(
                    ApplicationErrorType.MISSING_RESULT,
                    "Received no result nor error",
                )

    def set_persistent_header(SyncClient self, string key, string value):
        self._persistent_headers[key] = value
