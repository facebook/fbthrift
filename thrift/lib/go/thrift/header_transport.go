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
	"bufio"
	"bytes"
	"compress/zlib"
	"encoding/binary"
	"fmt"
	"io/ioutil"
	"math"
)

const (
	DefaultProtoID    = CompactProtocol
	DefaultClientType = HeaderClientType
)

type tHeaderTransportFactory struct {
	factory TTransportFactory
}

func NewTHeaderTransportFactory(factory TTransportFactory) TTransportFactory {
	return &tHeaderTransportFactory{factory: factory}
}

func (p *tHeaderTransportFactory) GetTransport(base TTransport) TTransport {
	return NewTHeaderTransport(base)
}

type THeaderTransport struct {
	transport TTransport

	// Used on read
	rbuf       *bufio.Reader
	framebuf   byteReader
	readHeader *tHeader
	// remaining bytes in the current frame. If 0, read in a new frame.
	frameSize uint64

	// Used on write
	wbuf                       *bytes.Buffer
	identity                   string
	writeInfoHeaders           map[string]string
	persistentWriteInfoHeaders map[string]string

	// Negotiated
	protoID         ProtocolID
	seqID           uint32
	flags           uint16
	clientType      ClientType
	writeTransforms []TransformID
}

// NewTHeaderTransport Create a new transport with defaults.
func NewTHeaderTransport(transport TTransport) *THeaderTransport {
	return &THeaderTransport{
		transport: transport,
		rbuf:      bufio.NewReader(transport),
		framebuf:  newLimitedByteReader(bytes.NewReader(nil), 0),
		frameSize: 0,

		wbuf:                       bytes.NewBuffer(nil),
		writeInfoHeaders:           map[string]string{},
		persistentWriteInfoHeaders: map[string]string{},

		protoID:         DefaultProtoID,
		flags:           0,
		clientType:      DefaultClientType,
		writeTransforms: []TransformID{},
	}
}

func (t *THeaderTransport) SetSeqID(seq uint32) {
	t.seqID = seq
}

func (t *THeaderTransport) SeqID() uint32 {
	return t.seqID
}

func (t *THeaderTransport) Identity() string {
	return t.identity
}

func (t *THeaderTransport) SetIdentity(identity string) {
	t.identity = identity
}

func (t *THeaderTransport) PeerIdentity() string {
	v, ok := t.ReadHeader(IdentityHeader)
	vers, versok := t.ReadHeader(IDVersionHeader)
	if ok && versok && vers == IDVersion {
		return v
	}
	return ""
}

func (t *THeaderTransport) SetPersistentHeader(key, value string) {
	t.persistentWriteInfoHeaders[key] = value
}

func (t *THeaderTransport) PersistentHeader(key string) (string, bool) {
	v, ok := t.persistentWriteInfoHeaders[key]
	return v, ok
}

func (t *THeaderTransport) PersistentHeaders() map[string]string {
	res := map[string]string{}
	for k, v := range t.persistentWriteInfoHeaders {
		res[k] = v
	}
	return res
}

func (t *THeaderTransport) ClearPersistentHeaders() {
	t.persistentWriteInfoHeaders = map[string]string{}
}

func (t *THeaderTransport) SetHeader(key, value string) {
	t.writeInfoHeaders[key] = value
}

func (t *THeaderTransport) Header(key string) (string, bool) {
	v, ok := t.writeInfoHeaders[key]
	return v, ok
}

func (t *THeaderTransport) Headers() map[string]string {
	res := map[string]string{}
	for k, v := range t.writeInfoHeaders {
		res[k] = v
	}
	return res
}

func (t *THeaderTransport) ClearHeaders() {
	t.writeInfoHeaders = map[string]string{}
}

func (t *THeaderTransport) ReadHeader(key string) (string, bool) {
	if t.readHeader == nil {
		return "", false
	}
	// per the C++ implementation, prefer persistent headers
	if v, ok := t.readHeader.pHeaders[key]; ok {
		return v, ok
	}
	v, ok := t.readHeader.headers[key]
	return v, ok
}

func (t *THeaderTransport) ReadHeaders() map[string]string {
	res := map[string]string{}
	if t.readHeader == nil {
		return res
	}
	for k, v := range t.readHeader.headers {
		res[k] = v
	}
	for k, v := range t.readHeader.pHeaders {
		res[k] = v
	}
	return res
}

func (t *THeaderTransport) ProtocolID() ProtocolID {
	return t.protoID
}

