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
	"bytes"
	"compress/zlib"
	"testing"
)

type closeableBuffer struct {
	bytes.Buffer
}

func (b *closeableBuffer) Close() error {
	b.Reset()
	return nil
}

func TestZlibTransport(t *testing.T) {
	trans, err := NewZlibTransport(&closeableBuffer{}, zlib.BestCompression)
	if err != nil {
		t.Fatal(err)
	}
	TransportTest(t, trans, trans)
}
