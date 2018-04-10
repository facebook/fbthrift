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

// A Serializer is used to turn a Struct in to a byte stream
type Serializer struct {
	Transport *MemoryBuffer
	Protocol  Protocol
}

// Struct is the interface used to encapsulate a message that can be read and written to a protocol
type Struct interface {
	Write(p Protocol) error
	Read(p Protocol) error
}

// NewSerializer create a new serializer using the binary protocol
func NewSerializer() *Serializer {
	transport := NewMemoryBufferLen(1024)
	protocol := NewBinaryProtocolFactoryDefault().GetProtocol(transport)

	return &Serializer{transport, protocol}
}

// WriteString writes msg to the serializer and returns it as a string
func (s *Serializer) WriteString(msg Struct) (str string, err error) {
	s.Transport.Reset()

	if err = msg.Write(s.Protocol); err != nil {
		return
	}

	if err = s.Protocol.Flush(); err != nil {
		return
	}
	if err = s.Transport.Flush(); err != nil {
		return
	}

	return s.Transport.String(), nil
}

// Write writes msg to the serializer and returns it as a byte array
func (s *Serializer) Write(msg Struct) (b []byte, err error) {
	s.Transport.Reset()

	if err = msg.Write(s.Protocol); err != nil {
		return
	}

	if err = s.Protocol.Flush(); err != nil {
		return
	}

	if err = s.Transport.Flush(); err != nil {
		return
	}

	b = append(b, s.Transport.Bytes()...)
	return
}
