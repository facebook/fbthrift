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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals

import socket
import ssl

from thrift.transport.TSocketOverHttpTunnel import TSocketOverHttpTunnel
from thrift.transport.TTransport import TTransportException

class TSSLSocketOverHttpTunnel(TSocketOverHttpTunnel):
    def __init__(self, host, port, proxy_host, proxy_port,
                 ssl_version=ssl.PROTOCOL_TLSv1,
                 cert_reqs=ssl.CERT_NONE,
                 ca_certs=None,
                 keyfile=None,
                 certfile=None):
        TSocketOverHttpTunnel.__init__(self, host, port, proxy_host, proxy_port)
        self.ssl_version = ssl_version
        self.cert_reqs = cert_reqs
        self.keyfile, self.certfile, self.ca_certs = \
                keyfile, certfile, ca_certs

    def open(self):
        TSocketOverHttpTunnel.open(self)
        try:
            sslh = ssl.SSLSocket(self.handle,
                                 ssl_version=self.ssl_version,
                                 cert_reqs=self.cert_reqs,
                                 keyfile=self.keyfile,
                                 certfile=self.certfile,
                                 ca_certs=self.ca_certs)
            self.handle = sslh
        except ssl.SSLError as e:
            self.close()
            raise TTransportException(TTransportException.NOT_OPEN,
                    "SSL error during handshake: " + str(e))
        except socket.error as e:
            self.close()
            raise TTransportException(TTransportException.NOT_OPEN,
                    "socket error during SSL handshake: " + str(e))
