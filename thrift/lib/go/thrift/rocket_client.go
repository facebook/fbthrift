/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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
	"maps"
	"net"
	"time"
)

type rocketClient struct {
	Encoder
	Decoder

	// rsocket client state
	client RSocketClient

	resultData []byte
	resultErr  error

	timeout time.Duration

	protoID ProtocolID
	zstd    bool

	messageName string
	writeType   MessageType
	seqID       int32

	reqHeaders        map[string]string
	respHeaders       map[string]string
	persistentHeaders map[string]string

	rbuf *MemoryBuffer
	wbuf *MemoryBuffer
}

// NewRocketClient creates a new Rocket client given an RSocketClient.
func NewRocketClient(client RSocketClient, protoID ProtocolID, timeout time.Duration, persistentHeaders map[string]string) (Protocol, error) {
	return newRocketClientFromRsocket(client, protoID, timeout, persistentHeaders)
}

func newRocketClient(conn net.Conn, protoID ProtocolID, timeout time.Duration, persistentHeaders map[string]string) (Protocol, error) {
	return newRocketClientFromRsocket(newRSocketClient(conn), protoID, timeout, persistentHeaders)
}

func newRocketClientFromRsocket(client RSocketClient, protoID ProtocolID, timeout time.Duration, persistentHeaders map[string]string) (Protocol, error) {
	p := &rocketClient{
		client:            client,
		protoID:           protoID,
		persistentHeaders: persistentHeaders,
		rbuf:              NewMemoryBuffer(),
		wbuf:              NewMemoryBuffer(),
		timeout:           timeout,
		reqHeaders:        make(map[string]string),
		zstd:              false, // zstd adds a performance overhead, so we default to false
	}
	switch p.protoID {
	case ProtocolIDBinary:
		p.Decoder = newBinaryDecoder(p.rbuf)
		p.Encoder = newBinaryEncoder(p.wbuf)
	case ProtocolIDCompact:
		p.Decoder = newCompactDecoder(p.rbuf)
		p.Encoder = newCompactEncoder(p.wbuf)
	default:
		return nil, NewProtocolException(fmt.Errorf("Unknown protocol id: %#x", p.protoID))
	}
	return p, nil
}

func (p *rocketClient) WriteMessageBegin(name string, typeID MessageType, seqid int32) error {
	p.wbuf.Reset()
	p.seqID = seqid
	p.writeType = typeID
	p.messageName = name
	return nil
}

func (p *rocketClient) WriteMessageEnd() error {
	return nil
}

func (p *rocketClient) Flush() (err error) {
	dataBytes := p.wbuf.Bytes()
	if err := p.client.SendSetup(p.serverMetadataPush); err != nil {
		return err
	}
	headers := unionMaps(p.reqHeaders, p.persistentHeaders)
	if p.writeType == ONEWAY {
		return p.client.FireAndForget(p.messageName, p.protoID, p.writeType, headers, p.zstd, dataBytes)
	}
	if p.writeType != CALL {
		return nil
	}
	ctx := context.Background()
	if p.timeout > 0 {
		var cancel context.CancelFunc
		ctx, cancel = context.WithTimeout(ctx, p.timeout)
		defer cancel()
	}
	p.respHeaders, p.resultData, p.resultErr = p.client.RequestResponse(ctx, p.messageName, p.protoID, p.writeType, headers, p.zstd, dataBytes)
	clear(p.reqHeaders)
	return nil
}

func unionMaps(dst, src map[string]string) map[string]string {
	if dst == nil {
		return src
	}
	if src == nil {
		return dst
	}
	maps.Copy(dst, src)
	return dst
}

func (p *rocketClient) serverMetadataPush(zstd bool, drain bool) {
	// zstd is only supported if both the client and the server support it.
	p.zstd = p.zstd && zstd
}

func (p *rocketClient) ReadMessageBegin() (string, MessageType, int32, error) {
	name := p.messageName
	if p.resultErr != nil {
		return name, EXCEPTION, p.seqID, p.resultErr
	}

	p.rbuf.Init(p.resultData)
	return name, REPLY, p.seqID, nil
}

func (p *rocketClient) ReadMessageEnd() error {
	return nil
}

func (p *rocketClient) Skip(fieldType Type) (err error) {
	return SkipDefaultDepth(p, fieldType)
}

func (p *rocketClient) SetRequestHeader(key, value string) {
	p.reqHeaders[key] = value
}

func (p *rocketClient) GetResponseHeaders() map[string]string {
	return p.respHeaders
}

func (p *rocketClient) Close() error {
	return p.client.Close()
}
