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

from __future__ import absolute_import
from __future__ import division
from __future__ import print_function
from __future__ import unicode_literals


from .TTransport import TTransportBase, TTransportException, \
        TServerTransportBase
import os
import sys
import errno
import select
import socket
import fcntl
import warnings

class ConnectionEpoll:
    """ epoll is preferred over select due to its efficiency and ability to
        handle more than 1024 simultaneous connections """

    def __init__(self):
        self.epoll = select.epoll()
        # TODO should we set any other masks?
        # http://docs.python.org/library/select.html#epoll-objects
        self.READ_MASK = select.EPOLLIN | select.EPOLLPRI
        self.WRITE_MASK = select.EPOLLOUT
        self.ERR_MASK = select.EPOLLERR | select.EPOLLHUP

    def read(self, fileno):
        self.epoll.register(
            fileno, self.READ_MASK | self.ERR_MASK
        )

    def write(self, fileno):
        self.epoll.register(fileno, self.WRITE_MASK)

    def process(self, timeout):
        msgs = self.epoll.poll(timeout=(float(timeout or -1)))

        rset = []
        wset = []
        xset = []
        for fd, mask in msgs:
            if mask & self.READ_MASK:
                rset.append(fd)
            if mask & self.WRITE_MASK:
                wset.append(fd)
            if mask & self.ERR_MASK:
                xset.append(fd)

        return rset, wset, xset


class ConnectionSelect:
    def __init__(self):
        self.readable = []
        self.writable = []

    def read(self, fileno):
        self.readable.append(fileno)

    def write(self, fileno):
        self.writable.append(fileno)

    def process(self, timeout):
        return select.select(
            self.readable, self.writable, self.readable, timeout
        )


class TSocketBase(TTransportBase):
  """Base class for both connected and listening sockets"""
  def __init__(self):
    self.handles = {}

  def _resolveAddr(self, family=None):
    if family is None:
      family = socket.AF_UNSPEC
    if self._unix_socket is not None:
      return [(socket.AF_UNIX, socket.SOCK_STREAM, None, None,
               self._unix_socket)]
    else:
      return socket.getaddrinfo(self.host, self.port, family,
                                socket.SOCK_STREAM, 0,
                                socket.AI_PASSIVE | socket.AI_ADDRCONFIG)

  def close(self):
    klist = self.handles.keys() if sys.version_info[0] < 3 else \
        list(self.handles.keys())
    for key in klist:
      self.handles[key].close()
      del self.handles[key]

  def getSocketName(self):
    if not self.handles:
      raise TTransportException(TTransportException.NOT_OPEN,
          'Transport not open')
    return self.handles.values()[0].getsockname()

  def fileno(self):
    if not self.handles:
      raise TTransportException(TTransportException.NOT_OPEN,
          'Transport not open')
    if sys.version_info[0] >= 3:
      return list(self.handles.values())[0].fileno()
    else:
      return self.handles.values()[0].fileno()

  def setCloseOnExec(self, closeOnExec):
    self.close_on_exec = closeOnExec
    for handle in self.handles.values():
      self._setHandleCloseOnExec(handle)

  def _setHandleCloseOnExec(self, handle):
    flags = fcntl.fcntl(handle, fcntl.F_GETFD, 0)
    if flags < 0:
      raise IOError('Error in retrieving file options')
    if self.close_on_exec:
      fcntl.fcntl(handle, fcntl.F_SETFD, flags | fcntl.FD_CLOEXEC)
    else:
      fcntl.fcntl(handle, fcntl.F_SETFD, flags & ~fcntl.FD_CLOEXEC)


