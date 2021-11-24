#
# Autogenerated by Thrift
#
# DO NOT EDIT
#  @generated
#
from abc import ABCMeta
import typing as _typing

import folly.iobuf

from thrift.py3lite.serializer import serialize_iobuf, deserialize, Protocol
from thrift.py3lite.server import ServiceInterface, oneway

import module.lite_types

class MyServiceInterface(
    ServiceInterface,
    metaclass=ABCMeta
):

    @staticmethod
    def service_name() -> bytes:
        return b"MyService"

    # pyre-ignore[3]: it can return anything
    def getFunctionTable(self) -> _typing.Mapping[bytes, _typing.Callable[..., _typing.Any]]:
        functionTable = {
            b"ping": self._fbthrift__handler_ping,
            b"getRandomData": self._fbthrift__handler_getRandomData,
            b"sink": self._fbthrift__handler_sink,
            b"putDataById": self._fbthrift__handler_putDataById,
            b"hasDataById": self._fbthrift__handler_hasDataById,
            b"getDataById": self._fbthrift__handler_getDataById,
            b"deleteDataById": self._fbthrift__handler_deleteDataById,
            b"lobDataById": self._fbthrift__handler_lobDataById,
        }
        return {**super().getFunctionTable(), **functionTable}



    async def ping(
            self
        ) -> None:
        raise NotImplementedError("async def ping is not implemented")

    async def _fbthrift__handler_ping(self, args: folly.iobuf.IOBuf, protocol: Protocol) -> folly.iobuf.IOBuf:
        args_struct = deserialize(module.lite_types._fbthrift_MyService_ping_args, args, protocol)
        value = await self.ping()
        return_struct = module.lite_types._fbthrift_MyService_ping_result()

        return serialize_iobuf(return_struct, protocol)


    async def getRandomData(
            self
        ) -> str:
        raise NotImplementedError("async def getRandomData is not implemented")

    async def _fbthrift__handler_getRandomData(self, args: folly.iobuf.IOBuf, protocol: Protocol) -> folly.iobuf.IOBuf:
        args_struct = deserialize(module.lite_types._fbthrift_MyService_getRandomData_args, args, protocol)
        value = await self.getRandomData()
        return_struct = module.lite_types._fbthrift_MyService_getRandomData_result(success=value)

        return serialize_iobuf(return_struct, protocol)


    async def sink(
            self,
            sink: int
        ) -> None:
        raise NotImplementedError("async def sink is not implemented")

    async def _fbthrift__handler_sink(self, args: folly.iobuf.IOBuf, protocol: Protocol) -> folly.iobuf.IOBuf:
        args_struct = deserialize(module.lite_types._fbthrift_MyService_sink_args, args, protocol)
        value = await self.sink(args_struct.sink,)
        return_struct = module.lite_types._fbthrift_MyService_sink_result()

        return serialize_iobuf(return_struct, protocol)


    async def putDataById(
            self,
            id: int,
            data: str
        ) -> None:
        raise NotImplementedError("async def putDataById is not implemented")

    async def _fbthrift__handler_putDataById(self, args: folly.iobuf.IOBuf, protocol: Protocol) -> folly.iobuf.IOBuf:
        args_struct = deserialize(module.lite_types._fbthrift_MyService_putDataById_args, args, protocol)
        value = await self.putDataById(args_struct.id,args_struct.data,)
        return_struct = module.lite_types._fbthrift_MyService_putDataById_result()

        return serialize_iobuf(return_struct, protocol)


    async def hasDataById(
            self,
            id: int
        ) -> bool:
        raise NotImplementedError("async def hasDataById is not implemented")

    async def _fbthrift__handler_hasDataById(self, args: folly.iobuf.IOBuf, protocol: Protocol) -> folly.iobuf.IOBuf:
        args_struct = deserialize(module.lite_types._fbthrift_MyService_hasDataById_args, args, protocol)
        value = await self.hasDataById(args_struct.id,)
        return_struct = module.lite_types._fbthrift_MyService_hasDataById_result(success=value)

        return serialize_iobuf(return_struct, protocol)


    async def getDataById(
            self,
            id: int
        ) -> str:
        raise NotImplementedError("async def getDataById is not implemented")

    async def _fbthrift__handler_getDataById(self, args: folly.iobuf.IOBuf, protocol: Protocol) -> folly.iobuf.IOBuf:
        args_struct = deserialize(module.lite_types._fbthrift_MyService_getDataById_args, args, protocol)
        value = await self.getDataById(args_struct.id,)
        return_struct = module.lite_types._fbthrift_MyService_getDataById_result(success=value)

        return serialize_iobuf(return_struct, protocol)


    async def deleteDataById(
            self,
            id: int
        ) -> None:
        raise NotImplementedError("async def deleteDataById is not implemented")

    async def _fbthrift__handler_deleteDataById(self, args: folly.iobuf.IOBuf, protocol: Protocol) -> folly.iobuf.IOBuf:
        args_struct = deserialize(module.lite_types._fbthrift_MyService_deleteDataById_args, args, protocol)
        value = await self.deleteDataById(args_struct.id,)
        return_struct = module.lite_types._fbthrift_MyService_deleteDataById_result()

        return serialize_iobuf(return_struct, protocol)


    async def lobDataById(
            self,
            id: int,
            data: str
        ) -> None:
        raise NotImplementedError("async def lobDataById is not implemented")

    @oneway
    async def _fbthrift__handler_lobDataById(self, args: folly.iobuf.IOBuf, protocol: Protocol) -> None:
        args_struct = deserialize(module.lite_types._fbthrift_MyService_lobDataById_args, args, protocol)
        value = await self.lobDataById(args_struct.id,args_struct.data,)


class DbMixedStackArgumentsInterface(
    ServiceInterface,
    metaclass=ABCMeta
):

    @staticmethod
    def service_name() -> bytes:
        return b"DbMixedStackArguments"

    # pyre-ignore[3]: it can return anything
    def getFunctionTable(self) -> _typing.Mapping[bytes, _typing.Callable[..., _typing.Any]]:
        functionTable = {
            b"getDataByKey0": self._fbthrift__handler_getDataByKey0,
            b"getDataByKey1": self._fbthrift__handler_getDataByKey1,
        }
        return {**super().getFunctionTable(), **functionTable}



    async def getDataByKey0(
            self,
            key: str
        ) -> bytes:
        raise NotImplementedError("async def getDataByKey0 is not implemented")

    async def _fbthrift__handler_getDataByKey0(self, args: folly.iobuf.IOBuf, protocol: Protocol) -> folly.iobuf.IOBuf:
        args_struct = deserialize(module.lite_types._fbthrift_DbMixedStackArguments_getDataByKey0_args, args, protocol)
        value = await self.getDataByKey0(args_struct.key,)
        return_struct = module.lite_types._fbthrift_DbMixedStackArguments_getDataByKey0_result(success=value)

        return serialize_iobuf(return_struct, protocol)


    async def getDataByKey1(
            self,
            key: str
        ) -> bytes:
        raise NotImplementedError("async def getDataByKey1 is not implemented")

    async def _fbthrift__handler_getDataByKey1(self, args: folly.iobuf.IOBuf, protocol: Protocol) -> folly.iobuf.IOBuf:
        args_struct = deserialize(module.lite_types._fbthrift_DbMixedStackArguments_getDataByKey1_args, args, protocol)
        value = await self.getDataByKey1(args_struct.key,)
        return_struct = module.lite_types._fbthrift_DbMixedStackArguments_getDataByKey1_result(success=value)

        return serialize_iobuf(return_struct, protocol)

