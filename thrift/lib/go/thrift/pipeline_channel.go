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
	"sync"
)

// PipelineChannel is an RequestChannel which wraps an open Protocol that
// can support pipelining. The PipelineChannel allows for mutltiple outstanding
// requests from different threads.
type PipelineChannel struct {
	protocol Protocol
	ctx      context.Context
	seqID    int32
	lock     sync.Mutex
	futlock  sync.Mutex
	// TODO: Shard this map/lock to reduce lock contention
	pending map[int32]*requestFuture
}

// NewPipelineChannel creates a new pipeline channel. It will start a background goroutine
// to read and demux incoming messages. When the context is cancelled or the protocol is closed,
// the goroutine will stop and free associated resources with pending requests.
func NewPipelineChannel(ctx context.Context, protocol Protocol) RequestChannel {
	channel := &PipelineChannel{
		protocol: protocol,
		ctx:      ctx,
		seqID:    0,
		pending:  map[int32]*requestFuture{},
	}
	go func() {
		err := channel.recvLoop()
		// always cancel remaining futures when the loop is done
		channel.cancelFutures(err)
	}()
	return channel
}

// Close closes the client connection
func (c *PipelineChannel) Close() error {
	return c.protocol.Transport().Close()
}

// IsOpen return true if the client connection is open; otherwise, it returns false.
func (c *PipelineChannel) IsOpen() bool {
	return c.protocol.Transport().IsOpen()
}

// Open opens the client connection
func (c *PipelineChannel) Open() error {
	return c.protocol.Transport().Open()
}

// Call will call the given method with the given thrift struct, and read the response
// into the given response struct. It only allows multiple outstanding requests and is thread-safe.
func (c *PipelineChannel) Call(ctx context.Context, method string, request IRequest, response IResponse) error {

	c.lock.Lock()
	seqID, err := c.sendMsg(method, request, CALL)
	c.lock.Unlock()
	if err != nil {
		return err
	}

	// xxx: Race condition making this future after we finish sending the request?
	fut := &requestFuture{
		method:   method,
		seqID:    seqID,
		response: response,
		ctx:      ctx,
		done:     make(chan error, 1),
	}
	c.futlock.Lock()
	c.pending[seqID] = fut
	c.futlock.Unlock()

	select {
	case err := <-fut.done:
		return err
	case <-ctx.Done():
		return fmt.Errorf("request cancelled: %s", ctx.Err())
	}
}

// Oneway will call the given method with the given thrift struct. It returns immediately when the request is sent.
// It only allows multiple outstanding requests and is thread-safe.
func (c *PipelineChannel) Oneway(ctx context.Context, method string, request IRequest) error {
	c.lock.Lock()
	defer c.lock.Unlock()
	// No cancellation for oneway as we cannot reasonably cancel the call
	_, err := c.sendMsg(method, request, ONEWAY)
	return err
}

type requestFuture struct {
	method   string
	seqID    int32
	response IResponse
	done     chan error
	ctx      context.Context
}

func (c *PipelineChannel) sendMsg(method string, request IRequest, msgType MessageType) (int32, error) {
	c.seqID++
	seqID := c.seqID

	if err := c.protocol.WriteMessageBegin(method, msgType, seqID); err != nil {
		return seqID, err
	}

	if err := request.Write(c.protocol); err != nil {
		return seqID, err
	}

	if err := c.protocol.WriteMessageEnd(); err != nil {
		return seqID, err
	}

	if err := c.protocol.Flush(); err != nil {
		return seqID, err
	}

	return seqID, nil
}

func (c *PipelineChannel) recvOne(fut *requestFuture, recvMethod string, mTypeID MessageType) (err error) {
	defer func() {
		if r := recover(); r != nil {
			err = fmt.Errorf("panic in thrift receive: %v", r)
		}
	}()

	if fut.method != recvMethod {
		return NewApplicationException(WRONG_METHOD_NAME, fmt.Sprintf("%s failed: wrong method name '%s'", fut.method, recvMethod))
	}

	// XXX: We could potentially check if the context was cancelled here
	// and .Skip over the struct in the channel

	switch mTypeID {
	case REPLY:
		if err := fut.response.Read(c.protocol); err != nil {
			return err
		}

		return c.protocol.ReadMessageEnd()
	case EXCEPTION:
		err := NewApplicationException(UNKNOWN_APPLICATION_EXCEPTION, "Unknown exception")

		recvdErr, readErr := err.Read(c.protocol)

		if readErr != nil {
			return readErr
		}

		if msgEndErr := c.protocol.ReadMessageEnd(); msgEndErr != nil {
			return msgEndErr
		}
		return recvdErr
	default:
		return NewApplicationException(INVALID_MESSAGE_TYPE_EXCEPTION, fmt.Sprintf("%s failed: invalid message type", fut.method))
	}
}

func (c *PipelineChannel) getFuture(seqID int32) (*requestFuture, error) {
	c.futlock.Lock()
	defer c.futlock.Unlock()
	fut, ok := c.pending[seqID]
	if !ok {
		return nil, NewProtocolException(fmt.Errorf("invalid sequence ID received: %d", seqID))
	}
	delete(c.pending, seqID)
	return fut, nil
}

func (c *PipelineChannel) cancelFutures(err error) {
	c.futlock.Lock()
	for seqID, fut := range c.pending {
		if err == nil {
			err = fmt.Errorf("thrift request channel closing")
		}
		fut.done <- err
		close(fut.done)
		delete(c.pending, seqID)
	}
	c.futlock.Unlock()
}

func (c *PipelineChannel) recvLoop() (err error) {
	for {
		recvMethod, mTypeID, msgSeqID, err := c.protocol.ReadMessageBegin()
		if err != nil {
			return err
		}

		select {
		case <-c.ctx.Done():
			// if the connection context is cancelled, make sure to close the channel
			// as well, to stop all clients
			c.Close()
			return nil
		default:
		}

		fut, err := c.getFuture(msgSeqID)
		if err != nil {
			return err
		}

		fut.done <- c.recvOne(fut, recvMethod, mTypeID)
		close(fut.done)
	}

}
