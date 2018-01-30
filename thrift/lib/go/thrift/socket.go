/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

package thrift

import (
	"net"
	"time"
)

type Socket struct {
	conn    net.Conn
	addr    net.Addr
	timeout time.Duration
}

// NewSocket creates a net.Conn-backed Transport, given a host and port
//
// Example:
// 	trans, err := thrift.NewSocket("localhost:9090")
func NewSocket(hostPort string) (*Socket, error) {
	return NewSocketTimeout(hostPort, 0)
}

// NewSocketTimeout creates a net.Conn-backed Transport, given a host and port
// it also accepts a timeout as a time.Duration
func NewSocketTimeout(hostPort string, timeout time.Duration) (*Socket, error) {
	//conn, err := net.DialTimeout(network, address, timeout)
	addr, err := net.ResolveTCPAddr("tcp6", hostPort)
	if err != nil {
		addr, err = net.ResolveTCPAddr("tcp", hostPort)
		if err != nil {
			return nil, err
		}
	}
	return NewSocketFromAddrTimeout(addr, timeout), nil
}

// Creates a Socket from a net.Addr
func NewSocketFromAddrTimeout(addr net.Addr, timeout time.Duration) *Socket {
	return &Socket{addr: addr, timeout: timeout}
}

// Creates a Socket from an existing net.Conn
func NewSocketFromConnTimeout(conn net.Conn, timeout time.Duration) *Socket {
	return &Socket{conn: conn, addr: conn.RemoteAddr(), timeout: timeout}
}

// Sets the socket timeout
func (p *Socket) SetTimeout(timeout time.Duration) error {
	p.timeout = timeout
	return nil
}

func (p *Socket) pushDeadline(read, write bool) {
	var t time.Time
	if p.timeout > 0 {
		t = time.Now().Add(time.Duration(p.timeout))
	}
	if read && write {
		p.conn.SetDeadline(t)
	} else if read {
		p.conn.SetReadDeadline(t)
	} else if write {
		p.conn.SetWriteDeadline(t)
	}
}

// Connects the socket, creating a new socket object if necessary.
func (p *Socket) Open() error {
	if p.IsOpen() {
		return NewTransportException(ALREADY_OPEN, "Socket already connected.")
	}
	if p.addr == nil {
		return NewTransportException(NOT_OPEN, "Cannot open nil address.")
	}
	if len(p.addr.Network()) == 0 {
		return NewTransportException(NOT_OPEN, "Cannot open bad network name.")
	}
	if len(p.addr.String()) == 0 {
		return NewTransportException(NOT_OPEN, "Cannot open bad address.")
	}
	var err error
	if p.conn, err = net.DialTimeout(p.addr.Network(), p.addr.String(), p.timeout); err != nil {
		return NewTransportException(NOT_OPEN, err.Error())
	}
	return nil
}

// Retrieve the underlying net.Conn
func (p *Socket) Conn() net.Conn {
	return p.conn
}

// Returns true if the connection is open
func (p *Socket) IsOpen() bool {
	if p.conn == nil {
		return false
	}
	return true
}

// Closes the socket.
func (p *Socket) Close() error {
	// Close the socket
	if p.conn != nil {
		err := p.conn.Close()
		if err != nil {
			return err
		}
		p.conn = nil
	}
	return nil
}

//Returns the remote address of the socket.
func (p *Socket) Addr() net.Addr {
	return p.addr
}

func (p *Socket) Read(buf []byte) (int, error) {
	if !p.IsOpen() {
		return 0, NewTransportException(NOT_OPEN, "Connection not open")
	}
	p.pushDeadline(true, false)
	n, err := p.conn.Read(buf)
	return n, NewTransportExceptionFromError(err)
}

func (p *Socket) Write(buf []byte) (int, error) {
	if !p.IsOpen() {
		return 0, NewTransportException(NOT_OPEN, "Connection not open")
	}
	p.pushDeadline(false, true)
	return p.conn.Write(buf)
}

func (p *Socket) Flush() error {
	return nil
}

func (p *Socket) Interrupt() error {
	if !p.IsOpen() {
		return nil
	}
	return p.conn.Close()
}

func (p *Socket) RemainingBytes() (num_bytes uint64) {
	const maxSize = ^uint64(0)
	return maxSize // the thruth is, we just don't know unless framed is used
}
