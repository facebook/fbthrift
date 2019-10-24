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
	"strings"
	"testing"
	"time"
)

func TestReadWriteBinaryProtocol(t *testing.T) {
	ReadWriteProtocolTest(t, NewBinaryProtocolFactoryDefault())
}

func TestSkipUnknownTypeBinaryProtocol(t *testing.T) {
	var m MyTestStruct
	d := NewDeserializer()
	f := NewBinaryProtocolFactoryDefault()
	d.Protocol = f.GetProtocol(d.Transport)
	// skip over a map with invalid key/value type and 1.7B entries
	data := []byte("\n\x10\rO\t6\x03\n\n\n\x10\r\n\tslice\x00")
	start := time.Now()
	err := d.Read(&m, data)
	if err == nil {
		t.Fatalf("Parsed invalid message correctly")
	} else if !strings.Contains(err.Error(), "unknown type") {
		t.Fatalf("Failed for reason besides unknown type")
	}

	if time.Now().Sub(start).Seconds() > 5 {
		t.Fatalf("It should not take seconds to parse a small message")
	}
}
