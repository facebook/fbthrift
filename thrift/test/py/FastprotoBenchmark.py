from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

from thrift.protocol import fastproto, TBinaryProtocol, TCompactProtocol
from thrift.transport import TTransport

import timeit
import gc
try:
    from guppy import hpy
except:
    hpy = None

from FastProto.ttypes import AStruct, OneOfEach

ooe = OneOfEach()
ooe.aBool = True
ooe.aByte = 1
ooe.anInteger16 = 234
ooe.anInteger32 = 2345678
ooe.anInteger64 = 23456789012345
ooe.aString = "This is my rifle" * 100
ooe.aDouble = 2.3456789012
ooe.aFloat = 12345.678
ooe.aList = [12, 34, 56, 78, 90, 100, 123, 456, 789]
ooe.aSet = set(["This", "is", "my", "rifle"])
ooe.aMap = {"What": 4, "a": 1, "wonderful": 9, "day": 3, "!": 1}
ooe.aStruct = AStruct(aString="isn't it?", anInteger=999)

trans = TTransport.TMemoryBuffer()
proto = TBinaryProtocol.TBinaryProtocol(trans)
ooe.write(proto)
binary_buf = trans.getvalue()

trans = TTransport.TMemoryBuffer()
proto = TCompactProtocol.TCompactProtocol(trans)
ooe.write(proto)
compact_buf = trans.getvalue()

class TDevNullTransport(TTransport.TTransportBase):
    def __init__(self):
        pass

    def isOpen(self):
        return True

iters = 100000

def benchmark_fastbinary():
    setup_write = """
from __main__ import ooe, TDevNullTransport
from thrift.protocol import TBinaryProtocol

trans = TDevNullTransport()
proto = TBinaryProtocol.TBinaryProtocol{}(trans)
"""
    print("Standard write = {}".format(
        timeit.Timer('ooe.write(proto)', setup_write.format(""))
            .timeit(number=iters)))
    print("Fastbinary write = {}".format(
        timeit.Timer('ooe.write(proto)', setup_write.format("Accelerated"))
            .timeit(number=iters)))

    setup_read = """
from __main__ import binary_buf
from FastProto.ttypes import OneOfEach
from thrift.protocol import TBinaryProtocol
from thrift.transport import TTransport

def doRead():
    trans = TTransport.TMemoryBuffer(binary_buf)
    proto = TBinaryProtocol.TBinaryProtocol{}(trans)
    ooe = OneOfEach()
    ooe.read(proto)
"""
    print("Standard read = {}".format(
        timeit.Timer('doRead()', setup_read.format(""))
            .timeit(number=iters)))
    print("Fastbinary read = {}".format(
        timeit.Timer('doRead()', setup_read.format("Accelerated"))
            .timeit(number=iters)))

def benchmark_fastproto():
    setup_write = """
from __main__ import ooe, TDevNullTransport
from FastProto.ttypes import OneOfEach
from thrift.protocol import fastproto

trans = TDevNullTransport()
def doWrite():
    buf = fastproto.encode(ooe, [OneOfEach, OneOfEach.thrift_spec, False],
        utf8strings=0, protoid={0})
    trans.write(buf)
"""
    print("Fastproto binary write = {}".format(
        timeit.Timer('doWrite()', setup_write.format(0))
            .timeit(number=iters)))
    print("Fastproto compact write = {}".format(
        timeit.Timer('doWrite()', setup_write.format(2))
            .timeit(number=iters)))

    setup_read = """
from __main__ import binary_buf, compact_buf
from FastProto.ttypes import OneOfEach
from thrift.protocol import fastproto
from thrift.transport import TTransport

def doReadBinary():
    trans = TTransport.TMemoryBuffer(binary_buf)
    ooe = OneOfEach()
    fastproto.decode(ooe, trans, [OneOfEach, OneOfEach.thrift_spec, False],
        utf8strings=0, protoid=0)

def doReadCompact():
    trans = TTransport.TMemoryBuffer(compact_buf)
    ooe = OneOfEach()
    fastproto.decode(ooe, trans, [OneOfEach, OneOfEach.thrift_spec, False],
        utf8strings=0, protoid=2)
"""
    print("Fastproto binary read = {}".format(
        timeit.Timer("doReadBinary()", setup_read).timeit(number=iters)))
    print("Fastproto compact read = {}".format(
        timeit.Timer("doReadCompact()", setup_read).timeit(number=iters)))

def memory_usage_fastproto():
    hp = hpy()
    trans = TDevNullTransport()
    global ooe
    for pid in (0, 2):
        before = hp.heap()
        for i in range(iters):
            buf = fastproto.encode(
                    ooe,
                    [OneOfEach, OneOfEach.thrift_spec, False],
                    utf8strings=0,
                    protoid=pid)
            trans.write(buf)
        gc.collect()
        after = hp.heap()
        leftover = after - before
        print("Memory leftover after running fastproto.encode with "
                "protocol id {0} for {1} times".format(pid, iters))
        print(leftover)

    for pid in (0, 2):
        before = hp.heap()
        for i in range(iters):
            trans = TTransport.TMemoryBuffer(
                    binary_buf if pid == 0 else compact_buf)
            ooe_local = OneOfEach()
            fastproto.decode(
                    ooe_local,
                    trans,
                    [OneOfEach, OneOfEach.thrift_spec, False],
                    utf8strings=0,
                    protoid=pid)
        gc.collect()
        after = hp.heap()
        leftover = after - before
        print("Memory leftover after running fastproto.decode with "
                "protocol id {0} for {1} times".format(pid, iters))
        print(leftover)

if __name__ == "__main__":
    print("Starting Benchmarks")
    benchmark_fastbinary()
    benchmark_fastproto()
    if hpy is not None:
        memory_usage_fastproto()
