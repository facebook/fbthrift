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
import gevent

from apache.thrift.test.load.ttypes import LoadError
from apache.thrift.test.load import LoadTest

def us_to_sec(microseconds):
    return float(microseconds) / 1000000

class LoadHandler(object):
    def __init__(self):
        pass

    def noop(self):
        pass

    def onewayNoop(self):
        pass

    def asyncNoop(self):
        pass

    def sleep(self, us):
        time.sleep(us_to_sec(us))

    def onewaySleep(self, us):
        self.sleep(us)

    def burn(self, us):
        start = now = time.time()
        end = now + us
        while True:
            now = time.time()
            if now > end:
                break

    def onewayBurn(self, us):
        self.burn(us)

    def badSleep(self, us):
        self.sleep(us)

    def badBurn(self, us):
        self.burn(us)

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

class GeventLoadHandler(LoadHandler):
    def asyncNoop(self):
        gevent.spawn(self.noop)

    def gevent_sleep(self, us):
        gevent.sleep(us_to_sec(us))

    def sleep(self, us):
        # Note that spawning a new greenlet here allows
        # multiple requests per connection to occur simultaneously.
        # TODO(davejwatson) could modify the generated code in
        # process() to do this automagically.
        gevent.spawn(self.gevent_sleep, us)

    def badSleep(self, us):
        self.gevent_sleep(us)

    def gevent_burn(self, us):
        start = now = time.time()
        end = now + us_to_sec(us)
        while True:
            now = time.time()
            if now > end:
                break
            gevent.sleep(0)  # yield

    def burn(self, us):
        # see note in sleep
        gevent.spawn(self.gevent_burn, us)

    def badBurn(self, us):
        start = now = time.time()
        end = now + us_to_sec(us)
        while True:
            now = time.time()
            if now > end:
                break
