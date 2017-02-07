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
	"bytes"
	"io/ioutil"
	"testing"
)

func TestHeaderTransport(t *testing.T) {
	trans := NewTHeaderTransport(NewTMemoryBuffer())
	TransportTest(t, trans, trans)
}

func testRWOnce(t *testing.T, n int, data []byte, trans *THeaderTransport) {
	_, err := trans.Write(data)
	if err != nil {
		t.Fatalf("failed to write frame %d: %s", n, err)
	}
	err = trans.Flush()
	if err != nil {
		t.Fatalf("failed to xmit frame %d: %s", n, err)
	}

	err = trans.ResetProtocol()
	if err != nil {
		t.Fatalf("failed to reset proto for frame %d: %s", n, err)
	}
	frame, err := ioutil.ReadAll(trans)

	if err != nil {
		t.Fatalf("failed to recv frame %d: %s", n, err)
	}

	if bytes.Compare(data, frame) != 0 {
		t.Fatalf("data sent does not match receieve on frame %d", n)
	}
}

func TestHeaderTransportRWMultiple(t *testing.T) {
	tmb := NewTMemoryBuffer()
	trans := NewTHeaderTransport(tmb)

	// Test Junk Data
	testRWOnce(t, 1, []byte("ASDF"), trans)
	// Some sane thrift req/replies
	testRWOnce(t, 2, GetStatusCallData, trans)
	testRWOnce(t, 3, GetStatusReplyData, trans)
}
