#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#

import typing as _typing

from thrift.py3lite.client import (
    AsyncClient as _fbthrift_py3lite_AsyncClient,
    SyncClient as _fbthrift_py3lite_SyncClient,
    Client as _fbthrift_py3lite_Client,
)
import thrift.py3lite.exceptions as _fbthrift_py3lite_exceptions
import thrift.py3lite.types as _fbthrift_py3lite_types
import my.namespacing.extend.test.extend.lite_types
import my.namespacing.test.hsmodule.lite_types
import my.namespacing.test.hsmodule.lite_clients


class ExtendTestService(_fbthrift_py3lite_Client["ExtendTestService.Async", "ExtendTestService.Sync"]):
    class Async(my.namespacing.test.hsmodule.lite_clients.HsTestService.Async):
        async def check(
            self,
            struct1: my.namespacing.test.hsmodule.lite_types.HsFoo
        ) -> bool:
            resp = await self._send_request(
                "ExtendTestService",
                "check",
                my.namespacing.extend.test.extend.lite_types._fbthrift_ExtendTestService_check_args(
                    struct1=struct1,),
                my.namespacing.extend.test.extend.lite_types._fbthrift_ExtendTestService_check_result,
            )
            # shortcut to success path for non-void returns
            if resp.success is not None:
                return resp.success
            raise _fbthrift_py3lite_exceptions.ApplicationError(
                _fbthrift_py3lite_exceptions.ApplicationErrorType.MISSING_RESULT,
                "Empty Response",
            )


    class Sync(my.namespacing.test.hsmodule.lite_clients.HsTestService.Sync):
        def check(
            self,
            struct1: my.namespacing.test.hsmodule.lite_types.HsFoo
        ) -> bool:
            resp = self._send_request(
                "ExtendTestService",
                "check",
                my.namespacing.extend.test.extend.lite_types._fbthrift_ExtendTestService_check_args(
                    struct1=struct1,),
                my.namespacing.extend.test.extend.lite_types._fbthrift_ExtendTestService_check_result,
            )
            # shortcut to success path for non-void returns
            if resp.success is not None:
                return resp.success
            raise _fbthrift_py3lite_exceptions.ApplicationError(
                _fbthrift_py3lite_exceptions.ApplicationErrorType.MISSING_RESULT,
                "Empty Response",
            )


