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
	"io"
	"strings"
	"testing"

	"github.com/facebook/fbthrift/thrift/lib/go/thrift/types"
)

func TestReadWriteCompactProtocol(t *testing.T) {
	ReadWriteProtocolTest(t, func(transport io.ReadWriter) types.Format { return NewCompactFormat(transport) })
	// CompactProtocol is capable of reading and writing in different goroutines.
	ReadWriteProtocolParallelTest(t, func(transport io.ReadWriter) types.Format { return NewCompactFormat(transport) })
	transports := []io.ReadWriter{
		NewMemoryBuffer(),
	}
	for _, trans := range transports {
		p := NewCompactFormat(trans)
		ReadWriteBool(t, p)
		p = NewCompactFormat(trans)
		ReadWriteByte(t, p)
		p = NewCompactFormat(trans)
		ReadWriteI16(t, p)
		p = NewCompactFormat(trans)
		ReadWriteI32(t, p)
		p = NewCompactFormat(trans)
		ReadWriteI64(t, p)
		p = NewCompactFormat(trans)
		ReadWriteDouble(t, p)
		p = NewCompactFormat(trans)
		ReadWriteFloat(t, p)
		p = NewCompactFormat(trans)
		ReadWriteString(t, p)
		p = NewCompactFormat(trans)
		ReadWriteBinary(t, p)
		p = NewCompactFormat(trans)
		ReadWriteStruct(t, p)
	}
}

func TestInitialAllocationMapCompactProtocol(t *testing.T) {
	var m MyTestStruct
	// attempts to allocate a map of 930M elements for a 9 byte message
	data := []byte("%0\x88\x8a\x97\xb7\xc4\x030")
	format := NewCompactFormat(NewMemoryBufferWithData(data))
	err := m.Read(format)
	if err == nil {
		t.Fatalf("Parsed invalid message correctly")
	} else if !strings.Contains(err.Error(), "Invalid data length") {
		t.Fatalf("Failed for reason besides Invalid data length")
	}
}

func TestInitialAllocationListCompactProtocol(t *testing.T) {
	var m MyTestStruct
	// attempts to allocate a list of 950M elements for an 11 byte message
	data := []byte("%0\x98\xfa\xb7\xb7\xc4\xc4\x03\x01a")
	format := NewCompactFormat(NewMemoryBufferWithData(data))
	err := m.Read(format)
	if err == nil {
		t.Fatalf("Parsed invalid message correctly")
	} else if !strings.Contains(err.Error(), "Invalid data length") {
		t.Fatalf("Failed for reason besides Invalid data length")
	}
}

func TestInitialAllocationSetCompactProtocol(t *testing.T) {
	var m MyTestStruct
	// attempts to allocate a list of 950M elements for an 11 byte message
	data := []byte("%0\xa8\xfa\x97\xb7\xc4\xc4\x03\x01a")
	format := NewCompactFormat(NewMemoryBufferWithData(data))
	err := m.Read(format)
	if err == nil {
		t.Fatalf("Parsed invalid message correctly")
	} else if !strings.Contains(err.Error(), "Invalid data length") {
		t.Fatalf("Failed for reason besides Invalid data length")
	}
}

func TestInitialAllocationMapCompactProtocolLimitedR(t *testing.T) {
	var m MyTestStruct

	// attempts to allocate a map of 930M elements for a 9 byte message
	data := []byte("%0\x88\x8a\x97\xb7\xc4\x030")
	p := NewCompactFormat(NewMemoryBufferWithData(data))

	err := m.Read(p)
	if err == nil {
		t.Fatalf("Parsed invalid message correctly")
	} else if !strings.Contains(err.Error(), "Invalid data length") {
		t.Fatalf("Failed for reason besides Invalid data length")
	}
}
