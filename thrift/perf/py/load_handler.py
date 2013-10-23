#!/usr/local/bin/python2.6 -tt
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#
import time

from twisted.internet.defer import Deferred
from twisted.internet import reactor
import zope.interface

from apache.thrift.test.twisted.load.ttypes import LoadError
from apache.thrift.test.twisted.load import LoadTest


def us_to_sec(microseconds):
    return float(microseconds) / 1000000


class AsyncCall(object):
    """
    A class to represent an outstanding asynchronous thrift call currently
    being processed by the handler.

    The main purpose of this class is so that the handler can track all pending
    calls, and cancel them when AsyncLoadHandler.shutdown() is called.
    """
    def __init__(self, handler):
        self.deferred = Deferred()
        self.handler = handler

    def callback(self, *args, **kwargs):
        self.deferred.callback(*args, **kwargs)
        self._done()

    def errback(self, *args, **kwargs):
        self.deferred.errback(*args, **kwargs)
        self._done()

    def start(self):
        raise NotImplementedError()

    def cancel(self):
        raise NotImplementedError()

    def _done(self):
        self.handler.async_call_done(self)


class AsyncSleepCall(AsyncCall):
    """
    An AsyncCall that sleeps for the specified amount of time, then completes.
    """
    def __init__(self, handler, seconds):
        AsyncCall.__init__(self, handler)
        self.seconds = seconds

    def start(self):
        self.timeout = reactor.callLater(self.seconds, self.callback, None)

    def cancel(self):
        self.timeout.cancel()


class CpuBurner(AsyncCall):
    """
    An AsyncCall that burns CPU cycles for the specified amount of time before
    completing.

    In order to play nicely with the twisted reactor, and not hog the CPU for
    too long, it only burns for at most max_burn_at_once seconds before
    yielding and returning back to the reactor loop.  It will repeat the next
    time around the reactor loop, for as many times as necessary to meet the
    requested burn duration.
    """
    def __init__(self, handler, seconds, max_burn_at_once):
        AsyncCall.__init__(self, handler)
        self.time_left = seconds
        self.max_burn_at_once = max_burn_at_once

        self.timeout = None

    def start(self):
        self.run()

    def cancel(self):
        self.timeout.cancel()

    def run(self):
        start = now = time.time()
        end = now + min(self.time_left, self.max_burn_at_once)
        while True:
            now = time.time()
            if now > end:
                break

        time_burned = now - start
        if time_burned > self.time_left:
            self.callback(None)
        else:
            # Yield to the reactor loop so other events can be processed.
            # Reschedule another call to run() for the next time around the
            # reactor loop.
            self.time_left -= time_burned
            self.timeout = reactor.callLater(0, self.run)


class AsyncLoadHandler(object):
    """
    A twisted handler implementation of the LoadTest service interface.
    """
    zope.interface.implements(LoadTest.Iface)

    def __init__(self):
        self.max_burn_at_once = 0.005
        self.pending_calls = set()

    def shutdown(self):
        for call in self.pending_calls:
            call.cancel()
        self.pending_calls = set()

    def async_call_done(self, call):
        self.pending_calls.remove(call)

    def new_async_call(self, call):
        self.pending_calls.add(call)
        call.start()
        return call.deferred

    def noop(self):
        pass

    def onewayNoop(self):
        pass

    def asyncNoop(self):
        return self.new_async_call(AsyncSleepCall(self, 0))

    def sleep(self, microseconds):
        seconds = us_to_sec(microseconds)
        return self.new_async_call(AsyncSleepCall(self, seconds))

    def onewaySleep(self, microseconds):
        seconds = us_to_sec(microseconds)
        return self.new_async_call(AsyncSleepCall(self, seconds))

    def burn(self, microseconds):
        burner = CpuBurner(self, us_to_sec(microseconds),
                           self.max_burn_at_once)
        return self.new_async_call(burner)

    def onewayBurn(self, microseconds):
        return self.burn(microseconds)

    def badSleep(self, microseconds):
        time.sleep(us_to_sec(microseconds))

    def badBurn(self, microseconds):
        now = time.time()
        end = now + us_to_sec(microseconds)
        while True:
            if time.time() > end:
                break

    def throwError(self, code):
        raise LoadError(code=code)

    def throwUnexpected(self, code):
        raise LoadError(code=code)

    def onewayThrow(self, code):
        raise LoadError(code=code)

    def send(self, data):
        pass

    def onewaySend(self, data):
        pass

    def recv(self, bytes):
        return 'a' * bytes

    def sendrecv(self, data, recvBytes):
        return 'a' * recvBytes

    def echo(self, data):
        return data

    def add(self, a, b):
        return a + b

class AsyncContextLoadHandler(object):
    """
    A twisted handler implementation of the LoadTest service interface.
    """
    zope.interface.implements(LoadTest.ContextIface)

    def __init__(self):
        self.alh = AsyncLoadHandler()

    def noop(self, handler_ctx):
        self.alh.noop()

    def onewayNoop(self, handler_ctx):
        self.alh.onewayNoop()

    def asyncNoop(self, handler_ctx):
        return self.alh.asyncNoop()

    def sleep(self, handler_ctx, microseconds):
        return self.alh.sleep(microseconds)

    def onewaySleep(self, handler_ctx, microseconds):
        return self.alh.onewaySleep(microseconds)

    def burn(self, handler_ctx, microseconds):
        return self.alh.burn(microseconds)

    def onewayBurn(self, handler_ctx, microseconds):
        return self.alh.onewayBurn(microseconds)

    def badSleep(self, handler_ctx, microseconds):
        self.alh.badSleep(microseconds)

    def badBurn(self, handler_ctx, microseconds):
        self.alh.badBurn(microseconds)

    def throwError(self, handler_ctx, code):
        self.alh.throwError(code)

    def throwUnexpected(self, handler_ctx, code):
        self.alh.throwUnexpected(code)

    def onewayThrow(self, handler_ctx, code):
        self.alh.onewayThrow(code)

    def send(self, handler_ctx, data):
        self.alh.send(data)

    def onewaySend(self, handler_ctx, data):
        self.alh.onewaySend(data)

    def recv(self, handler_ctx, bytes):
        self.alh.recv(bytes)

    def sendrecv(self, handler_ctx, data, recvBytes):
        self.alh.sendrecv(data, recvBytes)

    def echo(self, handler_ctx, data):
        self.alh.echo(data)

    def add(self, handler_ctx, a, b):
        self.alh.add(a, b)
