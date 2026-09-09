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
	"errors"
	"fmt"
	"iter"
	"maps"
	"net"
	"runtime"
	"time"

	"github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
	"github.com/facebook/fbthrift/thrift/lib/thrift/rpcmetadata"
)

type rocketClient struct {
	// rsocket client state
	client RSocketClient
	// Handle containing the cleanup (.Close call) for the 'client' (RSocketClient) above,
	// for when the enclosing 'rocketClient' object goes out of scope, and in case the user
	// forgets to explicitly close the client.
	// This cleanup is VERY IMPORTANT - not cleaning up can lead to Goroutine and FD leaks!
	clientCleanup runtime.Cleanup

	ioTimeout time.Duration

	protoID types.ProtocolID

	persistentHeaders map[string]string
}

var _ RequestChannel = (*rocketClient)(nil)
var _ types.RequestChannelExtended = (*rocketClient)(nil)

func newRocketClient(
	conn net.Conn,
	protoID types.ProtocolID,
	ioTimeout time.Duration,
	persistentHeaders map[string]string,
) (RequestChannel, error) {
	var rpcProtocolID rpcmetadata.ProtocolId
	switch protoID {
	case types.ProtocolIDBinary:
		rpcProtocolID = rpcmetadata.ProtocolId_BINARY
	case types.ProtocolIDCompact:
		rpcProtocolID = rpcmetadata.ProtocolId_COMPACT
	default:
		return nil, fmt.Errorf("unsupported ProtocolID: %d", protoID)
	}
	rsocketClient := newRSocketClient(conn, rpcProtocolID, protoID)
	p := &rocketClient{
		client:            rsocketClient,
		protoID:           protoID,
		persistentHeaders: persistentHeaders,
		ioTimeout:         ioTimeout,
	}
	p.clientCleanup = runtime.AddCleanup(p,
		func(underlyingClient RSocketClient) {
			underlyingClient.Close()
		}, rsocketClient)
	return p, nil
}

func (p *rocketClient) SendRequestNoResponse(ctx context.Context, messageName string, request WritableStruct) error {
	if p.ioTimeout > 0 {
		var cancel context.CancelFunc
		ctx, cancel = context.WithTimeout(ctx, p.ioTimeout)
		defer cancel()
	}

	headers := p.getWriteHeaders(ctx)
	return p.client.FireAndForget(ctx, messageName, headers, request)
}

func (p *rocketClient) SendRequestResponse(ctx context.Context, messageName string, request WritableStruct, response ReadableResult) error {
	if p.ioTimeout > 0 {
		var cancel context.CancelFunc
		ctx, cancel = context.WithTimeout(ctx, p.ioTimeout)
		defer cancel()
	}

	headers := p.getWriteHeaders(ctx)
	resultData, resultErr := p.client.RequestResponse(ctx, messageName, headers, request)
	if resultErr != nil {
		return resultErr
	}

	err := decodeResultOrException(p.protoID, resultData, response)
	if err != nil {
		return err
	}
	return nil
}

func (p *rocketClient) SendRequestStream(
	ctx context.Context,
	messageName string,
	request WritableStruct,
	response ReadableResult,
	newStreamElemFn func() ReadableResult,
) (iter.Seq2[ReadableStruct, error], error) {
	if ctx.Done() == nil {
		// We require that the context is cancellable, to prevent goroutine leaks.
		return nil, errors.New("context does not support cancellation")
	}

	headers := p.getWriteHeaders(ctx)
	resultData, streamSeq, resultErr := p.client.RequestStream(ctx, messageName, headers, request, newStreamElemFn)
	if resultErr != nil {
		return nil, resultErr
	}

	err := decodeResultOrException(p.protoID, resultData, response)
	if err != nil {
		return nil, err
	}
	return streamSeq, nil
}

func (p *rocketClient) SendRequestSink(
	ctx context.Context,
	messageName string,
	request WritableStruct,
	firstResponse ReadableResult,
) (func(sinkSeq iter.Seq2[WritableResult, error], finalResponse ReadableResult) error, error) {
	headers := p.getWriteHeaders(ctx)
	resultData, sinkCallback, resultErr := p.client.RequestSink(ctx, messageName, headers, request)
	if resultErr != nil {
		return nil, resultErr
	}

	err := decodeResultOrException(p.protoID, resultData, firstResponse)
	if err != nil {
		return nil, err
	}
	return sinkCallback, nil
}

func (p *rocketClient) SendRequestBiDi(
	ctx context.Context,
	messageName string,
	request WritableStruct,
	firstResponse ReadableResult,
	newStreamElemFn func() ReadableResult,
) (func(sinkSeq iter.Seq2[WritableResult, error]), iter.Seq2[ReadableStruct, error], error) {
	if ctx.Done() == nil {
		// We require that the context is cancellable, to prevent goroutine leaks.
		return nil, nil, errors.New("context does not support cancellation")
	}

	headers := p.getWriteHeaders(ctx)
	resultData, sinkCallback, streamSeq, resultErr := p.client.RequestBiDiStream(ctx, messageName, headers, request, newStreamElemFn)
	if resultErr != nil {
		return nil, nil, resultErr
	}

	err := decodeResultOrException(p.protoID, resultData, firstResponse)
	if err != nil {
		return nil, nil, err
	}
	return sinkCallback, streamSeq, nil
}

func (p *rocketClient) TerminateInteraction(interactionID int64) error {
	interactionTerminate := rpcmetadata.NewInteractionTerminate().
		SetInteractionId(interactionID)
	metadata := rpcmetadata.NewClientPushMetadata().
		SetInteractionTerminate(interactionTerminate)
	return p.client.MetadataPush(context.Background(), metadata)
}

func decodeResultOrException(protoID types.ProtocolID, data []byte, result ReadableResult) error {
	err := decodeResponse(protoID, data, result)
	if err != nil {
		return err
	}
	// Declared exception (inside the response)
	if exception := result.Exception(); exception != nil {
		return exception
	}
	return nil
}

func (p *rocketClient) getWriteHeaders(ctx context.Context) map[string]string {
	rpcOpts := GetRPCOptions(ctx)
	var writeHeaders map[string]string
	if rpcOpts != nil {
		writeHeaders = rpcOpts.GetWriteHeaders()
	}
	return unionMaps(writeHeaders, p.persistentHeaders)
}

func encodeRequest(protoID types.ProtocolID, request WritableStruct) ([]byte, error) {
	switch protoID {
	case types.ProtocolIDBinary:
		return EncodeBinary(request)
	case types.ProtocolIDCompact:
		return EncodeCompact(request)
	default:
		return nil, types.NewProtocolException(fmt.Errorf("Unknown protocol id: %d", protoID))
	}
}

func decodeResponse(protoID types.ProtocolID, data []byte, response ReadableStruct) error {
	switch protoID {
	case types.ProtocolIDBinary:
		return DecodeBinary(data, response)
	case types.ProtocolIDCompact:
		return DecodeCompact(data, response)
	default:
		return types.NewProtocolException(fmt.Errorf("Unknown protocol id: %d", protoID))
	}
}

func unionMaps(args ...map[string]string) map[string]string {
	// Creates a brand new unified map and copies contents of 'args' into it.
	unifiedMap := make(map[string]string)
	for _, arg := range args {
		maps.Copy(unifiedMap, arg)
	}
	return unifiedMap
}

func (p *rocketClient) Close() error {
	// no need for the cleanup anymore (idempotent method)
	p.clientCleanup.Stop()
	return p.client.Close()
}
