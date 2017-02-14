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
	"fmt"
)

type THeaderProtocol struct {
	TProtocol
	origTransport TTransport
	trans         *THeaderTransport

	protoID ProtocolID
}

type THeaderProtocolFactory struct{}

func NewTHeaderProtocolFactory() *THeaderProtocolFactory {
	return &THeaderProtocolFactory{}
}

func (p *THeaderProtocolFactory) GetProtocol(trans TTransport) TProtocol {
	return NewTHeaderProtocol(trans)
}

func NewTHeaderProtocol(trans TTransport) *THeaderProtocol {
	p := &THeaderProtocol{
		origTransport: trans,
		protoID:       CompactProtocol,
	}
	if et, ok := trans.(*THeaderTransport); ok {
		p.trans = et
	} else {
		p.trans = NewTHeaderTransport(trans)
	}

	// Effectively an invariant violation.
	if err := p.ResetProtocol(); err != nil {
		panic(err)
	}
	return p
}

func (p *THeaderProtocol) ResetProtocol() error {
	if p.TProtocol != nil && p.protoID == p.trans.ProtocolID() {
		return nil
	}

	p.protoID = p.trans.ProtocolID()
	switch p.protoID {
	case BinaryProtocol:
		p.TProtocol = NewTBinaryProtocol(p.trans, false, false)
	case CompactProtocol:
		p.TProtocol = NewTCompactProtocol(p.trans)
	default:
		return NewTProtocolException(fmt.Errorf("Unknown protocol id: %#x", p.protoID))
	}
	return nil
}

//
// Writing methods.
//

func (p *THeaderProtocol) WriteMessageBegin(name string, typeId TMessageType, seqid int32) error {
	p.ResetProtocol()
	// FIXME: Python is doing this -- don't know if it's correct.
	// Should we be using this seqid or the header's?
	if typeId == CALL || typeId == ONEWAY {
		p.trans.SetSeqID(uint32(seqid))
	}
	return p.TProtocol.WriteMessageBegin(name, typeId, seqid)
}

//
// Reading methods.
//

func (p *THeaderProtocol) ReadMessageBegin() (name string, typeId TMessageType, seqid int32, err error) {
	if typeId == INVALID_TMESSAGE_TYPE {
		if err = p.trans.ResetProtocol(); err != nil {
			return name, EXCEPTION, seqid, err
		}
	}

	err = p.ResetProtocol()
	if err != nil {
		return name, EXCEPTION, seqid, err
	}

	return p.TProtocol.ReadMessageBegin()
}

func (p *THeaderProtocol) Flush() (err error) {
	return NewTProtocolException(p.trans.Flush())
}

func (p *THeaderProtocol) Skip(fieldType TType) (err error) {
	return SkipDefaultDepth(p, fieldType)
}

func (p *THeaderProtocol) Transport() TTransport {
	return p.origTransport
}
