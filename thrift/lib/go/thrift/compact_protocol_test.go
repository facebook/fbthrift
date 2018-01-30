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
	"testing"
)

func TestReadWriteCompactProtocol(t *testing.T) {
	ReadWriteProtocolTest(t, NewCompactProtocolFactory())
	transports := []Transport{
		NewMemoryBuffer(),
		NewStreamTransportRW(bytes.NewBuffer(make([]byte, 0, 16384))),
		NewFramedTransport(NewMemoryBuffer()),
	}
	for _, trans := range transports {
		p := NewCompactProtocol(trans)
		ReadWriteBool(t, p, trans)
		p = NewCompactProtocol(trans)
		ReadWriteByte(t, p, trans)
		p = NewCompactProtocol(trans)
		ReadWriteI16(t, p, trans)
		p = NewCompactProtocol(trans)
		ReadWriteI32(t, p, trans)
		p = NewCompactProtocol(trans)
		ReadWriteI64(t, p, trans)
		p = NewCompactProtocol(trans)
		ReadWriteDouble(t, p, trans)
		p = NewCompactProtocol(trans)
		ReadWriteFloat(t, p, trans)
		p = NewCompactProtocol(trans)
		ReadWriteString(t, p, trans)
		p = NewCompactProtocol(trans)
		ReadWriteBinary(t, p, trans)
		trans.Close()
	}
}
