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

package rocket

import (
	"context"
	"testing"
	"time"

	"github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
	"github.com/facebook/fbthrift/thrift/lib/thrift/rpcmetadata"
	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

func TestRequestRPCMetadata(t *testing.T) {
	wantName := "test123"
	wantProto := rpcmetadata.ProtocolId_COMPACT
	wantKind := rpcmetadata.RpcKind_SINGLE_REQUEST_SINGLE_RESPONSE
	wantHeaders := map[string]string{"header": "1"}
	wantQueueTimeoutMs := 500
	wantClientTimeoutMs := 1000

	ctx := types.WithRPCOptions(context.Background(), &types.RPCOptions{
		QueueTimeout: time.Duration(wantQueueTimeoutMs) * time.Millisecond,
		Timeout:      time.Duration(wantClientTimeoutMs) * time.Millisecond,
	})

	payload, err := EncodeRequestPayload(
		ctx,
		wantName,
		wantProto,
		wantKind,
		wantHeaders,
		rpcmetadata.CompressionAlgorithm_NONE,
		nil,
	)
	require.NoError(t, err)

	metadata := rpcmetadata.NewRequestRpcMetadata()
	err = DecodePayloadMetadata(payload, metadata)
	require.NoError(t, err)

	assert.EqualValues(t, wantName, metadata.GetName())
	assert.EqualValues(t, wantProto, metadata.GetProtocol())
	assert.EqualValues(t, wantKind, metadata.GetKind())
	assert.EqualValues(t, wantHeaders, metadata.GetOtherMetadata())
	assert.True(t, metadata.IsSetQueueTimeoutMs())
	assert.EqualValues(t, wantQueueTimeoutMs, metadata.GetQueueTimeoutMs())
	assert.True(t, metadata.IsSetClientTimeoutMs())
	assert.EqualValues(t, wantClientTimeoutMs, metadata.GetClientTimeoutMs())
}

func TestRequestRPCMetadataClientOnlyTimeouts(t *testing.T) {
	ctx := types.WithRPCOptions(context.Background(), &types.RPCOptions{
		QueueTimeout:       500 * time.Millisecond,
		Timeout:            1000 * time.Millisecond,
		ClientOnlyTimeouts: true,
	})

	payload, err := EncodeRequestPayload(
		ctx,
		"test123",
		rpcmetadata.ProtocolId_COMPACT,
		rpcmetadata.RpcKind_SINGLE_REQUEST_SINGLE_RESPONSE,
		nil,
		rpcmetadata.CompressionAlgorithm_NONE,
		nil,
	)
	require.NoError(t, err)

	metadata := rpcmetadata.NewRequestRpcMetadata()
	err = DecodePayloadMetadata(payload, metadata)
	require.NoError(t, err)

	assert.False(t, metadata.IsSetQueueTimeoutMs())
	assert.False(t, metadata.IsSetClientTimeoutMs())
}