class TSocket(TSocketBase):
  """Connection Socket implementation of TTransport base."""

  def __init__(self, host='localhost', port=9090, unix_socket=None):
    """Initialize a TSocket

    @param host(str)  The host to connect to.
    @param port(int)  The (TCP) port to connect to.
    @param unix_socket(str)  The filename of a unix socket to connect to.
                             (host and port will be ignored.)
    """
    TSocketBase.__init__(self)
    self.host = host
    self.port = port
    self.handle = None
    self._unix_socket = unix_socket
    self._timeout = None
    self.close_on_exec = True

  def setHandle(self, h):
    self.handle = h
    self.handles[h.fileno()] = h

  def getHandle(self):
    return self.handle

  def close(self):
    TSocketBase.close(self)
    self.handle = None

  def isOpen(self):
    return self.handle != None

  def setTimeout(self, ms):
    if ms is None:
      self._timeout = None
    else:
      self._timeout = ms / 1000.0

    if self.handle != None:
      self.handle.settimeout(self._timeout)

  def getPeerName(self):
    if not self.handle:
      raise TTransportException(TTransportException.NOT_OPEN,
          'Transport not open')
    return self.handle.getpeername()

  def open(self):
    try:
      res0 = self._resolveAddr()
      for res in res0:
        handle = socket.socket(res[0], res[1])
        self.setHandle(handle)
        handle.settimeout(self._timeout)
        self.setCloseOnExec(self.close_on_exec)
        try:
          handle.connect(res[4])
        except socket.error as e:
          if res is not res0[-1]:
            continue
          else:
            raise e
        break
    except socket.error as e:
      if self._unix_socket:
        message = 'Could not connect to socket %s' % self._unix_socket
      else:
        message = 'Could not connect to %s:%d' % (self.host, self.port)
      raise TTransportException(TTransportException.NOT_OPEN, message)

  def read(self, sz):
    buff = self.handle.recv(sz)
    if len(buff) == 0:
      raise TTransportException(TTransportException.UNKNOWN,
                                'TSocket read 0 bytes')
    return buff

  def write(self, buff):
    if not self.handle:
      raise TTransportException(TTransportException.NOT_OPEN, 'Transport not open')
    sent = 0
    have = len(buff)
    while sent < have:
      try:
        plus = self.handle.send(buff)
      except socket.error as e:
        message = 'Socket send failed with error %s (%s)' %(e.errno, e.strerror)
        raise TTransportException(TTransportException.UNKNOWN, message)
      assert plus > 0
      sent += plus
      buff = buff[plus:]

  def flush(self):
    pass

class TServerSocket(TSocketBase, TServerTransportBase):
  """Socket implementation of TServerTransport base."""

  def __init__(self, port=9090, unix_socket=None, family=None, backlog=128):
    TSocketBase.__init__(self)
    self.host = None
    self.port = port
    self._unix_socket = unix_socket
    self.family = family
    self.tcp_backlog = backlog
    self.close_on_exec = True

    # Since we now rely on select() by default to do accepts across multiple
    # socket fds, we can receive two connections concurrently.  In order to
    # maintain compatibility with the existing .accept() API, we need to keep
    # track of the accept backlog.
    self._queue = []

  def getSocketName(self):
    warnings.warn('getSocketName() is deprecated for TServerSocket.  '
                  'Please use getSocketNames() instead.')
    return TSocketBase.getSocketName(self)

  def getSocketNames(self):
    return [handle.getsockname() for handle in self.handles.values()]

  def fileno(self):
    warnings.warn('fileno() is deprecated for TServerSocket.  '
                  'Please use filenos() instead.')
    return TSocketBase.fileno(self)

  def filenos(self):
    return [handle.fileno() for handle in self.handles.values()]

  def _cleanup_unix_socket(self, addrinfo):
    tmp = socket.socket(addrinfo[0], addrinfo[1])
    try:
      tmp.connect(addrinfo[4])
    except socket.error as err:
      eno, message = err.args
      if eno == errno.ECONNREFUSED:
        os.unlink(addrinfo[4])

  def listen(self):
    res0 = self._resolveAddr(self.family)

    for res in res0:
      if res[0] == socket.AF_INET6 and res[4][0] == socket.AF_INET6:
        # This happens if your version of python was built without IPv6
        # support.  getaddrinfo() will return IPv6 addresses, but the
        # contents of the address field are bogus.
        # (For example, see http://bugs.python.org/issue8858)
        #
        # Ignore IPv6 addresses if python doesn't have IPv6 support.
        continue

      # We need remove the old unix socket if the file exists and
      # nobody is listening on it.
      if self._unix_socket:
        self._cleanup_unix_socket(res)

      handle = socket.socket(res[0], res[1])
      handle.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
      self._setHandleCloseOnExec(handle)

      # Always set IPV6_V6ONLY for IPv6 sockets
      if res[0] == socket.AF_INET6:
        handle.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_V6ONLY, True)

      handle.settimeout(None)
      handle.bind(res[4])
      handle.listen(self.tcp_backlog)

      self.handles[handle.fileno()] = handle

    if not self.handles:
      raise TTransportException("No valid interfaces to listen on!")

  def _sock_accept(self):
    if self._queue:
      return self._queue.pop()

    if hasattr(select, "epoll"):
        poller = ConnectionEpoll()
    else:
        poller = ConnectionSelect()

    for filenos in self.handles.keys():
        poller.read(filenos)

    r, _, x = poller.process(0)

    for fd in r:
      self._queue.append(self.handles[fd].accept())

    if not self._queue:
      raise TTransportException("Accept interrupt without client?")

    return self._queue.pop()

  def accept(self):
    return self._makeTSocketFromAccepted(self._sock_accept())

  def _makeTSocketFromAccepted(self, accepted):
    client, addr = accepted
    result = TSocket()
    result.setHandle(client)
    return result
