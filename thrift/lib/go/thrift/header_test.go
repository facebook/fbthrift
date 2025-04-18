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
	"bufio"
	"bytes"
	"encoding/hex"
	"testing"

	"github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
	"github.com/stretchr/testify/require"
)

func MustDecodeHex(s string) []byte {
	res, err := hex.DecodeString(s)
	if err != nil {
		panic(err)
	}
	return res
}

var GetStatusCall = MustDecodeHex(
	"0000001c0fff0000000000000001020000008222000967657453746174757300",
)
var GetStatusCallData = MustDecodeHex(
	"8222000967657453746174757300",
)

var GetStatusReply = MustDecodeHex(
	"0000001f0fff0000000000000001020000008242000967657453746174757305000400",
)

var GetStatusReplyData = MustDecodeHex("8242000967657453746174757305000400")

func TestHeaderDeserSer(t *testing.T) {

	buf := bufio.NewReader(bytes.NewBuffer(GetStatusCall))
	hdr := &tHeader{}
	err := hdr.Read(buf)
	require.NoError(t, err)
	require.Equal(t, types.ProtocolIDCompact, hdr.protoID)

	wbuf := bytes.NewBuffer(nil)
	err = hdr.Write(wbuf)
	require.NoError(t, err)
}
