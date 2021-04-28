/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package thrift

import (
	"context"
	"fmt"
)

type ClientConn interface {
	Transport() Transport
	Open() error
	Close() error
	IsOpen() bool
	SendMsg(method string, req IRequest, msgType MessageType) error
	RecvMsg(method string, res IResponse) error
}

type ClientConnContext interface {
	Transport() Transport
	Open() error
	Close() error
	IsOpen() bool
	SendMsg(ctx context.Context, method string, req IRequest, msgType MessageType) error
	RecvMsg(ctx context.Context, method string, res IResponse) error
}

// clientConn holds all the connection information for a thrift client
type clientConn struct {
	transport       Transport
	protocolFactory ProtocolFactory
	iproto          Protocol
	oproto          Protocol
	seqID           int32
}

type clientConnContext struct {
	*clientConn
}

// Transport returns the underlying Transport object inside the clientConn
// object
func (cc *clientConn) Transport() Transport {
	return cc.transport
}

// NewClientConn creates a new ClientConn object using the provided ProtocolFactory
func NewClientConn(t Transport, pf ProtocolFactory) ClientConn {
	return &clientConn{
		transport:       t,
		protocolFactory: pf,
		iproto:          pf.GetProtocol(t),
		oproto:          pf.GetProtocol(t),
	}
}

// NewClientConnContext creates a new ClientConnContext object using the provided ProtocolFactory
func NewClientConnContext(t Transport, pf ProtocolFactory) ClientConnContext {
	return &clientConnContext{
		clientConn: NewClientConn(t, pf).(*clientConn),
	}
}

// NewClientConnWithProtocols creates a new ClientConn object using the input and output protocols provided
func NewClientConnWithProtocols(t Transport, iproto, oproto Protocol) ClientConn {
	return &clientConn{
		transport:       t,
		protocolFactory: nil,
		iproto:          iproto,
		oproto:          oproto,
	}
}

// NewClientConnContextWithProtocols creates a new ClientConnContext object using the input and output protocols provided
func NewClientConnContextWithProtocols(t Transport, iproto, oproto Protocol) ClientConnContext {
	return &clientConnContext{
		clientConn: NewClientConnWithProtocols(t, iproto, oproto).(*clientConn),
	}
}

// IRequest represents a request to be sent to a thrift endpoint
type IRequest interface {
	Write(p Protocol) error
}

// IResponse represents a response received from a thrift call
type IResponse interface {
	Read(p Protocol) error
}

// Open opens the client connection
func (cc *clientConn) Open() error {
	return cc.transport.Open()
}

// Close closes the client connection
func (cc *clientConn) Close() error {
	return cc.transport.Close()
}

// IsOpen return true if the client connection is open; otherwise, it returns false.
func (cc *clientConn) IsOpen() bool {
	return cc.transport.IsOpen()
}

// SendMsg sends a request to a given thrift endpoint
func (cc *clientConn) SendMsg(method string, req IRequest, msgType MessageType) error {
	cc.seqID++

	if err := cc.oproto.WriteMessageBegin(method, msgType, cc.seqID); err != nil {
		return err
	}

	if err := req.Write(cc.oproto); err != nil {
		return err
	}

	if err := cc.oproto.WriteMessageEnd(); err != nil {
		return err
	}

	return cc.oproto.Flush()
}

// RecvMsg receives the response from a call to a thrift endpoint
func (cc *clientConn) RecvMsg(method string, res IResponse) error {
	recvMethod, mTypeID, seqID, err := cc.iproto.ReadMessageBegin()

	if err != nil {
		return err
	}

	if method != recvMethod {
		return NewApplicationException(WRONG_METHOD_NAME, fmt.Sprintf("%s failed: wrong method name", method))
	}

	if cc.seqID != seqID {
		return NewApplicationException(BAD_SEQUENCE_ID, fmt.Sprintf("%s failed: out of sequence response", method))
	}

	switch mTypeID {
	case REPLY:
		if err := res.Read(cc.iproto); err != nil {
			return err
		}

		return cc.iproto.ReadMessageEnd()
	case EXCEPTION:
		err := NewApplicationException(UNKNOWN_APPLICATION_EXCEPTION, "Unknown exception")

		recvdErr, readErr := err.Read(cc.iproto)

		if readErr != nil {
			return readErr
		}

		if msgEndErr := cc.iproto.ReadMessageEnd(); msgEndErr != nil {
			return msgEndErr
		}
		return recvdErr
	default:
		return NewApplicationException(INVALID_MESSAGE_TYPE_EXCEPTION, fmt.Sprintf("%s failed: invalid message type", method))
	}
}

func (cc *clientConnContext) SendMsg(ctx context.Context, method string, req IRequest, msgType MessageType) error {
	if p, ok := cc.oproto.(*HeaderProtocol); ok {
		for k, v := range HeadersFromContext(ctx) {
			p.SetHeader(k, v)
		}
	}
	return cc.clientConn.SendMsg(method, req, msgType)
}

func (cc *clientConnContext) RecvMsg(ctx context.Context, method string, res IResponse) error {
	return cc.clientConn.RecvMsg(method, res)
}
