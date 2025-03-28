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
	"fmt"
	"maps"

	"github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
	"github.com/facebook/fbthrift/thrift/lib/thrift/rpcmetadata"
	"github.com/rsocket/rsocket-go/payload"
)

type requestPayload struct {
	metadata *rpcmetadata.RequestRpcMetadata
	data     []byte
	typeID   types.MessageType
	protoID  types.ProtocolID
}

func encodeRequestPayload(
	name string,
	protoID types.ProtocolID,
	rpcKind rpcmetadata.RpcKind,
	headers map[string]string,
	zstd bool,
	dataBytes []byte,
) (payload.Payload, error) {
	metadata := rpcmetadata.NewRequestRpcMetadata()
	metadata.SetName(&name)
	rpcProtocolID, err := protocolIDToRPCProtocolID(protoID)
	if err != nil {
		return nil, err
	}
	metadata.SetProtocol(&rpcProtocolID)
	metadata.SetKind(&rpcKind)
	if zstd {
		compression := rpcmetadata.CompressionAlgorithm_ZSTD
		metadata.SetCompression(&compression)
	}
	metadata.OtherMetadata = make(map[string]string)
	maps.Copy(metadata.OtherMetadata, headers)
	metadataBytes, err := EncodeCompact(metadata)
	if err != nil {
		return nil, err
	}
	if zstd {
		dataBytes, err = compressZstd(dataBytes)
		if err != nil {
			return nil, err
		}
	}
	pay := payload.New(dataBytes, metadataBytes)
	return pay, nil
}

func decodeRequestPayload(msg payload.Payload) (*requestPayload, error) {
	msg = payload.Clone(msg)

	metadataBytes, ok := msg.Metadata()
	if !ok {
		return nil, fmt.Errorf("request payload is missing metadata")
	}

	metadata := &rpcmetadata.RequestRpcMetadata{}
	err := DecodeCompact(metadataBytes, metadata)
	if err != nil {
		return nil, err
	}

	typeID, err := rpcKindToMessageType(metadata.GetKind())
	if err != nil {
		return nil, err
	}

	protoID, err := rpcProtocolIDToProtocolID(metadata.GetProtocol())
	if err != nil {
		return nil, err
	}

	dataBytes := msg.Data()
	switch metadata.GetCompression() {
	case rpcmetadata.CompressionAlgorithm_ZSTD:
		dataBytes, err = decompressZstd(dataBytes)
	case rpcmetadata.CompressionAlgorithm_ZLIB:
		dataBytes, err = decompressZlib(dataBytes)
	}
	if err != nil {
		return nil, fmt.Errorf("request payload decompression failed: %w", err)
	}

	res := &requestPayload{
		metadata: metadata,
		typeID:   typeID,
		protoID:  protoID,
		data:     dataBytes,
	}
	return res, nil
}

func (r *requestPayload) Data() []byte {
	return r.data
}

func (r *requestPayload) HasMetadata() bool {
	return r.metadata != nil
}

func (r *requestPayload) Name() string {
	if r.metadata == nil {
		return ""
	}
	return r.metadata.GetName()
}

func (r *requestPayload) TypeID() types.MessageType {
	return r.typeID
}

func (r *requestPayload) ProtoID() types.ProtocolID {
	return r.protoID
}

func (r *requestPayload) Zstd() bool {
	return r.metadata != nil && r.metadata.GetCompression() == rpcmetadata.CompressionAlgorithm_ZSTD
}

func (r *requestPayload) Headers() map[string]string {
	if r.metadata == nil {
		return nil
	}

	headersMap := make(map[string]string)
	maps.Copy(headersMap, r.metadata.GetOtherMetadata())
	if r.metadata.IsSetClientId() {
		headersMap["client_id"] = r.metadata.GetClientId()
	}
	if r.metadata.IsSetLoadMetric() {
		headersMap["load"] = r.metadata.GetLoadMetric()
	}
	if r.metadata.IsSetClientTimeoutMs() {
		headersMap["client_timeout"] = fmt.Sprintf("%d", r.metadata.GetClientTimeoutMs())
	}
	return headersMap
}

func protocolIDToRPCProtocolID(protocolID types.ProtocolID) (rpcmetadata.ProtocolId, error) {
	switch protocolID {
	case types.ProtocolIDBinary:
		return rpcmetadata.ProtocolId_BINARY, nil
	case types.ProtocolIDCompact:
		return rpcmetadata.ProtocolId_COMPACT, nil
	}
	return 0, fmt.Errorf("unsupported ProtocolID %v", protocolID)
}

func rpcProtocolIDToProtocolID(protocolID rpcmetadata.ProtocolId) (types.ProtocolID, error) {
	switch protocolID {
	case rpcmetadata.ProtocolId_BINARY:
		return types.ProtocolIDBinary, nil
	case rpcmetadata.ProtocolId_COMPACT:
		return types.ProtocolIDCompact, nil
	}
	return 0, fmt.Errorf("unsupported ProtocolId %v", protocolID)
}

func rpcKindToMessageType(kind rpcmetadata.RpcKind) (types.MessageType, error) {
	switch kind {
	case rpcmetadata.RpcKind_SINGLE_REQUEST_SINGLE_RESPONSE:
		return types.CALL, nil
	case rpcmetadata.RpcKind_SINGLE_REQUEST_NO_RESPONSE:
		return types.ONEWAY, nil
	}
	return 0, fmt.Errorf("unsupported RpcKind %v", kind)
}
