/*
 * Copyright 2019-present Facebook, Inc.
 *
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
	"context"
	"crypto/tls"
	"net"
)

type contextKey int

const (
	connInfoKey contextKey = iota
)

// ConnInfo contains connection information from clients of the SimpleServer.
type ConnInfo struct {
	LocalAddr  net.Addr
	RemoteAddr net.Addr

	netConn  net.Conn             // set by thrift tcp servers
	tlsState *tls.ConnectionState // set by thrift http servers
}

// String implements the fmt.Stringer interface.
func (c ConnInfo) String() string {
	return c.RemoteAddr.String() + " -> " + c.LocalAddr.String()
}

// tlsConnectionStater is an abstract interface for types that can return
// the state of TLS connections. This is used to support not only tls.Conn
// but also custom wrappers such as permissive TLS/non-TLS sockets.
//
// Caveat: this interface has to support at least tls.Conn, which has
// the current signature for ConnectionState. Because of that, wrappers
// for permissive TLS/non-TLS may return an empty tls.ConnectionState.
type tlsConnectionStater interface {
	ConnectionState() tls.ConnectionState
}

// TLS returns the TLS connection state.
func (c ConnInfo) TLS() *tls.ConnectionState {
	if c.tlsState != nil {
		return c.tlsState
	}
	tlsConn, ok := c.netConn.(tlsConnectionStater)
	if !ok {
		return nil
	}
	cs := tlsConn.ConnectionState()
	// See the caveat in tlsConnectionStater.
	if cs.Version == 0 {
		return nil
	}
	return &cs
}

// ConnInfoFromContext extracts and returns ConnInfo from context.
func ConnInfoFromContext(ctx context.Context) (ConnInfo, bool) {
	v, ok := ctx.Value(connInfoKey).(ConnInfo)
	return v, ok
}