func (t *THeaderTransport) SetProtocolID(protoID ProtocolID) error {
	if !(protoID == BinaryProtocol || protoID == CompactProtocol) {
		return NewTTransportException(
			NOT_IMPLEMENTED, fmt.Sprintf("unimplemented proto ID: %s (%#x)", protoID.String(), int64(protoID)),
		)
	}
	t.protoID = protoID
	return nil
}

func (t *THeaderTransport) AddTransform(trans TransformID) error {
	if sup, ok := supportedTransforms[trans]; !ok || !sup {
		return NewTTransportException(
			NOT_IMPLEMENTED, fmt.Sprintf("unimplemented transform ID: %s (%#x)", trans.String(), int64(trans)),
		)
	}
	for _, t := range t.writeTransforms {
		if t == trans {
			return nil
		}
	}
	t.writeTransforms = append(t.writeTransforms, trans)
	return nil
}

// applyUntransform Fully read the frame and untransform into a local buffer
// we need to know the full size of the untransformed data
func (t *THeaderTransport) applyUntransform() error {
	out, err := ioutil.ReadAll(t.framebuf)
	if err != nil {
		return err
	}
	t.frameSize = uint64(len(out))
	t.framebuf = newLimitedByteReader(bytes.NewBuffer(out), int64(t.readHeader.payloadLen))
	return nil
}

// ResetProtocol Needs to be called between every frame receive (BeginMessageRead)
// We do this to read out the header for each frame. This contains the length of the
// frame and protocol / metadata info.
func (t *THeaderTransport) ResetProtocol() error {
	t.readHeader = nil
	// TODO(carlverge): We should probably just read in the whole
	// frame here. A bit of extra memory, probably a lot less CPU.
	// Needs benchmark to test.

	hdr := &tHeader{}
	// Consume the header from the input stream
	err := hdr.Read(t.rbuf)
	if err != nil {
		return NewTTransportExceptionFromError(err)
	}

	// Set new header
	t.readHeader = hdr
	// Adopt the client's protocol
	t.protoID = hdr.protoID
	t.clientType = hdr.clientType
	t.seqID = hdr.seq
	t.flags = hdr.flags

	// If the client is using unframed, just pass up the data to the protocol
	if t.clientType == UnframedDeprecated || t.clientType == UnframedCompactDeprecated {
		t.framebuf = t.rbuf
		return nil
	}

	// Make sure we can't read past the current frame length
	t.frameSize = hdr.payloadLen
	t.framebuf = newLimitedByteReader(t.rbuf, int64(hdr.payloadLen))

	for _, trans := range hdr.transforms {
		xformer, terr := trans.Untransformer()
		if terr != nil {
			return NewTTransportExceptionFromError(terr)
		}

		t.framebuf, terr = xformer(t.framebuf)
		if terr != nil {
			return NewTTransportExceptionFromError(terr)
		}
	}

	// Fully read the frame and apply untransforms if we have them
	if len(hdr.transforms) > 0 {
		err = t.applyUntransform()
		if err != nil {
			return NewTTransportExceptionFromError(err)
		}
	}

	// respond in kind with the client's transforms
	t.writeTransforms = hdr.transforms

	return nil
}

// Open Open the internal transport
func (t *THeaderTransport) Open() error {
	return t.transport.Open()
}

// IsOpen Is the current transport open
func (t *THeaderTransport) IsOpen() bool {
	return t.transport.IsOpen()
}

// Close Close the internal transport
func (t *THeaderTransport) Close() error {
	return t.transport.Close()
}

// Read Read from the current framebuffer. EOF if the frame is done.
func (t *THeaderTransport) Read(buf []byte) (int, error) {
	// If we detected unframed, just pass the transport up
	if t.clientType == UnframedDeprecated || t.clientType == UnframedCompactDeprecated {
		return t.framebuf.Read(buf)
	}
	n, err := t.framebuf.Read(buf)
	// Shouldn't be possibe, but just in case the frame size was flubbed
	if uint64(n) > t.frameSize {
		n = int(t.frameSize)
	}
	t.frameSize -= uint64(n)
	return n, err
}

// ReadByte Read a single byte from the current framebuffer. EOF if the frame is done.
func (t *THeaderTransport) ReadByte() (byte, error) {
	// If we detected unframed, just pass the transport up
	if t.clientType == UnframedDeprecated || t.clientType == UnframedCompactDeprecated {
		return t.framebuf.ReadByte()
	}
	b, err := t.framebuf.ReadByte()
	t.frameSize--
	return b, err
}

// Write Write multiple bytes to the framebuffer, does not send to transport.
func (t *THeaderTransport) Write(buf []byte) (int, error) {
	n, err := t.wbuf.Write(buf)
	return n, NewTTransportExceptionFromError(err)
}

// WriteByte Write a single byte to the framebuffer, does not send to transport.
func (t *THeaderTransport) WriteByte(c byte) error {
	err := t.wbuf.WriteByte(c)
	return NewTTransportExceptionFromError(err)
}

// WriteString Write a string to the framebuffer, does not send to transport.
func (t *THeaderTransport) WriteString(s string) (int, error) {
	n, err := t.wbuf.WriteString(s)
	return n, NewTTransportExceptionFromError(err)
}

// RemainingBytes Return how many bytes remain in the current recv framebuffer.
func (t *THeaderTransport) RemainingBytes() uint64 {
	if t.clientType == UnframedDeprecated || t.clientType == UnframedCompactDeprecated {
		// We cannot really tell the size without reading the whole struct in here
		return math.MaxUint64
	}
	return t.frameSize
}

func applyTransforms(buf *bytes.Buffer, transforms []TransformID) (*bytes.Buffer, error) {
	tmpbuf := bytes.NewBuffer(nil)
	for _, trans := range transforms {
		switch trans {
		case TransformZlib:
			zwr := zlib.NewWriter(tmpbuf)
			_, err := buf.WriteTo(zwr)
			if err != nil {
				return nil, err
			}
			err = zwr.Close()
			if err != nil {
				return nil, err
			}
			buf, tmpbuf = tmpbuf, buf
			tmpbuf.Reset()
		default:
			return nil, NewTTransportException(
				NOT_IMPLEMENTED, fmt.Sprintf("unimplemented transform ID: %s (%#x)", trans.String(), int64(trans)),
			)
		}
	}
	return buf, nil
}

func (t *THeaderTransport) flushHeader() error {
	hdr := tHeader{}
	hdr.headers = t.writeInfoHeaders
	hdr.pHeaders = t.persistentWriteInfoHeaders
	hdr.protoID = t.protoID
	hdr.clientType = t.clientType
	hdr.seq = t.seqID
	hdr.flags = t.flags
	hdr.transforms = t.writeTransforms

	if t.identity != "" {
		hdr.headers[IdentityHeader] = t.identity
		hdr.headers[IDVersionHeader] = IDVersion
	}

	outbuf, err := applyTransforms(t.wbuf, t.writeTransforms)
	if err != nil {
		return NewTTransportExceptionFromError(err)
	}
	t.wbuf = outbuf

	hdr.payloadLen = uint64(t.wbuf.Len())
	err = hdr.calcLenFromPayload()
	if err != nil {
		return NewTTransportExceptionFromError(err)
	}

	hdrbuf := bytes.NewBuffer(make([]byte, 64))
	hdrbuf.Reset()
	err = hdr.Write(hdrbuf)
	if err != nil {
		return NewTTransportExceptionFromError(err)
	}

	_, err = hdrbuf.WriteTo(t.transport)
	return NewTTransportExceptionFromError(err)
}

func (t *THeaderTransport) flushFramed() error {
	buflen := t.wbuf.Len()
	framesize := uint32(buflen)
	if buflen > int(MaxFrameSize) {
		return NewTTransportException(
			INVALID_FRAME_SIZE,
			fmt.Sprintf("cannot send bigframe of size %d", buflen),
		)
	}

	err := binary.Write(t.transport, binary.BigEndian, framesize)
	return NewTTransportExceptionFromError(err)
}

func (t *THeaderTransport) Flush() error {
	// Closure incase wbuf pointer changes in xform
	defer func(tp *THeaderTransport) {
		tp.wbuf.Reset()
	}(t)
	var err error

	switch t.clientType {
	case HeaderClientType:
		err = t.flushHeader()
	case FramedDeprecated:
		err = t.flushFramed()
	case FramedCompact:
		err = t.flushFramed()
	case UnframedCompactDeprecated:
		err = nil
	case UnframedDeprecated:
		err = nil
	default:
		return NewTTransportException(
			UNKNOWN_TRANSPORT_EXCEPTION,
			fmt.Sprintf("tHeader cannot flush for clientType %s", t.clientType.String()),
		)
	}

	if err != nil {
		return err
	}

	// Writeout the payload
	if t.wbuf.Len() > 0 {
		_, err = t.wbuf.WriteTo(t.transport)
		if err != nil {
			return NewTTransportExceptionFromError(err)
		}
	}

	// Remove the non-persistent headers on flush
	t.ClearHeaders()

	err = t.transport.Flush()
	return NewTTransportExceptionFromError(err)
}
